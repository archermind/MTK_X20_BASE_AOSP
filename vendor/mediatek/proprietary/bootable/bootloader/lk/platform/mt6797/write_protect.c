#include <platform/partition.h>
#include <platform/partition_wp.h>
#include <printf.h>
#include <platform/boot_mode.h>
#include <platform/mtk_wdt.h>
#include <platform/env.h>


#ifdef MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT
extern int is_protect2_ready_for_wp(void);
extern int sync_sml_data(void);
#endif

/* check if protect2 is formatted and unmounted correctly. */
static int is_fs_partition_ready_for_wp(const char *part_name)
{
	int part_index = 0;
	int part_id = 0;
	unsigned long fs_part_offset = 0;
	unsigned long fmt_offset = 0x438; /* ext4 offset where the magic exists */
	unsigned long unmount_offset = 0x460; /* check clean unmount flag */
	unsigned char ext4_magic[2] = {0x53, 0xEF};
	unsigned long read_size = 16;
	unsigned long len;
	unsigned char buf[16] = {0};
	int i = 0;
	int ret = 0;

	/* -----------------------------------  */
	/* Get Partition Offset                 */
	/* -----------------------------------  */
	part_index = partition_get_index(part_name);

	if (part_index == -1) {
		dprintf(CRITICAL, "partition not found\n");
		ret = -1;
		goto _exit;
	}

	fs_part_offset = partition_get_offset(part_index);
	/* return offset is always positivie, negative should indicate a failure.
	   this should be fixed in partition_get_offset */
	if (fs_part_offset < 0) {
		dprintf(CRITICAL, "%s offset not found\n", part_name);
		ret = -2;
		goto _exit;
	}

	dprintf(INFO, "%s off:0x%x \n", part_name, fs_part_offset);
	dprintf(INFO, "[before] ext4 magic:0x%x, 0x%x\n", ext4_magic[0], ext4_magic[1]);
	dprintf(INFO, "[before] buf read:0x%x, 0x%x\n", buf[0], buf[1]);
	/* ---------------------------------------- */
	/* reading data at offset 0x438 for format  */
	/* ---------------------------------------- */
	len = emmc_read(part_id, fs_part_offset+fmt_offset, buf, read_size);

	dprintf(INFO, "ext4 magic:0x%x, 0x%x\n", ext4_magic[0], ext4_magic[1]);
	dprintf(INFO, "buf read:0x%x, 0x%x\n", buf[0], buf[1]);

	if (len != read_size) {
		dprintf(CRITICAL, "emmc_read failed!!\n");
		ret = -3;
		goto _exit;
	}

	/* ---------------------------------------- */
	/* checking magic 0xEF53 at offset 0x438    */
	/* ---------------------------------------- */
	for (i = 0 ; i < 2; i++) {
		if (ext4_magic[i] != buf[i]) {
			ret = -4;
			dprintf(CRITICAL, "%s ext4 magic not match!!\n", part_name);
			goto _exit;
		}
	}

	dprintf(INFO, "%s ext4 magic match!!\n", part_name);

	/* ----------------------------------------------- */
	/* reading data at offset 0x460 for clean unmount  */
	/* ----------------------------------------------- */
	len = emmc_read(part_id, fs_part_offset+unmount_offset, buf, read_size);

	dprintf(INFO, "buf read:0x%x, 0x%x\n", buf[0], buf[1]);

	if (len != read_size) {
		dprintf(CRITICAL, "emmc_read failed!!\n");
		ret = -5;
		goto _exit;
	}

	/* ------------------------------------------------------------------------- */
	/* checking offset 460h,  bit 2 (0x4) is clear(0) , 1 means recovery is on-going*/
	/* ------------------------------------------------------------------------- */
	if (0 == (buf[0] & 0x4)) {
		dprintf(INFO, "%s did not perform journal recovery !!\n", part_name);
	} else {
		ret = -6;
		dprintf(CRITICAL, "%s is not clean-unmounted, journal recovery may not finish!!\n", part_name);
		goto _exit;
	}

_exit:
	return ret;
}

int set_write_protect()
{
	int err = 0;
	if (g_boot_mode == NORMAL_BOOT) {
		/*Boot Region*/
		dprintf(INFO, "[%s]: Lock boot region \n", __func__);
		err = partition_write_prot_set("preloader", "preloader", WP_POWER_ON);

		if (err != 0) {
			dprintf(CRITICAL, "[%s]: Lock boot region failed: %d\n", __func__,err);
			return err;
		}
		/*Group 1*/
#ifdef MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT
		/* sync protect1 sml data to protect2 if needed */
		int sync_ret = 0;

		mtk_wdt_disable();

		sync_ret = sync_sml_data();
		if (0 != sync_ret) {
			dprintf(INFO,"sml data not sync \n");
		} else {
			dprintf(INFO,"sml data sync \n");
		}

		mtk_wdt_init();

		if (g_boot_mode == NORMAL_BOOT) {
			if (0 == is_protect2_ready_for_wp()) {
				dprintf(CRITICAL,"[%s]: protect2 is fmt \n", __func__);
				dprintf(CRITICAL, "[%s]: Lock protect2 \n", __func__);
				err = partition_write_prot_set("protect2", "protect2" , WP_POWER_ON);
				if (err != 0) {
					dprintf(CRITICAL, "[%s]: Lock protect region failed:%d\n", __func__,err);
					return err;
				}
			}
		}
#endif

		/*Group 3 - from persist/oemkeystore to system/keystore*/
		unsigned char wp_start[32] = {0};
		unsigned char wp_end[32] = {0};
#ifdef MTK_PERSIST_PARTITION_SUPPORT
		if (0 == is_fs_partition_ready_for_wp("persist")) {
			dprintf(CRITICAL, "[%s]: persist is fmt \n", __func__);
			sprintf(wp_start, "%s", "persist");
		} else {
			sprintf(wp_start, "%s", "oemkeystore");
		}
#else
		sprintf(wp_start, "%s", "oemkeystore");
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
		if (TRUE == seclib_sec_boot_enabled(TRUE)) {
			sprintf(wp_end, "%s", "system");
		} else {
			sprintf(wp_end, "%s", "keystore");
		}
#else
		sprintf(wp_end, "%s", "keystore");
#endif
		dprintf(INFO, "[%s]: Lock %s->%s \n", __func__, wp_start, wp_end);
		err = partition_write_prot_set(wp_start, wp_end, WP_POWER_ON);
		if (err != 0) {
			dprintf(CRITICAL, "[%s]: Lock %s->%s failed:%d\n",
			        __func__, wp_start, wp_end, err);
			return err;
		}
	}

	/*Group 2*/
	dprintf(INFO, "[%s]: Lock seccfg \n", __func__);
	err = partition_write_prot_set("seccfg", "seccfg", WP_POWER_ON);
	if (err != 0) {
		dprintf(CRITICAL, "[%s]: Lock seccfg  failed:%d\n", __func__,err);
		return err;
	}

	return 0;
}

void write_protect_flow()
{
	int bypass_wp = 0;
	int ret = 0;

#ifndef USER_BUILD
	bypass_wp= atoi(get_env("bypass_wp"));
	dprintf(ALWAYS, "bypass write protect flag = %d! \n",bypass_wp);
#endif

	if (!bypass_wp) {
		ret = set_write_protect();
		if (ret != 0)
			dprintf(CRITICAL, "write protect fail! \n");
		dprintf(ALWAYS, "write protect Done! \n");
	} else
		dprintf(ALWAYS, "Bypass write protect! \n");
}

