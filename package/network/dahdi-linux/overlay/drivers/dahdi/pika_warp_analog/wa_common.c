#include <linux/module.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <asm/time.h>
#include <asm/io.h>

#include <dahdi/kernel.h>

#include "driver.h"
#include "daytona.h"
#include "wa_parms.h"
#include "wa_devext.h"
#include "wa_dma.h"

int taco_audio_init(PDEVICE_EXTENSION pdx);
int taco_audio_fini(PDEVICE_EXTENSION pdx);
int taco_pft_init(PDEVICE_EXTENSION pdx);

/* dma initialization function */
int warp_dma_init(PDEVICE_EXTENSION pdx)
{
	pikadma_get_buffers((void **)&pdx->rx_buf, (void **)&pdx->tx_buf);
	return 0;
}

/* dma finalization/teardown function */
void warp_dma_fini(PDEVICE_EXTENSION pdx) 
{

}

/* dma enable control function -- TODO : call the function directly */
int warp_dma_enable(void (*dma_handler)(int))
{
	return (warp_dma_register(dma_handler));
}

/* dma disable control function -- TODO : call the function directly */
void warp_dma_disable(void (*dma_handler)(int))
{
	warp_dma_unregister();
}

/* function to enable audio on the warp */
int warp_audio_enable(PDEVICE_EXTENSION pdx)
{
	return (taco_audio_init(pdx));
}

/* function to disable audio on the warp */
int warp_audio_disable(PDEVICE_EXTENSION pdx)
{
	return (taco_audio_fini(pdx));
}

/* This is *only* called once at module init time! */
int warp_pft_init(PDEVICE_EXTENSION pdx)
{
	return taco_pft_init(pdx);
}

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
