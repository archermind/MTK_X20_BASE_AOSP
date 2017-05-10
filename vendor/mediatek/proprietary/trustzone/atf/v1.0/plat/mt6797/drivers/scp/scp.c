#include <arch.h>
#include <arch_helpers.h>
#include <sip_error.h>
#include <platform.h>
#include <mmio.h>
#include "plat_private.h"
#include "scp.h"

uint64_t sip_reset_scp(unsigned int reset)
{
	uint32_t reg;

	reg = mmio_read_32(SCP_RESET_REG);

	if (reset) {
		if ((reg & SCP_RESET_EN) == 0) {
			return SIP_SVC_E_PERMISSION_DENY;
		}

		reg &= ~SCP_RESET_EN;
		mmio_write_32(SCP_RESET_REG, reg);
		dsb();
	}

	reg |= SCP_RESET_EN;
	mmio_write_32(SCP_RESET_REG, reg);
	dsb();

	return SIP_SVC_E_SUCCESS;
}

