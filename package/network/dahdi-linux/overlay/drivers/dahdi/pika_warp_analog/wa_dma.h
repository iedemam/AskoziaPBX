#ifndef __WA_DMA_H__
#define __WA_DMA_H__

struct dma_ctx {
	struct list_head list;

	void __iomem *sgl;
	void __iomem *fpga;

	u8 *dma_buf;
	u8 *rx_buf, *tx_buf;
	dma_addr_t dma_handle;
	int dma_size;

	int irq;
};


struct dma_inst {
	struct list_head list;
	int cardid;
	void (*dma_handler)(int);
};

#define MODNAME 		"pikadma"
#define DMA_INTS		0x000C0000

#define FRAMES_PER_BUFFER	320 /* 40ms / (125us per frame) */
#define FRAMES_PER_TRANSFER	8
#define FRAMESIZE		64
#define JITTER			2

#define DAHDI_FXSO_CARD_ID	98

/* dma handler registration/removal functions */

//int warp_dma_register(dma_isr_func dma_func);
//int warp_dma_unregister(dma_isr_func dma_func);

int warp_dma_register(void (*cb)(int cardid));
int warp_dma_unregister(void);

/* helper functions */

void pikadma_get_buffers(void **rx_buf, void **tx_buf);
void pikadma_disable(void);
void pikadma_enable(void);

/* system wide initialization/deinitialization functions */

int dma_init_module(PDEVICE_EXTENSION pdx);
void dma_exit_module(void);

#endif /* __WA_DMA_H__ */
