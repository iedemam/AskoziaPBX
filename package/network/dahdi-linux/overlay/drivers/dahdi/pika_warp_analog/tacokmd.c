#include "pikacore.h"
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/of_i2c.h>
#include <linux/kthread.h>

#include "driver.h"
#include "pksystemver.h"


int debug;
module_param(debug, int, 0664);


/* We need a board list for the clocking code. */
LIST_HEAD(board_list);

static int taco_pft_init(PDEVICE_EXTENSION pdx);
static void taco_print_rev(PDEVICE_EXTENSION pdx);
int taco_proc_init(PDEVICE_EXTENSION pdx);


/* We must be very careful with the PFT register since the BRI module
 * reuses the bits for something else :(
 */
static inline void reset_relays(PDEVICE_EXTENSION pdx)
{
	unsigned pft = BAR0_READ(pdx, BAR0_TACO_PFT);
	pft &= ~pdx->pft_mask;
	BAR0_WRITE(pdx, BAR0_TACO_PFT, pft);
}


static void daytona_shutdown(void *arg)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)arg;

	daytona_reset(pdx);

	reset_relays(pdx);
}


#ifndef CONFIG_WARP_DAHDI
int __init taco_init_module(void)
{
	PDEVICE_EXTENSION pdx;
	int rc = -EINVAL;
	struct device_node *np;
	struct resource res;
	void __iomem *fpga;
	int irq;

	/* Create the private data */
	pdx = kmalloc(sizeof(DEVICE_EXTENSION), GFP_KERNEL);
	if (pdx == NULL) {
		printk(KERN_ERR __FILE__ " NO MEMORY\n");
		return -ENOMEM;
	}
	memset(pdx, 0, sizeof(DEVICE_EXTENSION));

	INIT_LIST_HEAD(&pdx->list);

	atomic_set(&pdx->open_count, 0);

	init_waitqueue_head(&pdx->read_queue);

	tasklet_init(&pdx->phone_bh, phone_tasklet, (unsigned long)pdx);
	tasklet_init(&pdx->ring_bh, ring_tasklet, (unsigned long)pdx);
	tasklet_init(&pdx->line_bh, line_tasklet, (unsigned long)pdx);

	spin_lock_init(&pdx->stat_lock);

	np = of_find_compatible_node(NULL, NULL, "pika,fpga");
	if (np == NULL) {
		printk(KERN_ERR __FILE__ ": Unable to find fpga\n");
		goto of_err;
	}

	irq = irq_of_parse_and_map(np, 0);
	if (irq  == NO_IRQ) {
		printk(KERN_ERR __FILE__ ": irq_of_parse_and_map failed\n");
		goto of_err;
	}

	if (of_address_to_resource(np, 0, &res)) {
		printk(KERN_ERR __FILE__ ": Unable to get FPGA address\n");
		goto of_err;
	}

	/* The line driver needs everything except SD. */
	fpga = ioremap(res.start, 0x2200);
	if (fpga == NULL) {
		printk(KERN_ERR __FILE__ ": Unable to map FPGA\n");
		goto of_err;
	}

	of_node_put(np);

	pdx->info.irql = irq;
	pdx->bar0 = fpga;

	taco_print_rev(pdx);

	rc = dma_init_module(pdx);
	if (rc)
		goto error_cleanup;

	rc = HmpCard_create(pdx);
	if (rc) {
		printk(KERN_ERR __FILE__ ": Error Creating HMP Card\n");
		goto error_cleanup;
	}

	rc = warp_read_serial(&pdx->info.eeprom.serialNumber);
	if (rc) {
		printk(KERN_ERR __FILE__ ": Error Reading serial number\n");
		goto error_cleanup;
	}

	rc = pikacore_register_board(pdx->info.eeprom.serialNumber,
				     &daytona_fops);
	if (rc < 0) {
		printk(KERN_ERR __FILE__ ": pikacore register failed!\n");
		goto error_cleanup;
	}

	pdx->id = rc;

	pdx->info.irql = irq;

	imr_clr(pdx, ~IMR_MASK); /* disable ints */
	rc = request_irq(irq, daytona_ISR, IRQF_SHARED, "pikataco", pdx);
	if (rc) {
		printk(KERN_ERR __FILE__ ": Unable to request irq %d\n", rc);
		goto register_cleanup;
	}

	list_add(&pdx->list, &board_list);

	taco_pft_init(pdx);

	pika_dtm_register_shutdown(daytona_shutdown, pdx);

	dmanl_init();

	taco_proc_init(pdx);

	return 0;

register_cleanup:
	pikacore_unregister(pdx->id);

error_cleanup:
	HmpCard_destroy(pdx);

	dma_exit_module();

	if (pdx->bar0)
		iounmap(pdx->bar0);

	kfree(pdx);

	return rc;

of_err:
	kfree(pdx);
	if (pdx->bar0)
		iounmap(pdx->bar0);
	if (np)
		of_node_put(np);
	return -ENOENT;
}
module_init(taco_init_module);
#endif

