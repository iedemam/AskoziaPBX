#ifndef __WARP_DEVEXT_H__
#define __WARP_DEVEXT_H__

#if 0
typedef struct {
  unsigned int line_voltage;
  unsigned int loop_current;
} trunk_info_t;

#define MAX_NUM_CHAN		9 /* 4 + 4 + 1 = 9 for FXS, 4 + 4 = 8 for FXO */
#define MAX_MODULE_COUNT	2
#endif

/* add the number of channels to the struct later on */
struct warp_analog {
   int module_type;
   int module_port;
   int chan_timeslot[MAX_NUM_CHAN];
   struct dahdi_span span;
   struct dahdi_chan _chans[MAX_NUM_CHAN]; 	
   struct dahdi_chan *chans[MAX_NUM_CHAN];
   int fxs_reversepolarity[MAX_NUM_CHAN];
   struct dahdi_vmwi_info fxs_vmwisetting[MAX_NUM_CHAN];
   int idletxhookstate[MAX_NUM_CHAN];
   int lasttxhook[MAX_NUM_CHAN];
   struct delay fxs_oht_delay[MAX_NUM_CHAN];
   int fxs_vmwi_active_msgs[MAX_NUM_CHAN];
   int fxs_vmwi_lrev[MAX_NUM_CHAN];
   int fxs_vmwi_hvdc[MAX_NUM_CHAN];
   int fxs_vmwi_hvac[MAX_NUM_CHAN];
   int pos;
   int removed;
   int usecount;
   unsigned chan_mask;
   const char *module_name;
   int num_chans;
   PDEVICE_EXTENSION devext;
}; 

#endif /* __WARP_DEVEXT_H__ */

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
