/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __GSM_H
#define __GSM_H


/* SC16IS752 UART registers */
#define	IS752_RHR		0x00 /* read */
#define	IS752_THR		0x00 /* write */
#define	IS752_IER		0x01
#define	IS752_FCR		0x02 /* write */
#define	IS752_IIR		0x02 /* read */
#define	IS752_LCR		0x03
#define	IS752_MCR		0x04
#define	IS752_LSR		0x05
#define	IS752_MSR		0x06
#define	IS752_SPR		0x07
#define	IS752_TCR		0x06 /* only when EFR[4]=1 and MCR[2]=1 */
#define	IS752_TLR		0x07 /* only when EFR[4]=1 and MCR[2]=1 */
#define	IS752_TXLVL		0x08
#define	IS752_RXLVL		0x09
#define	IS752_IODir		0x0A
#define	IS752_IOState		0x0B
#define	IS752_IOIntEna		0x0C
#define	IS752_IOControl		0x0E
#define	IS752_EFCR		0x0F
#define	IS752_DLL		0x00 /* only when LCR[7]=1 and LCR!=0xBF */
#define	IS752_DLH		0x01 /* only when LCR[7]=1 and LCR!=0xBF */
#define	IS752_EFR		0x02 /* only when LCR=0xBF */
#define	IS752_XON1		0x04 /* only when LCR=0xBF */
#define	IS752_XON2		0x05 /* only when LCR=0xBF */
#define	IS752_XOFF1		0x06 /* only when LCR=0xBF */
#define	IS752_XOFF2		0x07 /* only when LCR=0xBF */

/* Audio port related constants */
#define AUDIO_CH0_RX_GAIN	0
#define AUDIO_CH1_RX_GAIN	1
#define AUDIO_CH0_TX_GAIN	2
#define AUDIO_CH1_TX_GAIN	3
#define AUDIO_POWER_DOWN	4
#define AUDIO_PCM_CONTROL	5

#endif	/* __GSM_H */
