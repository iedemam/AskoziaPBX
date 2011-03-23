#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#include "driver.h"

struct dma_ctx {
	struct list_head list;

	void __iomem *sgl;
	void __iomem *fpga;

	u8 *dma_buf;
	u8 *rx_buf, *tx_buf;
	dma_addr_t dma_handle;
	int dma_size;

	struct device *dev;

	int irq;
};

struct dma_inst {
	struct list_head list;
	int cardid;
	void (*cb)(int cardid);
};

#define MODNAME 				"pikadma"
#define DMA_INTS				0xC0000
#define PKH_MEDIA_STREAM_BUFFER_SIZE_DEFAULT	(40 * 8) // 40 ms
#define FRAMES_PER_BUFFER			PKH_MEDIA_STREAM_BUFFER_SIZE_DEFAULT
#define FRAMES_PER_TRANSFER			8
#define FRAMESIZE				64
#define JITTER					2

/* There is only one instance of the dma */
struct dma_ctx g_dma_ctx;
static PDEVICE_EXTENSION pdx;

int taco_proc_init(PDEVICE_EXTENSION pdx);

static inline unsigned fpga_read(void __iomem *fpga, int reg)
{
	return in_be32(fpga + reg);
}

static inline void fpga_write(void __iomem *fpga, int reg, unsigned value)
{
	out_be32(fpga + reg, value);
}

void daytona_get_sgl_index(void *dummy, int *SglIndex)
{
	*SglIndex = fpga_read(g_dma_ctx.fpga, FPGA_DMA_SG_IDX) & 0x1ff;
}

static irqreturn_t dma_isr(int irq, void *context)
{
	struct dma_inst *inst;
	unsigned iccr = fpga_read(g_dma_ctx.fpga, FPGA_ICCR) & DMA_INTS;

	if (!iccr)
		return IRQ_NONE;

	fpga_write(g_dma_ctx.fpga, FPGA_ICCR, iccr);

#ifdef DMA_DISABLED
	printk(KERN_INFO "DMA interrupt in disabled state!\n");
	return IRQ_HANDLED;
#endif

	if (list_empty(&g_dma_ctx.list))
		return IRQ_HANDLED;

	if (iccr & 0x40000)
		printk(KERN_ERR "DMA underrun\n");

	if (iccr & 0x80000) { /* DMA done */

		list_for_each_entry(inst, &g_dma_ctx.list, list)
			inst->cb(inst->cardid);
#ifndef CONFIG_WARP_DAHDI
		K__HspQueueDpcs();
#endif
	}

	return IRQ_HANDLED;
}

static struct dma_inst *find_inst(struct dma_ctx *dma, int cardid)
{
	struct dma_inst *inst;

	list_for_each_entry(inst, &dma->list, list)
		if (inst->cardid == cardid)
			return inst;

	return NULL;
}

int pikadma_register_cb(int cardid, void (*cb)(int cardid))
{
	struct dma_ctx *dma = &g_dma_ctx;
	struct dma_inst *inst;
	unsigned imr = fpga_read(dma->fpga, FPGA_IMR);

	if (cardid == -1)
		return -EINVAL;

	if (find_inst(dma, cardid)) {
		printk(KERN_WARNING
		       "dma_register: cardid %d already registered.\n", cardid);
		return -EBUSY;
	}

	inst = kzalloc(sizeof(struct dma_inst), GFP_KERNEL);
	if (!inst) {
		printk(KERN_ERR "dma_register: out of memory\n");
		return -ENOMEM;
	}

	/* Disable the interrupts while adding to list. */
	fpga_write(dma->fpga, FPGA_IMR, imr & ~DMA_INTS);

	inst->cardid = cardid;
	inst->cb = cb;
	list_add_tail(&inst->list, &dma->list);

#ifndef DMA_DISABLED
	/* Always safe to reenable */
	fpga_write(dma->fpga, FPGA_IMR, imr | DMA_INTS);
	fpga_write(dma->fpga, FPGA_TDM_DMA_CTRL, 1);
#endif

	return 0;
}
EXPORT_SYMBOL(pikadma_register_cb);

#ifndef CONFIG_WARP_DAHDI
int pikadma_register(int cardid, void *dev, unsigned int irql)
{
	int status;

	status = pikadma_register_cb(cardid, K__HspCardIsrHandler);

	if (status == 0)
		if (K__HspClkSyncRegisterGetSglIndex(cardid, irql, dev,
				daytona_get_sgl_index)) {
			return PK_KERROR;
		}

	return status;
}
EXPORT_SYMBOL(pikadma_register);
#endif