#ifndef CONFIG_WARP_DAHDI
void __exit taco_exit_module(void)
{
	PDEVICE_EXTENSION pdx;

	dma_exit_module();

	if (list_empty(&board_list)) {
		printk(KERN_ERR __FILE__ ": Board list empty\n");
		return;
	}

	pdx = list_entry(board_list.next, DEVICE_EXTENSION, list);

	pika_dtm_unregister_shutdown(daytona_shutdown, pdx);

	daytona_reset(pdx);

	list_del(&pdx->list);

	HmpCard_destroy(pdx);

	free_irq(pdx->info.irql, pdx);

	pikacore_unregister(pdx->id);

	if (pdx->pft)
		kthread_stop(pdx->pft);

	reset_relays(pdx);

	iounmap(pdx->bar0);

	kfree(pdx);

	dmanl_exit();

	remove_proc_entry("driver/taco", NULL);
}

module_exit(taco_exit_module);
#endif


/* -------------------------------------------------------------- */

static char *taco_mod_type_to_str(unsigned type)
{
	switch (type & 0xf) {
	case 0xf:
		return "empty";
	case 1:
		return "FXO";
	case 2:
		return "FXS";
	case 3:
		return "BRI";
	case 4:
		return "GSM";
	default:
		return "???";
	}
}

static void taco_print_rev(PDEVICE_EXTENSION pdx)
{
	unsigned rev = BAR0_READ(pdx, BAR0_FPGA_REV);

	printk(KERN_INFO "taco: FPGA %08X\n", rev);
	printk(KERN_INFO "Modules A: %s B: %s\n",
	       taco_mod_type_to_str(rev >> 16),
	       taco_mod_type_to_str(rev >> 20));
}

static int taco_read_proc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	PDEVICE_EXTENSION pdx = data;
	unsigned rev = BAR0_READ(pdx, BAR0_FPGA_REV);

	*eof = 1;
	return sprintf(page, "FPGA %08X  %s/%s\n", rev,
		       taco_mod_type_to_str(rev >> 16),
		       taco_mod_type_to_str(rev >> 20));
}

int taco_proc_init(PDEVICE_EXTENSION pdx)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry("driver/taco", 0, NULL);
	if (!entry)
		return -ENOMEM;

	entry->read_proc = taco_read_proc;
	entry->data = pdx;

	return 0;
}

int taco_probe(PDEVICE_EXTENSION pdx)
{
	unsigned rev = BAR0_READ(pdx, BAR0_FPGA_REV);
	unsigned moda, modb, mask;

	pdx->info.FPGA_version = rev & 0xffff; /* save the revision */

	moda = (rev >> 16) & 0xf;
	modb = (rev >> 20) & 0xf;

	/* We must be very careful with the PFT register since the BRI
	 * module uses the same bits :(
	 */
	if (moda == 1 || moda == 2)
		pdx->pft_mask |= 1;
	if (modb == 1 || modb == 2)
		pdx->pft_mask |= 2;

	/* It works best to configure all FXO first, then FXS. */
	mask = 0;
	if (moda == 1)
		mask |= 0x3c;
	if (modb == 1)
		mask |= 0x3c0;
	if (daytona_configure_fxo(pdx, mask))
		return -EINVAL;

	mask = 2; /* always have onboard FXS */
	if (moda == 2)
		mask |= 0x3c;
	if (modb == 2)
		mask |= 0x3c0;
	if (daytona_configure_fxs(pdx, mask))
		return -EINVAL;

	return 0;
}


