#include <mmio.h>

#define readl(addr)		__asm__ volatile("dsb sy"); mmio_read_32((addr));
#define writel(addr, val)	mmio_write_32((addr), (val)); __asm__ volatile("dsb sy");

#define MCU_BIU_BASE		(0x10220000)
#define MISC1_CFG_BASE		(0x800)
#define DFD_CTRL		(MCU_BIU_BASE+MISC1_CFG_BASE+0x40)
#define DFS_PWR_ON		(MCU_BIU_BASE+MISC1_CFG_BASE+0x48)
#define DFD_CHAIN_LENGTH0	(MCU_BIU_BASE+MISC1_CFG_BASE+0x4c)
#define DFD_SHIFT_CLK_RATIO	(MCU_BIU_BASE+MISC1_CFG_BASE+0x50)
#define DFD_CHAIN_LENGTH1	(MCU_BIU_BASE+MISC1_CFG_BASE+0x5c)

void dfd_setup(void)
{
	writel(DFD_CTRL, 0x1);
	writel(DFD_CTRL, 0xb);
	writel(DFD_CHAIN_LENGTH0, 0x6ef2632e);
	writel(DFD_CHAIN_LENGTH1, 0x9d0d8471);
	writel(DFD_SHIFT_CLK_RATIO, 0x0);
	writel(DFS_PWR_ON, 0xb);
}

void dfd_disable(void)
{
	writel(DFD_CTRL, 0);
}
