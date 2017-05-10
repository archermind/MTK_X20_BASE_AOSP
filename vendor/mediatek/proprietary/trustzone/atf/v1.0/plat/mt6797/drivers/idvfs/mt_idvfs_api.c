 /**
 * @file    mt_idvfs_api.c
 * @brief   Driver for Over Current Protect
 *
 */

#include "mt_idvfs_api.h"

#ifdef __ATF_MODE_IDVFS__
#include <debug.h>
#include "mt_cpuxgpt.h"
#else
#include "common.h" /* for printf */
void idvfs_udelay(unsigned int us)
{
	volatile int count;

	do {
		count = us * 10;
		do {} while (count--);
	} while (0);
}
void idvfs_mdelay(unsigned int ms)
{
	unsigned long i;

	do {
		for (i = 0; i < ms; i++)
			idvfs_udelay(1000);
	} while (0);
}
#endif

#define PRINTF_IDVFS	0

/* define setting */

typedef struct IDVFS_setting {
	unsigned int addr;
	unsigned int value;
} IDVFS_SETTING_T;

static const IDVFS_SETTING_T idvfs_setting_table[] = {
	{IDVFS_PLLINDEX,	0x0000f627}, /* (((0x01 << 16) * 2500) / 26) / 100); Fmax setting */
	{IDVFS_PLLSTEP,		0x00010000}, /* ((400L * 100 * (1 << 12)) / 2500)); */
	{IDVFS_PLLOVSHT,	0x00000666}, /* ((10L * 100 * (1 << 12)) / 2500)); */
	{IDVFS_PLLSETTL,	0x00000068}, /* PLL settle time */
	{IDVFS_PLLSSCCTRL,	0x02080018}, /* PLL period time for SSC */
	{IDVFS_PLLSSCDEL,	0x00000069}, /* PLL for SSC ppm */
	{IDVFS_PLLSSCDEL1,	0x00000069}, /* PLL for SSC ppm */
	{IDVFS_VSETTLE,		0x0001001b}, /* mt6313 = 0x000d000d, ds9214 = 0x0001001b */
	{IDVFS_VDELTA,		0x460a001e}, /* mt6313 = 0x50100030, da9214 = 0x460a001e */
	{IDVFS_SWVOL,		0xffff6c46}, /* vproc wait = 0xffff, vsarm fixed = 1180mv, vproc by input */
	{IDVFS_SWREQ,		0x0001e000}, /* default 750/2500 = 30% */
	{ARMPLL_CON6_PTP3,	0x00028ccc}, /* H2L THD PRCT = 40.799805% * 2500 = 1020MHz */
	{ARMPLL_CON7_PTP3,	0x0003b999}, /* L2H THD PRCT = 59.599854% * 2500 = 1490MHz */
	{ARMPLL_CON8_PTP3,	0x00014333}, /* L Clamp PRCT = 20.199951% * 2500 = 505MHz */
	{ARMPLL_CON9_PTP3,	0x00074000}, /* H Clamp PRCT = 116% * 2500 = 2900MHz*/
	{IDVFS_SWAVG,		0x00000021}, /* SW AVG enable, time = 1.260ms */
};

/*******************************************************************************
 * SMC Call for Kernel UDI interface
 #define udi_jtag_clock(sw_tck, i_trst, i_tms, i_tdi, count)
 mt_secure_call(MTK_SIP_KERNEL_UDI_JTAG_CLOCK, (((0x1 << (sw_tck & 0x03)) << 3) |
												((i_trst & 0x01) << 2) |
												((i_tms & 0x01) << 1) |
												(i_tdi & 0x01)),
												count, 0)
 ******************************************************************************/
