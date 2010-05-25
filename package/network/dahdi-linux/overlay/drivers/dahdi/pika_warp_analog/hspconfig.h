#ifndef HSPCONFIG_H
#define HSPCONFIG_H

//#include "pk_basetypes.h"
#include "pkh_board.h"

#define HSP_FRAMESPERTRANSFER  8
#define HSP_FRAMESPERINTERRUPT 8

typedef struct _HSPCFG_DATA {
  u32  FramesPerTransfer;
  u32  FramesPerInterrupt;
  u32  FramesPerBuffer;
  u32  FrameSize;
  u32  HardwareLatencyInFrames;
} HSPCFG_DATA, *PHSPCFG_DATA;

/* Clock sync register defines. Must be non-zero. */
#define CLOCK_SYNC_TYPE_DAYTONA  1

#endif /* HSPCONFIG_H */