int pikadma_unregister(int cardid)
{
	struct dma_ctx *dma = &g_dma_ctx;
	struct dma_inst *inst = find_inst(dma, cardid);
	unsigned imr = fpga_read(dma->fpga, FPGA_IMR);

	if (!inst)
		return -EINVAL;

	/* Disable ints while removing from list */
	fpga_write(dma->fpga, FPGA_IMR, imr & ~DMA_INTS);
	list_del(&inst->list);
	kfree(inst);

#ifndef DMA_DISABLED
	if (list_empty(&dma->list))
		fpga_write(dma->fpga, FPGA_TDM_DMA_CTRL, 0);
	else /* reenable ints */
		fpga_write(dma->fpga, FPGA_IMR, imr);
#endif

	return 0;
}
EXPORT_SYMBOL(pikadma_unregister);

void pikadma_get_buffers(void **rx_buf, void **tx_buf)
{
	struct dma_ctx *dma = &g_dma_ctx;

	if (rx_buf)
		*rx_buf = dma->rx_buf;
	if (tx_buf)
		*tx_buf = dma->tx_buf;
}
EXPORT_SYMBOL(pikadma_get_buffers);

void pikadma_disable(void)
{
	fpga_write(g_dma_ctx.fpga, FPGA_TDM_DMA_CTRL, 0);
}
EXPORT_SYMBOL(pikadma_disable);

void pikadma_enable(void)
{
	fpga_write(g_dma_ctx.fpga, FPGA_TDM_DMA_CTRL, 1);
}
EXPORT_SYMBOL(pikadma_enable);

static int create_scatter_gather(struct dma_ctx *dma)
{
	int i;
	unsigned rx_buf, tx_buf, sgl = 0;
	int size, entries, bytes;

	size = FRAMES_PER_BUFFER * FRAMESIZE;
	entries = FRAMES_PER_BUFFER / FRAMES_PER_TRANSFER;
	bytes = FRAMES_PER_TRANSFER * FRAMESIZE;

	dma->dma_buf = dma_alloc_coherent(dma->dev, size * 2,
					  &dma->dma_handle, GFP_DMA);
	if (!dma->dma_buf)
		return -ENOMEM;

	dma->dma_size = size * 2;
	dma->rx_buf = dma->dma_buf;
	dma->tx_buf = dma->rx_buf + size;

	rx_buf = dma->dma_handle;
	tx_buf = dma->dma_handle + size;

	if (debug) {
		printk(KERN_INFO "DMA RX buf %p physical %x\n",
		       dma->rx_buf, rx_buf);
		printk(KERN_INFO "    TX buf %p physical %x\n",
		       dma->tx_buf, tx_buf);
	}

	tx_buf |= 2; /* HSP requires us to interrupt on each frame pair */

	for (i = 0; i < entries; ++i) {
		if (i == entries - 1)
			tx_buf |= 1; /* wrap bit */

		fpga_write(dma->sgl, sgl, rx_buf);
		if (debug > 1)
			printk(KERN_INFO "  rx %x %08x\n", sgl, rx_buf);
		sgl += 4;
		fpga_write(dma->sgl, sgl, tx_buf);
		if (debug > 1)
			printk(KERN_INFO "  tx %x %08x\n", sgl, tx_buf);
		sgl += 4;

		rx_buf += bytes;
		tx_buf += bytes;
	}

	fpga_write(dma->fpga, FPGA_TDM_DMA_SZ,  FRAMES_PER_TRANSFER);
	fpga_write(dma->fpga, FPGA_TDM_DMA_JIT, JITTER);

	return 0;
}


static void destroy_scatter_gather(struct dma_ctx *dma)
{
	if (dma->dma_buf)
		dma_free_coherent(dma->dev, dma->dma_size,
				  dma->dma_buf, dma->dma_handle);
	dma->dma_buf = NULL;
	dma->rx_buf  = NULL;
	dma->tx_buf  = NULL;
}