int taco_audio_init(PDEVICE_EXTENSION pdx)
{
	unsigned reg;

	/* Make sure FS and BCLK off. */
	reg = BAR0_READ(pdx, BAR0_CONFIG);
	reg &= ~1;
	BAR0_WRITE(pdx, BAR0_CONFIG, reg);

	/* Wait 200ms. */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(200));

	/* Initialize all registers to default values. This should be done
	 * again after a lightning strike.
	 */
	write_silabs(pdx, 0,  0, 6);
	write_silabs(pdx, 0,  2, 6);
	write_silabs(pdx, 0,  4, 6);
	write_silabs(pdx, 0,  6, 6);
	write_silabs(pdx, 0,  8, 4);
	write_silabs(pdx, 0, 10, 4);

	/* Supply FS and BCLK */
	reg = BAR0_READ(pdx, BAR0_CONFIG);
	reg |= 1;
	BAR0_WRITE(pdx, BAR0_CONFIG, reg);

	/* Wait 130 ms */
	delay(130);

	return 0;
}
#ifdef CONFIG_WARP_DAHDI
EXPORT_SYMBOL(taco_audio_init);
#endif


int taco_audio_fini(PDEVICE_EXTENSION pdx)
{
	unsigned reg;

	/* Full power down */
	write_silabs(pdx, 0, 8, 8 | 4);

	/* FS and BCLK off. */
	reg = BAR0_READ(pdx, BAR0_CONFIG);
	reg &= ~1;
	BAR0_WRITE(pdx, BAR0_CONFIG, reg);

	return 0;
}
#ifdef CONFIG_WARP_DAHDI
EXPORT_SYMBOL(taco_audio_fini);
#endif

/* WARNING: The audio chip takes the network point of view. So RX on
 * the chip means out and TX means in. Basically the meaning of RX and
 * TX are swapped.
 */
int taco_audio_gain(PDEVICE_EXTENSION pdx, audio_gain_t *rqst)
{
	u8 data, was;

	if (rqst->rx_gain < -18 || rqst->rx_gain > 6 ||
	    rqst->tx_gain < -18 || rqst->tx_gain > 6)
		return -EINVAL;

	rqst->rx_gain = abs(rqst->rx_gain - 6);
	rqst->tx_gain = abs(rqst->tx_gain - 6);

	write_silabs(pdx, 0, 0, rqst->tx_gain);
	write_silabs(pdx, 0, 4, rqst->rx_gain);

	read_silabs(pdx, 0, 8, &was);
	if (rqst->mute)
		data = was | 0x10;
	else
		data = was & ~0x10;
	if (data != was)
		write_silabs(pdx, 0, 8, data);

	read_silabs(pdx, 0, 10, &was);
	if (rqst->mode)
		data = was & ~4; /* alaw */
	else
		data = was | 4; /* mulaw */
	if (data != was)
		write_silabs(pdx, 0, 10, data);

	return 0;
}


static void clear_line(PDEVICE_EXTENSION pdx, int line)
{
	pdx->disabled_mask &= ~(1 << line);

	add_event(pdx, PFT_EVENT, line, 0);
}

#ifdef CONFIG_WARP_DAHDI
int taco_pft(void *arg)
#else
static int taco_pft(void *arg)
#endif
{
	PDEVICE_EXTENSION pdx = arg;
	int count_a = 0, count_b = 0;
	unsigned val;

	while (pdx->disabled_mask) {
		if (kthread_should_stop())
			return 0;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(100));

		val = BAR0_READ(pdx, BAR0_TACO_PFT);

		if (pdx->disabled_mask & (1 << 2)) { /* line 2 */
			if (val & 4) {
				if (++count_a > 20) {
					val |= 1;
					BAR0_WRITE(pdx, BAR0_TACO_PFT, val);
					clear_line(pdx, 2);
				}
			} else
				count_a = 0;
		}

		if (pdx->disabled_mask & (1 << 6)) { /* line 6 */
			if (val & 8) {
				if (++count_b > 20) {
					val |= 2;
					BAR0_WRITE(pdx, BAR0_TACO_PFT, val);
					clear_line(pdx, 6);
				}
			} else
				count_b = 0;
		}
	}

	pdx->pft = NULL;
	return 0;
}
#ifdef CONFIG_WARP_DAHDI
EXPORT_SYMBOL(taco_pft);
#endif

