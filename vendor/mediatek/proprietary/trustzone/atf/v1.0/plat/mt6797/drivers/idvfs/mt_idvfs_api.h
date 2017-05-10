#ifndef _MT_IDVFS_API_H
#define _MT_IDVFS_API_H

/**
 * @file    mt_idvfs_api.h
 * @brief   Driver header for iDVFS controller
 *
 */

#define __ATF_MODE_IDVFS__

#ifdef __ATF_MODE_IDVFS__
#include <mmio.h>
#endif

/*******************************************************************************
 * Defines for UDI Service queries
 ******************************************************************************/

 /* REG ACCESS */
#ifdef __ATF_MODE_IDVFS__
#define ptp3_reg_read(addr)			mmio_read_32(addr)
#define ptp3_reg_write(addr, val)	mmio_write_32(addr, val)
#else
#define ptp3_reg_read(addr)			(*(volatile unsigned int *)(addr))
#define ptp3_reg_write(addr, val)	(*(volatile unsigned int *)(addr) = (unsigned int)(val))
#endif

/* #define ptp3_reg_write_mask(addr, mask, val)
	ptp3_reg_write(addr, (ptp3_reg_read(addr) & ~(_BITMASK_(mask))) | _BITS_(mask, val)) */
#define ptp3_shift(val, bits, shift) \
	((val & (((unsigned int)0x1 << bits) - 1)) << shift)
#define ptp3_reg_write_bits(addr, val, bits, shift) \
	ptp3_reg_write(addr, ((ptp3_reg_read(addr) & ~(ptp3_shift((unsigned int)0xffffffff, \
	bits, shift))) | ptp3_shift(val, bits, shift)))
#define ptp3_reg_read_bits(addr, bits, shift) \
	((ptp3_reg_read(addr) >> shift) & (((unsigned int)0x1 << bits) - 1))

/* ptp3 Register Definition */
#define PTP3_BASEADDR                                       0x10222000
#define UDI_PTP3_UDI_REG                                    (PTP3_BASEADDR + 0x004)
/*
Bit(s)	Mnemonic	Name	Description
10:7	sw_tdo	Software JTAG control for top-level GatewayTAP
6		sw_trst	Software JTAG control for top-level GatewayTAP
5		sw_tms	Software JTAG control for top-level GatewayTAP
4	    sw_tdi	Software JTAG control for top-level GatewayTAP
3:0	sw_tck	Software JTAG control for top-level GatewayTAP, 1, 2 ,4 ,8 for sub_chains 0/1/2/3
*/

#define rg_sw_tck_w(val)                                ptp3_reg_write_bits(UDI_PTP3_UDI_REG, val, 4, 0)
#define rg_sw_tck_r()                                   ptp3_reg_read_bits(UDI_PTP3_UDI_REG, 4, 0)
#define rg_sw_tdi_w(val)                                ptp3_reg_write_bits(UDI_PTP3_UDI_REG, val, 1, 4)
#define rg_sw_tdi_r()                                   ptp3_reg_read_bits(UDI_PTP3_UDI_REG, 1, 4)
#define rg_sw_tms_w(val)                                ptp3_reg_write_bits(UDI_PTP3_UDI_REG, val, 1, 5)
#define rg_sw_tms_r()                                   ptp3_reg_read_bits(UDI_PTP3_UDI_REG, 1, 5)
#define rg_sw_trst_w(val)                               ptp3_reg_write_bits(UDI_PTP3_UDI_REG, val, 1, 6)
#define rg_sw_trst_r()                                  ptp3_reg_read_bits(UDI_PTP3_UDI_REG, 1, 6)
#define rg_sw_tdo_w(val)                                ptp3_reg_write_bits(UDI_PTP3_UDI_REG, val, 4, 7)
#define rg_sw_tdo_r()                                   ptp3_reg_read_bits(UDI_PTP3_UDI_REG, 4, 7)

/*******************************************************************************
 * Defines for iDVFS Service queries
 ******************************************************************************/


/* IDVFS Register Definition */

