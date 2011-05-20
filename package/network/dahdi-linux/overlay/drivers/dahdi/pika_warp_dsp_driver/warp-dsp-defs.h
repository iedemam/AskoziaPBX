#ifndef _WARP_DSP_HWDEFS_H_
#define _WARP_DSP_HWDEFS_H_

/*-----------------------------------------------------------------------------
 | External Bus Controller
 +----------------------------------------------------------------------------*/
#define EBC_DCR_BASE 			0x12
#define EBC0_CFGADDR 			(EBC_DCR_BASE+0x0)   /* External bus controller addr reg     */
#define EBC0_CFGDATA 			(EBC_DCR_BASE+0x1)   /* External bus controller data reg     */

/* values for EBC0_CFGADDR register - indirect addressing of these regs */
#define pb0cr				0x00    /* periph bank 0 config reg             */
#define pb1cr				0x01    /* periph bank 1 config reg             */
#define pb2cr				0x02    /* periph bank 2 config reg             */
#define pb3cr				0x03    /* periph bank 3 config reg             */
#define pb4cr				0x04    /* periph bank 4 config reg             */
#define pb5cr				0x05    /* periph bank 5 config reg             */
#define pb6cr				0x06    /* periph bank 6 config reg             */
#define pb7cr				0x07    /* periph bank 7 config reg             */
#define pb0ap				0x10    /* periph bank 0 access parameters      */
#define pb1ap				0x11    /* periph bank 1 access parameters      */
#define pb2ap				0x12    /* periph bank 2 access parameters      */
#define pb3ap				0x13    /* periph bank 3 access parameters      */
#define pb4ap				0x14    /* periph bank 4 access parameters      */
#define pb5ap				0x15    /* periph bank 5 access parameters      */
#define pb6ap				0x16    /* periph bank 6 access parameters      */
#define pb7ap				0x17    /* periph bank 7 access parameters      */
#define pbear				0x20    /* periph bus error addr reg            */
#define pbesr				0x21    /* periph bus error status reg          */
#define EBC0_CFG_REG_ADDR       	0x23    /* external bus configuration reg       */
#define xbcid           		0x24    /* external bus core id reg             */

#define stringify(s)    		tostring(s)
#define tostring(s)     		#s

#define mtdcr(rn, v)			asm volatile("mtdcr " stringify(rn) ",%0" : : "r" (v))
#define mtebc(reg, data)		do { mtdcr(EBC0_CFGADDR,reg);mtdcr(EBC0_CFGDATA,data); } while (0)
#define mfebc(reg, data)		do { mtdcr(EBC0_CFGADDR,reg);data = mfdcr(EBC0_CFGDATA); } while (0)

#define FPGA_RESET			0x14
#define FPGA_DSP_RESET_BIT		0x00001000
#define DSP_HCNTL0_SELECT_OFFSET	0x00200000
#define HPIC_RESET_MASK			0x0001
#define CONFIG_DSP_TIMING_MASK		0x0200
#define FPGA_CONFIG_REG_OFFSET		0x00000000
#define FPGA_REV_OFFSET          	0x0000001C
#define CFG_EBC_PB3AP_FAST    		0x03000500  //timing for after DSP clock generator is programmed
#define CFG_EBC_PB3AP         		0x05000D00  //timing for before DSP clock generator is programmed

#endif /* _WARP_DSP_HWDEFS_H_ */