int dma_init_module(PDEVICE_EXTENSION pdx)
{
	struct dma_ctx *dma = &g_dma_ctx;
	struct device_node *np;

	INIT_LIST_HEAD(&dma->list);

	/* Take straight from device extension */
	dma->fpga = pdx->bar0;
	dma->irq = pdx->info.irql;
	dma->dev = &pdx->dev->dev;

	np = of_find_compatible_node(NULL, NULL, "pika,fpga-sgl");
	if (np == NULL) {
		printk(KERN_ERR MODNAME ": Unable to find fpga-sgl\n");
		goto error_cleanup;
	}

	dma->sgl = of_iomap(np, 0);
	if (!dma->sgl) {
		printk(KERN_ERR MODNAME ": Unable to get SGL address\n");
		goto error_cleanup;
	}

	of_node_put(np);
	np = NULL;

	if (create_scatter_gather(dma)) {
		printk(KERN_ERR MODNAME ": Unable to allocate SGL\n");
		goto error_cleanup;
	}

	if (request_irq(dma->irq, dma_isr, IRQF_SHARED, "pikadma", dma)) {
		printk(KERN_ERR MODNAME ": Unable to request irq\n");
		goto error_cleanup;
	}

#ifdef DMA_DISABLED
	printk(KERN_WARNING "Warning: DMA disabled\n");
#endif

	return 0;

error_cleanup:
	of_node_put(np);

	destroy_scatter_gather(dma);

	if (dma->sgl) {
		iounmap(dma->sgl);
		dma->sgl = NULL;
	}

	return -ENOENT;
}

EXPORT_SYMBOL(dma_init_module);

void dma_exit_module(void)
{
	struct dma_ctx *dma = &g_dma_ctx;

	if (dma->sgl) {
		free_irq(dma->irq, dma);

		destroy_scatter_gather(dma);

		iounmap(dma->sgl);
		dma->sgl = NULL;
	}
}

EXPORT_SYMBOL(dma_exit_module);

void get_module_types(unsigned *moda, unsigned *modb)
{
	unsigned rev = BAR0_READ(pdx, BAR0_FPGA_REV);

	*moda = (rev >> 16) & 0xf;
	*modb = (rev >> 20) & 0xf;
}

EXPORT_SYMBOL(get_module_types);

// note that this function doesnot check for the existence of a BRI module
int get_bri_span_count(int module)
{
#define R_CHIPID	0x16
	// TODO define module A as 0 and module B as 1 
	unsigned channel = (module == 0) ? 2 : 6;
	u8 data = 0;
	fpga_read_indirect(pdx->bar0, channel, R_CHIPID, &data);

	return data;
}

EXPORT_SYMBOL(get_bri_span_count);

int __init shared_dma_init_module(void)
{
	struct device_node *np;
	struct resource res;
	void __iomem *fpga;
	int irq;
	int rc;

	if ((pdx = kzalloc(sizeof(DEVICE_EXTENSION), GFP_KERNEL)) == NULL) {
		printk(KERN_ERR "%s:%s() error allocating memory\n",
			__FILE__, __FUNCTION__);
		return -ENOMEM;
	}

	/* extract all fpga parameters from open firmware */
	if ((np = of_find_compatible_node(NULL, NULL, "pika,fpga")) == NULL) {
		printk(KERN_ERR "%s:%s() Unable to find fpga\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	if ((irq = irq_of_parse_and_map(np, 0)) == NO_IRQ) {
		printk(KERN_ERR "%s:%s() irq_of_parse_and_map failed\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	if (of_address_to_resource(np, 0, &res)) {
		printk(KERN_ERR "%s:%s() Unable to get FPGA address\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	/* map everything except SD. */
	if ((fpga = ioremap(res.start, 0x2200)) == NULL) {
		printk(KERN_ERR "%s:%s() Unable to map FPGA\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}	

   	of_node_put(np);

        pdx->info.irql = irq;
        pdx->bar0 = fpga;

	/* initialize the procfs interface */
	taco_proc_init(pdx);

	/* you need a device for DMA or nothing works with 2.6.31 */
	pdx->dev = platform_device_register_simple("warp-dev", 0, NULL, 0);
	pdx->dev->dev.coherent_dma_mask = ~0ULL;

	/* initialize the dma library 
	 * note that this should be done only once per boot 
	 */

	if ((rc = dma_init_module(pdx))) {
		remove_proc_entry("driver/taco", NULL);
		goto error_cleanup;
	}

        /* only reset the silabs once. */
        if (daytona_silabs_reset(pdx)) {
		remove_proc_entry("driver/taco", NULL);
                printk(KERN_ERR "Unable to reset silabs\n");
                return -ENODEV;
        }

	return 0;

error_cleanup:

	if (pdx->bar0)
		iounmap(pdx->bar0);

	kfree(pdx);

	if (np)
		of_node_put(np);

	return -ENOENT;


}

module_init(shared_dma_init_module);

/* todo : merge it with dma_exit_module() sometime */
void __exit shared_dma_exit_module(void)
{
	dma_exit_module();
	remove_proc_entry("driver/taco", NULL);
	if (pdx->bar0)
		iounmap(pdx->bar0);
	platform_device_unregister(pdx->dev);
	kfree(pdx);
}

module_exit(shared_dma_exit_module);
