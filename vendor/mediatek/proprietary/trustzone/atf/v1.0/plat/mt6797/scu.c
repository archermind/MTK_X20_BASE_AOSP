#include <stdio.h>
#include <stdarg.h>
#include <arch.h>
#include <platform_def.h>
#include <mmio.h>
#include <assert.h>

void disable_scu(unsigned long mpidr) {
	uint32_t axi_config;

	//printf("disable_scu(0x%x)\n", mpidr);
	switch (mpidr & MPIDR_CLUSTER_MASK) {
		case 0x000:
			axi_config = MP0_AXI_CONFIG;
			break;
		case 0x100:
			axi_config = MP1_AXI_CONFIG;
			break;
		case 0x200:
			axi_config = MP2_AXI_CONFIG;
			break;
		default:
			printf("wrong mpidr\n");
			assert(0);
			
	}
	mmio_write_32(axi_config, mmio_read_32(axi_config) | ACINACTM);
}

void enable_scu(unsigned long mpidr) {
	uint32_t axi_config;

	//printf("enable_scu(0x%x)\n", mpidr);
	switch (mpidr & MPIDR_CLUSTER_MASK) {
		case 0x000:
			axi_config = MP0_AXI_CONFIG;
			break;
		case 0x100:
			axi_config = MP1_AXI_CONFIG;
			break;
		case 0x200:
			axi_config = MP2_AXI_CONFIG;
			break;
		default:
			printf("wrong mpidr\n");
			assert(0);
			
	}
	mmio_write_32(axi_config, mmio_read_32(axi_config) & ~ACINACTM);
}
