#include "driver.h"

/* Generic version.
 *
 * It is not worth optimizing for the Warp case since this is only
 * called from silabs_reset for warped boxes. See fpga_wait_for_ciar.
 *
 * We do not use KeQueryPerformanceCounter on windows. From the docs:
 *     It is not intended for measuring elapsed time, for computing
 *     stalls or waits, or for iterations.
 */
int wait_for_ciar(PDEVICE_EXTENSION pdx)
{
#ifdef CONFIG_X86_TSC
	/* Roughly 1ms */
	unsigned long long start = get_cycles();

	while ((BAR0_READ(pdx, BAR0_STAT_LINES) & 0x1000) == 0) {
		if ((get_cycles() - start) > cpu_khz) {
			PK_PRINT("ERROR: wait_for_ciar timeout\n");
			return 1;
		}
		cpu_relax();
	}

	return 0;
#else
	int count = 0; /* 35 seems to be max count */

	while ((BAR0_READ(pdx, BAR0_STAT_LINES) & 0x1000) == 0) {
		if (++count > 1000) {
			PK_PRINT("ERROR: wait_for_ciar timeout\n");
			return 1;
		}
		ndelay(100);
	}

	return 0;
#endif
}
EXPORT_SYMBOL(wait_for_ciar);

#ifdef CONFIG_WARP
#include <asm/time.h> /* for get_tbl and friends */

/* This must be a spinlock since the fpga_*_indirect functions are
 * called from the brie isr.
 */
static DEFINE_SPINLOCK(_fpga_lock);


static inline unsigned fpga_read(void __iomem *fpga, int reg)
{
	return in_be32(fpga + reg);
}

static inline void fpga_write(void __iomem *fpga, int reg, unsigned value)
{
	out_be32(fpga + reg, value);
}

static int fpga_wait_for_ciar(void __iomem *fpga)
{
	unsigned long start, loops;

	/* This is the normal case */
	if (fpga_read(fpga, BAR0_STAT_LINES) & 0x1000)
		return 0;

	start = get_tbl();
	loops = tb_ticks_per_usec << 9; /* 512us */

	/* Max about 25us */
	while ((fpga_read(fpga, BAR0_STAT_LINES) & 0x1000) == 0) {
		if (tb_ticks_since(start) > loops) {
			PK_PRINT("ERROR: wait_for_ciar timeout\n");
			return 1;
		}
		cpu_relax();
	}

	return 0;
}

/* Only call this if you are holding the fpga lock */
static int fpga_read_indirect_locked(void __iomem *fpga, int channel,
			      int reg, __u8 *value)
{
	int val = (channel << 16) | (reg << 8) | (1 << 22);

	if (fpga_wait_for_ciar(fpga))
		return -ETIMEDOUT;
	fpga_write(fpga, BAR0_CIAR, val);
	if (fpga_wait_for_ciar(fpga))
		return -ETIMEDOUT;
	*value = fpga_read(fpga, BAR0_CIAR);
	return 0;
}

int fpga_read_indirect(void __iomem *fpga, int channel, int reg, __u8 *value)
{
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&_fpga_lock, flags);

	rc = fpga_read_indirect_locked(fpga, channel, reg, value);

	spin_unlock_irqrestore(&_fpga_lock, flags);

	return rc;
}
EXPORT_SYMBOL(fpga_read_indirect);

/* Only call this if you are holding the fpga lock */
static int fpga_write_indirect_locked(void __iomem *fpga, int channel,
				      int reg, __u8 value)
{
	int val = (channel << 16) | (reg << 8) | value;

	if (fpga_wait_for_ciar(fpga))
		return -ETIMEDOUT;
	out_be32(fpga + BAR0_CIAR, val);
	return 0;
}

int fpga_write_indirect(void __iomem *fpga, int channel, int reg, __u8 value)
{
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&_fpga_lock, flags);

	rc = fpga_write_indirect_locked(fpga, channel, reg, value);

	spin_unlock_irqrestore(&_fpga_lock, flags);

	return rc;
}
EXPORT_SYMBOL(fpga_write_indirect);

