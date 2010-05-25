#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#include <dahdi/kernel.h>

#include "driver.h"
#include "daytona.h"
#include "wa_parms.h"
#include "wa_devext.h"
#include "wa_dma.h"
#include "wa_common.h"

/* The following are exported from tacokmd */
int pikadma_register_cb(int cardid, void (*cb)(int cardid));
int pikadma_unregister(int cardid);

/* register our dma handler function */
int warp_dma_register(void (*cb)(int cardid))
{
	return pikadma_register_cb(DAHDI_FXSO_CARD_ID, cb);
}

/* unregister a given dma handler */
int warp_dma_unregister(void)
{
	return pikadma_unregister(DAHDI_FXSO_CARD_ID);
}
