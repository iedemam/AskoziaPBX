#ifndef __WARP_COMMON_H__
#define __WARP_COMMON_H__

#include <dahdi/kernel.h>	/* dahdi definitions */

#define BAR0_READ(pdx, reg) \
        in_be32((unsigned __iomem *)(pdx->bar0 + reg))
#define BAR0_WRITE(pdx, reg, value) \
        out_be32((unsigned __iomem *)(pdx->bar0 + reg), value)

#define msecs_to_tics(ms)	(ms)

int read_silabs(PDEVICE_EXTENSION pdx, unsigned trunk, unsigned reg, unsigned char *out);
int write_silabs(PDEVICE_EXTENSION pdx, unsigned trunk, unsigned reg, unsigned data);
void write_silabs_indirect(void __iomem *fpga, unsigned char channel, unsigned char ind_reg_offset, unsigned short data);

/* borrowed functions from daytona */
int daytona_phone_set_lf(PDEVICE_EXTENSION pdx, int phone, int direction);

/* fxs configuration functions */
int daytona_configure_fxs(PDEVICE_EXTENSION pdx, unsigned mask);

int daytona_phone_config(PDEVICE_EXTENSION pdx, u32 phone,
                         unsigned int intCtrl, unsigned int audioFormat);
int daytona_phone_start(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_set_onhook_tx(PDEVICE_EXTENSION pdx, phone_onhook_tx_t *rqst);
int daytona_phone_reversal(PDEVICE_EXTENSION pdx, int phone);


/* fxo configuration functions */
int daytona_configure_fxo(PDEVICE_EXTENSION pdx, unsigned mask);

int daytona_set_detect(PDEVICE_EXTENSION pdx, unsigned int trunk, unsigned int mask);
int daytona_trunk_set_config(PDEVICE_EXTENSION pdx, trunk_start_t *rqst);
int daytona_trunk_start(PDEVICE_EXTENSION pdx, unsigned int trunk);
int daytona_trunk_stop(PDEVICE_EXTENSION pdx, unsigned int trunk);
int daytona_hook_switch(PDEVICE_EXTENSION pdx, trunk_hookswitch_t *rqst);
int daytona_trunk_dial(PDEVICE_EXTENSION pdx, trunk_dial_t *rqst);
int daytona_trunk_set_audio(PDEVICE_EXTENSION pdx, trunk_audio_t *audio);
int daytona_set_threshold(PDEVICE_EXTENSION pdx, unsigned int trunk,
			  unsigned int threshold);

/* general configuration functions -- other stuff */
int daytona_silabs_reset(PDEVICE_EXTENSION pdx);

/* function to add an event */
int add_event(PDEVICE_EXTENSION pdx, unsigned type, unsigned line, unsigned mask);

irqreturn_t dahdi_warp_isr(int irq, void *context);

void imr_set(PDEVICE_EXTENSION pdx, unsigned set);
void imr_clr(PDEVICE_EXTENSION pdx, unsigned clr);

/* tasklets to handle work */
void phone_tasklet(unsigned long arg);
void ring_tasklet(unsigned long arg);
void line_tasklet(unsigned long arg);

/* other functions */
void do_phones(PDEVICE_EXTENSION pdx, unsigned iccr);
void do_trunks(PDEVICE_EXTENSION pdx, unsigned iccr);
void do_timers(PDEVICE_EXTENSION pdx);

/* dma init functions */
int warp_dma_init(PDEVICE_EXTENSION pdx);
void warp_dma_fini(PDEVICE_EXTENSION pdx);
int warp_dma_enable(void (*dma_handler)(int));
void warp_dma_disable(void (*dma_handler)(int));

/* audio control functions */
int warp_audio_enable(PDEVICE_EXTENSION pdx);
int warp_audio_disable(PDEVICE_EXTENSION pdx);

/* other events -- ring generator start/stop */
int daytona_phone_ringgen_start(PDEVICE_EXTENSION pdx, phone_ring_t *rqst);
int daytona_phone_ringgen_stop(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_stop(PDEVICE_EXTENSION pdx, u32 phone);

/* dahdi driver interface */
int dahdi_warp_analog_send_event_up(int warp_chan_num, unsigned int warp_event_code);
void dahdi_warp_analog_transmitprep(unsigned char *tx_addr);
void dahdi_warp_analog_receiveprep(unsigned char *rx_addr);

/* dahdi phone linefeed setup function */
int daytona_phone_set_linefeed(PDEVICE_EXTENSION pdx, int phone, int linefeed);
int daytona_phone_get_linefeed(PDEVICE_EXTENSION pdx, int phone, int *linefeed);

/* inline functions for low level access */
static inline unsigned fpga_read(void __iomem *fpga, int reg)
{
	return in_be32(fpga + reg);
}

static inline void fpga_write(void __iomem *fpga, int reg, unsigned value)
{
	out_be32(fpga + reg, value);
}

#endif /* __WARP_COMMON_H__ */

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
