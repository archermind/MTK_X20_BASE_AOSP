#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <interrupt_mgmt.h>
#include <platform.h>
#include <plat_config.h>
#include <stdint.h>
#include "plat_def.h"
#include "plat_private.h"
#include <platform_def.h>
#include <stdio.h>  //for printf
#include <emi_drv.h>
#include <sip_error.h>
#include <mmio.h>

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */

static unsigned char R2=0, R3=0, R4=0, R5=0, R6=0, R7=0, R8=0, R9=0, R10=0;
static unsigned char R11=0, R12=0, R13=0, R14=0, R15=0, R16=0, R17=0, R18=0, R19=0, R20=0;
static unsigned char R21=0, R22=0;
static unsigned int ATF_EMI_MPUI3_2ND=0, ATF_EMI_MPUJ3_2ND=0, ATF_EMI_MPUK3_2ND=0, ATF_EMI_MPUL3_2ND=0;

uint32_t sip_emimpu_set_region_protection(unsigned long long start, unsigned long long end, unsigned int region_permission)
{

	int region;
	unsigned int access_permission, locked;
	unsigned long long emi_physical_offset;

	region = (region_permission >> 27) & 0x1F;
	access_permission = region_permission & 0xFFFFFF;
	locked = (region_permission >> 26) & 0x1;

	if((end != 0) || (start !=0))
	{
		/* if not 4GB mode need offset 0x4000000 */
		if ((*(volatile unsigned int *)(0x10001f00) & 0x2000) == 0)
			emi_physical_offset = 0x40000000;
		else
			emi_physical_offset = 0;
		/* printf("atf emi_physical_offset=%llx\n",emi_physical_offset); */
		/*Address 64KB alignment*/
		start -= emi_physical_offset;
		end -= emi_physical_offset;
		start = (start >> 16) & 0xFFFF;
		end = (end >> 16) & 0xFFFF;

		if (end < start)
		{
			return SIP_SVC_E_INVALID_Range;
		}
	}

	switch (region) {
	case 2:
		if (R2 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R2 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK, 0);
		mmio_write_32(EMI_MPUC, (start << 16) | end);
		mmio_write_32(EMI_MPUK, access_permission);
		break;

	case 3:
		if (R3 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R3 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL, 0);
		mmio_write_32(EMI_MPUD, (start << 16) | end);
		mmio_write_32(EMI_MPUL, access_permission);
		break;

	case 4:
		if (R4 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R4 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUI_2ND, 0);
		mmio_write_32(EMI_MPUE, (start << 16) | end);
		mmio_write_32(EMI_MPUI_2ND, access_permission);
		break;

	case 5:
		if (R5 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R5 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUJ_2ND, 0);
		mmio_write_32(EMI_MPUF, (start << 16) | end);
		mmio_write_32(EMI_MPUJ_2ND, access_permission);
		break;

	case 6:
		if (R6 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R6 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK_2ND, 0);
		mmio_write_32(EMI_MPUG, (start << 16) | end);
		mmio_write_32(EMI_MPUK_2ND, access_permission);
		break;

	case 7:
		if (R7 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R7 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL_2ND, 0);
		mmio_write_32(EMI_MPUH, (start << 16) | end);
		mmio_write_32(EMI_MPUL_2ND, access_permission);
		break;

	case 8:
		if (R8 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R8 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUI2, 0);
		mmio_write_32(EMI_MPUA2, (start << 16) | end);
		mmio_write_32(EMI_MPUI2, access_permission);
		break;

	case 9:
		if (R9 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R9 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUJ2, 0);
		mmio_write_32(EMI_MPUB2, (start << 16) | end);
		mmio_write_32(EMI_MPUJ2, access_permission);
		break;

	case 10:
		if (R10 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R10 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK2, 0);
		mmio_write_32(EMI_MPUC2, (start << 16) | end);
		mmio_write_32(EMI_MPUK2, access_permission);
		break;

	case 11:
		if (R11 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R11 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL2, 0);
		mmio_write_32(EMI_MPUD2, (start << 16) | end);
		mmio_write_32(EMI_MPUL2, access_permission);
		break;

	case 12:
		if (R12 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R12 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUI2_2ND, 0);
		mmio_write_32(EMI_MPUE2, (start << 16) | end);
		mmio_write_32(EMI_MPUI2_2ND, access_permission);
		break;

	case 13:
		if (R13 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R13 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUJ2_2ND, 0);
		mmio_write_32(EMI_MPUF2, (start << 16) | end);
		mmio_write_32(EMI_MPUJ2_2ND, access_permission);
		break;

	case 14:
		if (R14 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R14 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK2_2ND, 0);
		mmio_write_32(EMI_MPUG2, (start << 16) | end);
		mmio_write_32(EMI_MPUK2_2ND, access_permission);
		break;

	case 15:
		if (R15 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R15 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL2_2ND, 0);
		mmio_write_32(EMI_MPUH2, (start << 16) | end);
		mmio_write_32(EMI_MPUL2_2ND, access_permission);
		break;

	case 16:
		if (R16 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R16 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUI3, 0);
		mmio_write_32(EMI_MPUA3, (start << 16) | end);
		mmio_write_32(EMI_MPUI3, access_permission);
		break;

	case 17:
		if (R17 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R17 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUJ3, 0);
		mmio_write_32(EMI_MPUB3, (start << 16) | end);
		mmio_write_32(EMI_MPUJ3, access_permission);
		break;

	case 18:
		if (R18 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R18 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK3, 0);
		mmio_write_32(EMI_MPUC3, (start << 16) | end);
		mmio_write_32(EMI_MPUK3, access_permission);
		break;

	case 19:
		if (R19 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R19 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL3, 0);
		mmio_write_32(EMI_MPUD3, (start << 16) | end);
		mmio_write_32(EMI_MPUL3, access_permission);
		break;

	case 20:
		if (R20 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R20 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUI3_2ND, 0);
		mmio_write_32(EMI_MPUE3, (start << 16) | end);
		mmio_write_32(EMI_MPUI3_2ND, access_permission);
		ATF_EMI_MPUI3_2ND = access_permission;
		break;

	case 21:
		if (R21 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R21 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUJ3_2ND, 0);
		mmio_write_32(EMI_MPUF3, (start << 16) | end);
		mmio_write_32(EMI_MPUJ3_2ND, access_permission);
		ATF_EMI_MPUJ3_2ND = access_permission;
		break;

	case 22:
		if (R22 == 1)
			return SIP_SVC_E_PERMISSION_DENY;
		else {
			if (locked)
				R22 = 1;
		}
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUK3_2ND, 0);
		mmio_write_32(EMI_MPUG3, (start << 16) | end);
		mmio_write_32(EMI_MPUK3_2ND, access_permission);
		ATF_EMI_MPUK3_2ND = access_permission;
		break;

	case 23:
		/* Clear access right before setting MPU address */
		mmio_write_32(EMI_MPUL3_2ND, 0);
		mmio_write_32(EMI_MPUH3, (start << 16) | end);
		mmio_write_32(EMI_MPUL3_2ND, access_permission);
		ATF_EMI_MPUL3_2ND = access_permission;
		break;

	default:
		return SIP_SVC_E_INVALID_PARAMS;
	}

	return SIP_SVC_E_SUCCESS;
}


void emimpu_setup(void) {
	//emimpu_set_security_access();
}

static int is_emi_mpu_reg_write_forbidden(unsigned int offset, unsigned int reg_value)
{
	return 0;
}


uint64_t sip_emimpu_write(unsigned int offset, unsigned int reg_value)
{
	unsigned int addr;
	if(is_emi_mpu_reg_write_forbidden(offset, reg_value))
		return SIP_SVC_E_INVALID_Range;

	addr = EMI_MPU_BASE + offset;
	mmio_write_32(addr, reg_value);
	dsb();
	return SIP_SVC_E_SUCCESS;
}

uint32_t sip_emimpu_read(unsigned int offset)
{
	unsigned int addr = EMI_MPU_BASE + offset;

	if (addr == EMI_MPUI3_2ND)
		return ATF_EMI_MPUI3_2ND;
	else if (addr == EMI_MPUJ3_2ND)
		return ATF_EMI_MPUJ3_2ND;
	else if (addr == EMI_MPUK3_2ND)
		return ATF_EMI_MPUK3_2ND;
	else if (addr == EMI_MPUL3_2ND)
		return ATF_EMI_MPUL3_2ND;
	else
		return mmio_read_32(addr);
}