int UDIRead(unsigned int reg_value, unsigned int t_count)
{
	unsigned short sub_chains, i_trst, i_tms, i_tdi, i_tdo = 0;

	/* sub changes = 1, 2, 4 ,8 for channel 0/1/2/3 */
	sub_chains = ((reg_value >> 3) & 0x0f);
	i_trst = ((reg_value >> 2) & 0x01);
	i_tms  = ((reg_value >> 1) & 0x01);
	i_tdi  =  (reg_value & 0x01);

	rg_sw_trst_w(i_trst);
	udelay(1);
	rg_sw_tms_w(i_tms);
	udelay(1);
	rg_sw_tdi_w(i_tdi);
	udelay(1);

#if PRINTF_IDVFS
	printf("CH = %d, UDI = 0x%x. TRST = %d, TMS = %d, TDI = %d, Count = %d, ",
	sub_chains, ptp3_reg_read(UDI_PTP3_UDI_REG), i_trst, i_tms, i_tdi, t_count);
#endif

	while (t_count--) {
			rg_sw_tck_w(0);
			udelay(1);
			rg_sw_tck_w(sub_chains); /* jtag_sw_tck is 0,1,2,3 */
			udelay(1);
	}

	udelay(1);
	i_tdo = (rg_sw_tdo_r() == 0) ? 0 : 1;

#if PRINTF_IDVFS
	printf(", TDO = %d.\n", rg_sw_tdo_r());
#endif

	return i_tdo;
}
/*******************************************************************************
 * SMC Call for Kernel iDVFS interface
 ******************************************************************************/

/* Fmax = 500~3000, Vproc, */
int API_BIGIDVFSENABLE(unsigned int idvfs_ctrl, unsigned int vproc_mv_x100, unsigned int vsram_mv_x100)
{
	int i;

	printf("iDVFS enable start.\n");

	/* enable idvfs also enable SW channel, and Vproc timeout interrupt, debug mux = 7 */
	if (idvfs_ctrl == 2500)
		ptp3_reg_write(IDVFS_CTL0, 0x0010a202);
	else
		ptp3_reg_write(IDVFS_CTL0, (idvfs_ctrl & 0xfffffffe));
	/* printf("[idvfs]idvfs_ctrl = 0x%x.\n", ptp3_reg_read(IDVFS_CTL0)); */

	for (i = 0; i < (sizeof(idvfs_setting_table)/sizeof(IDVFS_SETTING_T)); i++)
		ptp3_reg_write(idvfs_setting_table[i].addr, idvfs_setting_table[i].value);

	/* Read Vproc from SOC level <-- how to convert format, It is the same format as IDVFS_SWVOL[7:0] */
	/* base 600mv, step 6.25mv, ex: Vproc = 1000mv, sw_vproc = 0x60 */
	/* base 300mv, step 10mv, ex: Vproc = 1000mv, sw_vproc = 0x5a */
	/* rg_sw_vproc_w((((vproc_mv_x100 - 60000) / 625) & 0xff)); */
	rg_sw_vproc_w((((vproc_mv_x100 - 30000) / 1000) & 0xff));

	/* mark VSARM due to fixed 1180 */
	/* 16. Read sramldo_vosel <-- how to convert format, vosel to IDVFS_SWVOL[15:8] is follow this table. */

	printf("IDVFS_PLLINDEX = 0x%x, , rg_armpll_sdm_pcw_r = 0x%x, IDVFS_SWREQ = 0x%x\n",
			ptp3_reg_read(IDVFS_PLLINDEX), rg_armpll_sdm_pcw_r(), ptp3_reg_read(IDVFS_SWREQ));

	/* Wait 40ns */
	udelay(2);

	/* set idvfs init = 1 */
	/* enabel idvfs */
	rg_idvfs_en_w(1);

	printf("iDVFS enable success.\n");

	return 0;
}

