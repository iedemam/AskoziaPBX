/*
 * Copyright (c) 2001-2010, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakCust.h
 *
 * Description:
 *   This file contains host system dependent definitions and prototypes of
 *   functions to support generic G.PAK API functions. The file is used when
 *   integrating G.PAK API functions in a specific host processor environment.
 *
 *   Note: This file may need to be modified by the G.PAK system integrator.
 *
 * Version: 1.0
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *
 */

#ifndef _GPAKCUST_H  /* prevent multiple inclusion */
#define _GPAKCUST_H


//#define HOST_LITTLE_ENDIAN         /* un-comment this if little endian host */
#define HOST_BIG_ENDIAN              /* un-comment this if big endian host */

#define PCI_PG_SIZE      0x100000     /* Page size for PCI transfers (must be power of 2) */
#define MAX_DSP_CORES         1       /* maximum number of DSP cores */
#define MAX_CHANNELS         42       /* maximum number of channels */
#define DOWNLOAD_BLOCK_I8   512	      /* download block size (bytes) */
//#define MAX_WAIT_LOOPS      500       /* max number of wait delay loops */
#define MAX_WAIT_LOOPS      5       	/* max number of wait delay loops */

#endif  /* prevent multiple inclusion */


