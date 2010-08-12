#include <linux/io.h>

/* SAM This should go somewhere else.... */

int pikadma_register_cb(int cardid, void (*cb)(int cardid));
int pikadma_unregister(int cardid);
void pikadma_enable(void);
void pikadma_disable(void);
void pikadma_get_buffers(void **rx_buf, void **tx_buf);

static inline unsigned fpga_read(void __iomem *fpga, int reg)
{
	return in_be32(fpga + reg);
}

static inline void fpga_write(void __iomem *fpga, int reg, unsigned value)
{
	out_be32(fpga + reg, value);
}

int fpga_read_indirect(void __iomem *fpga, int channel, int reg, __u8 *value);
int fpga_write_indirect(void __iomem *fpga, int channel, int reg, __u8 value);
void fpga_lock(unsigned long *flags);
void fpga_unlock(unsigned long *flags);
