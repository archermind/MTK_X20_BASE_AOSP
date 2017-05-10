#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <platform/mt_typedefs.h>

#include "lcm_define.h"
#include "lcm_drv.h"
#include "lcm_util.h"


static LCM_STATUS _lcm_util_check_data(char type, const LCM_DATA_T1 *t1)
{
	switch (type) {
		case LCM_UTIL_RESET:
			switch (t1->data) {
				case LCM_UTIL_RESET_LOW:
				case LCM_UTIL_RESET_HIGH:
					break;

				default:
					return LCM_STATUS_ERROR;
			}
			break;

		case LCM_UTIL_MDELAY:
		case LCM_UTIL_UDELAY:
			// no limitation
			break;

		default:
			return LCM_STATUS_ERROR;
	}


	return LCM_STATUS_OK;
}


static LCM_STATUS _lcm_util_check_write_cmd_v1(const LCM_DATA_T5 *t5)
{
	if (t5 == NULL) {
		return LCM_STATUS_ERROR;
	}
	if (t5->cmd == NULL) {
		return LCM_STATUS_ERROR;
	}
	if (t5->size == 0) {
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


static LCM_STATUS _lcm_util_check_write_cmd_v2(const LCM_DATA_T3 *t3)
{
	if (t3 == NULL) {
		return LCM_STATUS_ERROR;
	}
	if ((t3->size > 0) && (t3->data == NULL)) {
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


static LCM_STATUS _lcm_util_check_read_cmd_v2(const LCM_DATA_T4 *t4)
{
	if (t4 == NULL) {
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_util_set_data(const LCM_UTIL_FUNCS *lcm_util, char type, LCM_DATA_T1 *t1)
{
	// check parameter is valid
	if (LCM_STATUS_OK == _lcm_util_check_data(type, t1)) {
		switch (type) {
			case LCM_UTIL_RESET:
				lcm_util->set_reset_pin((unsigned int)t1->data);
				break;

			case LCM_UTIL_MDELAY:
				lcm_util->mdelay((unsigned int)t1->data);
				break;

			case LCM_UTIL_UDELAY:
				lcm_util->udelay((unsigned int)t1->data);
				break;

			default:
				dprintf(0, "[LCM][ERROR] %s: %d \n", __func__, (unsigned int)type);
				return LCM_STATUS_ERROR;
		}
	} else {
		dprintf(0, "[LCM][ERROR] %s: 0x%x, 0x%x \n", __func__, (unsigned int)type, (unsigned int)t1->data);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_util_set_write_cmd_v1(const LCM_UTIL_FUNCS *lcm_util, LCM_DATA_T5 *t5, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd[32];

	// check parameter is valid
	if (LCM_STATUS_OK == _lcm_util_check_write_cmd_v1(t5)) {
		memset(cmd, 0x0, sizeof(unsigned int) * 32);
		for (i=0; i<t5->size; i++) {
			cmd[i] = (t5->cmd[i*4+3] << 24) | (t5->cmd[i*4+2] << 16) | (t5->cmd[i*4+1] << 8) | (t5->cmd[i*4]);
		}
		lcm_util->dsi_set_cmdq(cmd, (unsigned int)t5->size, force_update);
	} else {
		dprintf(0, "[LCM][ERROR] %s: 0x%p, %d, %d \n", __func__, t5->cmd, (unsigned int)t5->size, force_update);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_util_set_write_cmd_v2(const LCM_UTIL_FUNCS *lcm_util, LCM_DATA_T3 *t3, unsigned char force_update)
{
	// check parameter is valid
	if (LCM_STATUS_OK == _lcm_util_check_write_cmd_v2(t3)) {
		lcm_util->dsi_set_cmdq_V2((unsigned char)t3->cmd, (unsigned char)t3->size, (unsigned char*)t3->data, force_update);
	} else {
		dprintf(0, "[LCM][ERROR] %s: 0x%x, %d, 0x%p, %d \n", __func__, (unsigned int)t3->cmd, (unsigned int)t3->size, t3->data, force_update);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}


LCM_STATUS lcm_util_set_read_cmd_v2(const LCM_UTIL_FUNCS *lcm_util, LCM_DATA_T4 *t4, unsigned int *compare)
{
	if (compare == NULL) {
		dprintf(0, "[LCM][ERROR] %s: NULL parameter \n", __func__);
		return LCM_STATUS_ERROR;
	}

	*compare = 0;

	// check parameter is valid
	if (LCM_STATUS_OK == _lcm_util_check_read_cmd_v2(t4)) {
		unsigned char buffer[4];

		lcm_util->dsi_dcs_read_lcm_reg_v2((unsigned char)t4->cmd, buffer, 4);

		if (buffer[t4->location] == (unsigned char)t4->data) {
			*compare = 1;
		}
	} else {
		dprintf(0, "[LCM][ERROR] %s: 0x%x, %d, 0x%x \n", __func__, (unsigned int)t4->cmd, (unsigned int)t4->location, (unsigned int)t4->data);
		return LCM_STATUS_ERROR;
	}

	return LCM_STATUS_OK;
}
#endif