int read_silabs(PDEVICE_EXTENSION pdx, unsigned trunk, unsigned reg, u8 *out)
{
	return fpga_read_indirect(pdx->bar0, trunk, reg, out);
}
EXPORT_SYMBOL(read_silabs);

int write_silabs(PDEVICE_EXTENSION pdx, unsigned trunk,
		 unsigned reg, unsigned data)
{
	return fpga_write_indirect(pdx->bar0, trunk, reg, data);
}
EXPORT_SYMBOL(write_silabs);

void write_silabs_indirect(void __iomem *fpga, u8 channel,
			   u8 ind_reg_offset , u16 data)
{
	u8 val;
	unsigned long flags;
	int count = 0;

	/* We must guarantee no FPGA indirect access */
	spin_lock_irqsave(&_fpga_lock, flags);

	do {
		fpga_read_indirect_locked(fpga, channel, I_STATUS, &val);
		if (++count == 100) /* Count usually <= 6 */
			break;
	} while (val);

	fpga_write_indirect_locked(fpga, channel,
				   I_DATA_LOW,  (u8)(data & 0x00FF));
	fpga_write_indirect_locked(fpga, channel,
				   I_DATA_HIGH, (u8)(data >> 8));
	fpga_write_indirect_locked(fpga, channel,
				   I_ADDRESS,   ind_reg_offset);

	spin_unlock_irqrestore(&_fpga_lock, flags);
}
EXPORT_SYMBOL(write_silabs_indirect);

void fpga_lock(unsigned long *flags)
{
	spin_lock_irqsave(&_fpga_lock, *flags);
}
EXPORT_SYMBOL(fpga_lock);

void fpga_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&_fpga_lock, *flags);
}
EXPORT_SYMBOL(fpga_unlock);

#else

// #include "flashstructure.h"

int read_silabs(PDEVICE_EXTENSION pdx, unsigned trunk, unsigned reg, u8 *out)
{
	u32 val = (trunk << 16) | (reg << 8) | (1 << 22);
	unsigned data;

	if (wait_for_ciar(pdx))
		return -ETIMEDOUT;
	BAR0_WRITE(pdx, BAR0_CIAR, val);
	if (wait_for_ciar(pdx))
		return -ETIMEDOUT;
	data = BAR0_READ(pdx, BAR0_CIAR);
	*out = data;
	return 0;
}


int write_silabs(PDEVICE_EXTENSION pdx, unsigned trunk,
		 unsigned reg, unsigned data)
{
	u32 val = (trunk << 16) | (reg << 8) | data;

	if (wait_for_ciar(pdx))
		return -ETIMEDOUT;
	BAR0_WRITE(pdx, BAR0_CIAR, val);

	return 0;
}

static inline int wait_for_fiar(PDEVICE_EXTENSION pdx)
{
	int count = 0;

	/* Wait for flash ready.  We need to wait up to 5ms because the
	 * flash requires up to 5ms between page writes.
	 */
	while ((BAR0_READ(pdx, BAR0_STAT_LINES) & 0x2000) == 0) {
		if (++count > 100000) {
			PK_PRINT("ERROR: wait_for_fiar timeout\n");
			return 1;
		}
		ndelay(100);
	}

	return 0;
}


int read_flash(PDEVICE_EXTENSION pdx, u8 *flash)
{
	int i;

	if (wait_for_fiar(pdx))
		return 1; /* failed */

	/* Bank 1 read */
	BAR0_WRITE(pdx, BAR0_FIAR, 0x00410000);

	for (i = 0; i < sizeof(FLASH_FIELDS_STRUCT); ++i, ++flash) {
		if (wait_for_fiar(pdx))
			return 1; /* failed */

		*flash = BAR0_READ_BYTE(pdx, BAR0_FIAR);
	}

	return 0;
}

#endif