/* return 0: Ok, -1: timeout */
int API_BIGIDVFSDISABLE(void)
{
	/* SW hand over */
	unsigned int i;

	printf("iDVFS disable start and pause iDVFS.\n");
	rg_oc_ch_en_w(0);
	rg_nth_ch_en_w(0);
	rg_pause_w(1);
	rg_idvfs_debug_mux_w(0x03);

	i = 0;
	do {
		if (i++ >= 100) {
			/* printf("iDVFS timeout step 1, DEBUGOUT = 0x%x, IDVFS_CTL0 = 0x%x.\n",
						ptp3_reg_read(IDVFS_DEBUGOUT), ptp3_reg_read(IDVFS_CTL0)); */
			/* force timeout */
			/* return -1; */
			goto force_disable;
		}
		/* step 2usec * 100(timeout) = 200usec. */
		udelay(2);
	} while ((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x02) ==  0); /* wait idvfs_debugout[1] == 1 */

	/* set ARMPLL freq */
	rg_idvfs_debug_mux_w(0x07);
	rg_armpll_sdm_pcw_w((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x7fffffff));	/* idvfs_debugout[30:0] */
	/* 1020MHz = 0x273B13B1, 1500MHz(Default) = 0x39B13B13 */
	printf("iDVFS disable ARMPLL = %x.\n", (ptp3_reg_read(IDVFS_DEBUGOUT) & 0x7fffffff));

	/* set POSDIV freq */
	rg_idvfs_debug_mux_w(0x08);
	rg_armpll_posdiv_w((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x7));			/* idvfs_debugout[2:0] */
	/* rg_armpll_posdiv_w(1); */ /* div 2 */
	printf("iDVFS disable POSDIV = %x.\n", ((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x7)));

	/* due to DA9214 10mv step, Vsram fix mode and don't update status when disable iDVFS */
	/* rg_idvfs_debug_mux_w(0x0a); */
	/* rg_sramldo_vosel_w((ptp3_reg_read(IDVFS_DEBUGOUT) >> 24) & 0xf); */
	/* rg_sramldo_vosel_w(0x0e); */

	rg_idvfs_en_w(0);                                                       /* disable idvfs */
	rg_idvfs_debug_mux_w(0x03);

	i = 0;
	do {
		if (i++ >= 100) {
			/* printf("iDVFS timeout step 2, DEBUGOUT = 0x%x, IDVFS_CTL0 = 0x%x.\n",
					ptp3_reg_read(IDVFS_DEBUGOUT), ptp3_reg_read(IDVFS_CTL0)); */
			/* force timeout */
			/* return -1; */
			goto force_disable;
		}
		/* step 2usec * 100(timeout) = 200usec. */
		udelay(2);
		/* wait idvfs_debugout[0] == 1 */
	} while ((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x01) ==  0);

	printf("iDVFS disable success.\n");
	return 0;

force_disable:
	printf("iDVFS force disable.\n");
	rg_force_clear_w(1);
	rg_force_clear_w(0);
	/* force disable */
	ptp3_reg_write(IDVFS_CTL0, 0x0);

	/* force clear but still return ok. */
	return 0;
}

/******************************************************************************
 New SWREQ settting for fixed POS DIV by Fmin
 ARMPLL_CON6_PTP3,	0x00028ccc}, H2L THD PRCT = 40.799805% * 2500 = 1020MHz
 ARMPLL_CON7_PTP3,	0x0003b999}, L2H THD PRCT = 59.599854% * 2500 = 1490MHz
 ARMPLL_CON8_PTP3,	0x00014333}, L Clamp PRCT = 20.199951% * 2500 = 505MHz

 SWREQ new Flow
 pause -> wait idel -> delay 1us -> set SWREQ -> set fmin -> delay 1us -> un-pause
******************************************************************************/

int API_BIGIDVFSSWREQ(unsigned int swreq_reg)
{
	/* SW hand over */
	int i = 0;
	unsigned int temp_debug_mux;

	printf("iDVFS Freq = 0x%x.\n", swreq_reg);
	rg_pause_w(1);
	temp_debug_mux = rg_idvfs_debug_mux_r();
	rg_idvfs_debug_mux_w(0x03);

	/* wait iDVFS idle */
	do {
		if (i++ >= 100) {
			/* printf("iDVFS timeout step 1, DEBUGOUT = 0x%x, IDVFS_CTL0 = 0x%x.\n",
						ptp3_reg_read(IDVFS_DEBUGOUT), ptp3_reg_read(IDVFS_CTL0)); */
			/* force timeout */
			/* return -1; */
			i = -1;
			goto fail_swreq;
		}
		/* step 2usec * 100(timeout) = 200usec. */
		udelay(2);
	} while ((ptp3_reg_read(IDVFS_DEBUGOUT) & 0x02) ==  0); /* wait idvfs_debugout[1] == 1 */
	i = 0;

	udelay(1);
	ptp3_reg_write(IDVFS_SWREQ, swreq_reg);

	/* if opp >= 1.490MHz set fmin = 1020.313G (more then L2H)
	   else opp < 1.490MHz set fmin = 505.55MHz */
	udelay(1);
	if (swreq_reg >= 0x0003b999)
		ptp3_reg_write(ARMPLL_CON8_PTP3, 0x00028d00);
	else
		ptp3_reg_write(ARMPLL_CON8_PTP3, 0x00014333);

fail_swreq:
	rg_idvfs_debug_mux_w(temp_debug_mux);
	udelay(1);
	rg_pause_w(0);
	return i;
}