#define IDVFS_CTL0                                      (PTP3_BASEADDR + 0x470)
#define IDVFS_PLLINDEX                                  (PTP3_BASEADDR + 0x474)
#define IDVFS_PLLSTEP                                   (PTP3_BASEADDR + 0x478)
#define IDVFS_PLLOVSHT                                  (PTP3_BASEADDR + 0x47C)
#define IDVFS_PLLSETTL                                  (PTP3_BASEADDR + 0x480)
#define IDVFS_PLLSSCCTRL                                (PTP3_BASEADDR + 0x484)
#define IDVFS_PLLSSCDEL                                 (PTP3_BASEADDR + 0x488)
#define IDVFS_PLLSSCDEL1                                (PTP3_BASEADDR + 0x48C)
#define IDVFS_VSETTLE                                   (PTP3_BASEADDR + 0x490)
#define IDVFS_VDELTA                                    (PTP3_BASEADDR + 0x494)
#define IDVFS_SWREQ                                     (PTP3_BASEADDR + 0x498)
#define IDVFS_SWVOL                                     (PTP3_BASEADDR + 0x49C)
#define ARMPLL_CON0_PTP3                                (PTP3_BASEADDR + 0x4A0)
#define ARMPLL_CON1_PTP3                                (PTP3_BASEADDR + 0x4A4)
#define ARMPLL_CON2_PTP3                                (PTP3_BASEADDR + 0x4A8)
#define ARMPLL_CON3_PTP3                                (PTP3_BASEADDR + 0x4AC)
#define ARMPLL_CON4_PTP3                                (PTP3_BASEADDR + 0x4B0)
#define ARMPLL_CON5_PTP3                                (PTP3_BASEADDR + 0x4B4)
#define ARMPLL_CON6_PTP3                                (PTP3_BASEADDR + 0x4B8)
#define ARMPLL_CON7_PTP3                                (PTP3_BASEADDR + 0x4BC)
#define ARMPLL_CON8_PTP3                                (PTP3_BASEADDR + 0x4C0)
#define ARMPLL_CON9_PTP3                                (PTP3_BASEADDR + 0x4C4)
#define IDVFS_DEBUGOUT                                  (PTP3_BASEADDR + 0x4C8)
#define IDVFS_SWAVG                                     (PTP3_BASEADDR + 0x4CC)

#define SRAMLDO_PTP3                                    (PTP3_BASEADDR + 0x2b0)
#define SRAMLDO_CAL_PTP3                                (PTP3_BASEADDR + 0x2b4)

#define rg_idvfs_en_w(val)                              ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 0)
#define rg_idvfs_en_r()                                 ptp3_reg_read_bits(IDVFS_CTL0, 1, 0)
#define rg_sw_ch_en_w(val)                              ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 1)
#define rg_sw_ch_en_r()                                 ptp3_reg_read_bits(IDVFS_CTL0, 1, 1)
#define rg_oc_ch_en_w(val)                              ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 2)
#define rg_oc_ch_en_r()                                 ptp3_reg_read_bits(IDVFS_CTL0, 1, 2)
#define rg_nth_ch_en_w(val)                             ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 3)
#define rg_nth_ch_en_r()                                ptp3_reg_read_bits(IDVFS_CTL0, 1, 3)
/* sw + oc + nth channel control */
#define rg_all_ch_en_w(val)                             ptp3_reg_write_bits(IDVFS_CTL0, val, 3, 1)
#define rg_all_ch_en_r()                                ptp3_reg_read_bits(IDVFS_CTL0, 3, 1)
#define rg_force_clear_w(val)                           ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 5)
#define rg_force_clear_r()                              ptp3_reg_read_bits(IDVFS_CTL0, 1, 5)
#define rg_pause_w(val)                                 ptp3_reg_write_bits(IDVFS_CTL0, val, 1, 6)
#define rg_pause_r()                                    ptp3_reg_read_bits(IDVFS_CTL0, 1, 6)
#define rg_idvfs_debug_mux_w(val)                       ptp3_reg_write_bits(IDVFS_CTL0, val, 4, 12)
#define rg_idvfs_debug_mux_r()                          ptp3_reg_read_bits(IDVFS_CTL0, 4, 12)
#define rg_armpll_posdiv_w(val)                         ptp3_reg_write_bits(ARMPLL_CON0_PTP3, val, 3, 12)
#define rg_armpll_posdiv_r()                            ptp3_reg_read_bits(ARMPLL_CON0_PTP3, 3, 12)
#define rg_armpll_sdm_fra_en_w(val)                     ptp3_reg_write_bits(ARMPLL_CON0_PTP3, val, 8, 0)
#define rg_armpll_sdm_fra_en_r()                        ptp3_reg_read_bits(ARMPLL_CON0_PTP3, 8, 0)
#define rg_armpll_en_w(val)                             ptp3_reg_write_bits(ARMPLL_CON0_PTP3, val, 1, 0)
#define rg_armpll_en_r()                                ptp3_reg_read_bits(ARMPLL_CON0_PTP3, 1, 0)
#define rg_armpll_sdm_pcw_w(val)                        ptp3_reg_write_bits(ARMPLL_CON1_PTP3, val, 31, 0)
#define rg_armpll_sdm_pcw_r()                           ptp3_reg_read_bits(ARMPLL_CON1_PTP3, 31, 0)
#define rg_armpll_sdm_pcw_chg_w(val)                    ptp3_reg_write_bits(ARMPLL_CON1_PTP3, val, 1, 31)
#define rg_armpll_sdm_pcw_chg()                         ptp3_reg_read_bits(ARMPLL_CON1_PTP3, 1, 31)
#define rg_sramldo_vosel_w(val)                         ptp3_reg_write_bits(SRAMLDO_PTP3, val, 4, 0)
#define rg_sramldo_vosel_r()                            ptp3_reg_read_bits(SRAMLDO_PTP3, 4, 0)
#define rg_sw_vproc_wait_to_w(val)                      ptp3_reg_write_bits(IDVFS_SWVOL, val, 16, 16)
#define rg_sw_vproc_wait_to_r()                         ptp3_reg_read_bits(IDVFS_SWVOL, 16, 16)
#define rg_sw_vsram_w(val)                              ptp3_reg_write_bits(IDVFS_SWVOL, val, 8, 8)
#define rg_sw_vsram_r()                                 ptp3_reg_read_bits(IDVFS_SWVOL, 8, 8)
#define rg_sw_vproc_w(val)                              ptp3_reg_write_bits(IDVFS_SWVOL, val, 8, 0)
#define rg_sw_vproc_r()                                 ptp3_reg_read_bits(IDVFS_SWVOL, 8, 0)
#define rg_sw_avgfreq_len_w(val)                        ptp3_reg_write_bits(IDVFS_SWAVG, val, 3, 4)
#define rg_sw_avgfreq_len_r()                           ptp3_reg_read_bits(IDVFS_SWAVG, 3, 4)
#define rg_sw_avgfreq_en_w(val)                         ptp3_reg_write_bits(IDVFS_SWAVG, val, 1, 0)
#define rg_sw_avgfreq_en_r()                            ptp3_reg_read_bits(IDVFS_SWAVG, 1, 0)
#define rg_sw_avgfreq_r()                               ptp3_reg_read_bits(IDVFS_SWAVG, 15, 16) /* read only  */