static unsigned i2c_read_serial(struct i2c_client *client, int offset)
{
	u8 str[16];
	int i;
	unsigned serial = 0;

	/* Serial number is 16 chars after 4 byte CRC. */
	str[0] = i2c_smbus_read_byte_data(client, 4);
	for (i = 0; i < offset; ++i)
		str[0] = i2c_smbus_read_byte(client);
	for (i = 1; i < 16; ++i)
		str[i] = i2c_smbus_read_byte(client);

	if (strncmp(str, "PIK-", 4))
		goto invalid_sn;

	/* The spec lies about the layout, so screw
	 * it, just look for digits. */
	for (i = 4; str[i] && i < 15; ++i)
		if (str[i] >= '0' && str[i] <= '9')
			serial = serial * 10 + str[i] - '0';
		else if (str[i] != '-' && str[i] != ' ')
			goto invalid_sn;

	return serial;

invalid_sn:
	printk(KERN_ERR "Invalid S/N:");
	for (i = 0; i < 16; ++i)
		printk(KERN_ERR " %02x", str[i]);
	printk(KERN_ERR "\n");
	return 0; /* invalid serial */
}

int warp_read_serial(unsigned *serial_out)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	struct device_node *np;
	struct i2c_client *client;
	unsigned serial;

	np = of_find_compatible_node(NULL, NULL, "at,24c04");
	if (np == NULL) {
		printk(KERN_ERR "Could not find seeprom node\n");
		return -ENOENT;
	}

	client = of_find_i2c_device_by_node(np);
	of_node_put(np);
	if (client == NULL) {
		printk(KERN_ERR "Could not find seeprom\n");
		return -ENOENT;
	}

	/* We must skip the first seeprom */
	serial = i2c_read_serial(client, 0x100);

	put_device(&client->dev);

	if (serial == 0)
		return -ENOENT;

	if (serial_out)
		*serial_out = serial;

	return 0;
#else
	struct i2c_adapter *adap;
	struct i2c_client *client;
	unsigned serial;

	adap = i2c_get_adapter(0);
	if (!adap) {
		printk(KERN_ERR "No I2C driver!\n");
		return -ENODEV;
	}

	list_for_each_entry(client, &adap->clients, list)
		if (client->addr == 0x53) {
			serial = i2c_read_serial(client, 0);

			i2c_put_adapter(adap);

			if (serial == 0)
				return -ENOENT;

			if (serial_out)
				*serial_out = serial;

			return 0;
		}

	i2c_put_adapter(adap);
	printk(KERN_ERR "Unable to find EEPROM\n");
	return -ENODEV;
#endif
}

#ifdef CONFIG_WARP_DAHDI
int taco_pft_init(PDEVICE_EXTENSION pdx)
#else
/* This is *only* called once at module init time! */
static int __init taco_pft_init(PDEVICE_EXTENSION pdx)
#endif
{
	struct task_struct *pft;
	unsigned rev = BAR0_READ(pdx, BAR0_FPGA_REV) >> 16;

	pdx->disabled_mask = 0;
	if ((rev & 0x0f) < 0x03)
		pdx->disabled_mask |= (1 << 2);
	if ((rev & 0xf0) < 0x30)
		pdx->disabled_mask |= (1 << 6);

	if (pdx->disabled_mask) {
		reset_relays(pdx); /* paranoia */

		pft = kthread_run(taco_pft, pdx, "pika_pft");
		if (IS_ERR(pft))
			return PTR_ERR(pft);

		pdx->pft = pft;
	}

	return 0;
}

#ifdef CONFIG_WARP_DAHDI
EXPORT_SYMBOL(taco_pft_init);
#endif

MODULE_DESCRIPTION("PIKA Warp Telephony Driver " PikaLoadVerString);
MODULE_AUTHOR("Sean MacLennan");
MODULE_LICENSE("GPL");

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