/* return 0: Ok, return -1: out of range(250~3000), return -2: idvfs cannot be enable */
int API_BIGPLLSETFREQ(int Freq)
{

	unsigned int freq_sdm_pcm;

	/* add for Hector */
	/* park to 26MHZ */

	/* if (rg_armpll_en_r() == 0) { */
	rg_armpll_en_w(0);  /* add by Ue */
	udelay(1);
	/* rg_armpll_sdm_fra_en_w(1); */
	/* udelay(1); */

	if (Freq <= 500) {
		/* Freq < 500 */
		freq_sdm_pcm = (Freq * 4);
		rg_armpll_posdiv_w(2);
	} else if (Freq <= 1000) {
		/* Freq = 501 ~ 1000 */
		freq_sdm_pcm = (Freq * 2);
		rg_armpll_posdiv_w(1);
	} else {
		/* Freq = 1001 ~ 3000 */
		freq_sdm_pcm = Freq;
		rg_armpll_posdiv_w(0);
	}

	rg_armpll_en_w(1);  /* enable pll */
	udelay(1);

	/* dds = ((khz / 1000) << 14) / 26; */

	/* pllsdm= cur freq = max freq */ /* mark for Hector */
	/* rg_armpll_sdm_pcw_w(((1L << 24) * freq_sdm_pcm ) / 26); */
	rg_armpll_sdm_pcw_w((unsigned long long)((1L << 24) * (unsigned long long)freq_sdm_pcm) / 26);
	udelay(1);

	rg_armpll_sdm_pcw_chg_w(1);
	udelay(1);

	rg_armpll_sdm_pcw_chg_w(0);
	udelay(1);

	/* wait pll stable */
	udelay(20);

	/* swithc back to ARMPLL */

#if PRINTF_IDVFS
	printf("Bigpll setting ARMPLL_CON0 = 0x%x, ARMPLL_CON1_PTP3 = 0x%x, Freq = %dMHz, POS_DIV = %d.\n",
		ptp3_reg_read(ARMPLL_CON0_PTP3), ptp3_reg_read(ARMPLL_CON1_PTP3), freq_sdm_pcm, rg_armpll_posdiv_r());
#endif

	return 0;
}

int BigSRAMLDOSet(int mVolts_x100)
{
	unsigned int reg_value;

	/* read ALL SRAM LDO eFuse */
	/* Big SRAMLDO eFuse */
	reg_value = (ptp3_reg_read(0x1020666c) & 0xffff);
	if (reg_value == 0)
		ptp3_reg_write(SRAMLDO_CAL_PTP3, 0x00007777);	/* eFuse empty */
	else
		ptp3_reg_write(SRAMLDO_CAL_PTP3, reg_value);	/* write eFuse data */

	/* Set sramldo_en*=1, modify bit[11:8] = 0x8 by Morris */
	reg_value = ((ptp3_reg_read(SRAMLDO_PTP3) & 0xfffff000) | 0x8f0);
	if (mVolts_x100 < 70000)
		reg_value |= 0x01;	/* 60000 ~ 69999 */
	else if (mVolts_x100 < 90000)
		reg_value |= 0x02;	/* 70000 ~ 89999 */
	else
		reg_value |= (((((mVolts_x100) - 90000) / 2500) + 3));	/* 90000 ~ 120000 */

	/* set voltage */
	ptp3_reg_write(SRAMLDO_PTP3, reg_value);

#if PRINTF_IDVFS
	printf("SRAM LDO volte = %dmv, enable success.\n", mVolts_x100);
#endif

	return 0;

}