/*******************************/
/* iDVFS function */
extern int UDIRead(unsigned int reg_value, unsigned int t_count);
/* Fmax = 500~3000, Vproc */
extern int API_BIGIDVFSENABLE(unsigned int Fmax, unsigned int vproc_mv_x100, unsigned int vsram_mv_x100);
extern int API_BIGIDVFSDISABLE(void);
extern int API_BIGPLLSETFREQ(int Freq);               /* 250~3000(MHz) */
extern int API_BIGIDVFSSWREQ(unsigned int swreq_reg); /* add for protect pod div */
extern int BigSRAMLDOSet(int mVolts_x100);            /* 60000 ~ 120000 */

/* extern int API_BIGIDVFSCHANNEL(int Channelm, int EnDis); */
/* extern int API_BIGIDVFSFREQ(int Freqpct_x100); */ /* freq = 100(1%)~10000(100%) */
/* extern int API_BIGIDVFSSWAVG(int Length, int EnDis); */
/* extern int API_BIGIDVFSSWAVGSTATUS(void); */
/* extern int API_BIGIDVFSPURESWMODE(int function, int parameter); */
/* extern int API_BIGIDVFSSLOWMODE(int pll_100M_us, int volt_step_us); */

/* extern unsigned int API_BIGPLLGETFREQ(void); */ /* retrun 250~300MHz */
/* extern int API_BIGPLLDISABLE(void); */

/* extern int API_BIGPLLSETPOSDIV(int pos_div); */ /* pos_div = 0~7  */
/* extern int API_BIGPLLGETPOSDIV(); */ /* return 0~7 */

/* extern int API_BIGPLLSETPCW(int pcw); */ /* <1000 ~ = 3000(MHz), with our pos div value */
/* extern unsigned int API_BIGPLLGETPCW(void); */ /* return 1000 ~ 3000(MHz), with our pos div value */

/* extern unsigned int BigSRAMLDOGet(void); */ /* move to android */
/* ********************************** */

#ifndef __ATF_MODE_IDVFS__
extern void idvfs_udelay(unsigned int us);
extern void idvfs_mdelay(unsigned int ms);
#define udelay(utime) idvfs_udelay(utime)
#define mdelay(mtime) idvfs_mdelay(mtime)
#endif

#endif
