#include <platform_def.h>
#include <stdio.h>
#include <stdarg.h>
#include <mmio.h>
#include <cci400.h>
#include <debug.h>
#include <assert.h>
#include "power.h"
#include <psci.h>
#include <arch_helpers.h>
#include "mt_cpuxgpt.h"
#include <platform.h>


#define RETRY_TIME_USEC   (10)

#if SPMC_DEBUG
#define PRINTF_SPMC	printf
#else
void __null_error(const char *fmt, ...) { }
#define PRINTF_SPMC	__null_error
#endif

int pend_off=-1;
extern unsigned long for_delay;
char big_on = 0;//at most 4 cores
char little_on = 0x1; //[7:0] = core7~core0, core 0 is power-on in defualt
int dummy=0;

void power_off_little_cl(unsigned int cl_idx);

void big_spmc_info(){
    printf("SwSeq SPMC T:0x%X C0:0x%X C1:0x%X\n",big_spmc_status(0x10),big_spmc_status(0x1),big_spmc_status(0x2));
}

int power_on_big(const unsigned int linear_id){
    unsigned int tmp;
//     unsigned long i;
    extern void bl31_on_entrypoint(void);
#if FPGA
     if(!dummy)
         return PSCI_E_SUCCESS;
#endif
     PRINTF_SPMC("%s linear_id:%d big_on:%x\n",__FUNCTION__,linear_id,big_on);

    if(linear_id > 9){
        PRINTF_SPMC("The required Big core:%d is not existed\n",linear_id);
        return PSCI_E_NOT_SUPPORTED;
    }
    if(big_on & (1<<(linear_id-8)))
        return PSCI_E_SUCCESS;

    if(mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) & (1<<2))
    {
	PRINTF_SPMC("The required Big core:%d was powered on\n",linear_id);
	return PSCI_E_SUCCESS;
    }

    if(!big_on){
#if SPMC_DVT
        udelay(SPMC_DVT_UDELAY);
        big_spmc_info();
#endif
        power_on_cl3();
#if SPMC_DVT
        udelay(SPMC_DVT_UDELAY);
        big_spmc_info();
#endif

    }
    PRINTF_SPMC("%s before top:%x c0:%x c1:%x\n",__FUNCTION__, big_spmc_status(0x10), big_spmc_status(0x01),big_spmc_status(0x02));
    mmio_write_32(0x10222208, 0xf<<16);//set BIG poweup on AARCH64

    mmio_write_32(MP2_MISC_CONFIG_BOOT_ADDR_L(linear_id-8), (unsigned long)bl31_on_entrypoint);
    mmio_write_32(MP2_MISC_CONFIG_BOOT_ADDR_H(linear_id-8), 0);

    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) & ~(1<<0);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//assert CPUx_PWR_RST_B

    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) | (1<<2);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//Assert PWR_ON_CPUx

    udelay(2);

    while(!(mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<(7-(linear_id-8)))));//PWR_ON_ACK_CPU0, wait for 1
	if(!(mmio_read_32(PTP3_CPU0_SPMC + ((linear_id-8)<<2)) & (1<<17)))
	  printf("SPMC and SPM are dismathed\n");
	while(!(mmio_read_32(PTP3_CPU0_SPMC + ((linear_id-8)<<2)) & (1<<17))); //double check
    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) | (1<<0);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//Release CPU0_PWR_RST_B

    big_on = big_on | (1<<(linear_id-8));
    PRINTF_SPMC("%s linear_id:%d done big_on:%x\n",__FUNCTION__,linear_id,big_on);
    PRINTF_SPMC("%s after top:%x c0:%x c1:%x\n",__FUNCTION__, big_spmc_status(0x10), big_spmc_status(0x01),big_spmc_status(0x02));

#if SPMC_DVT
    udelay(SPMC_DVT_UDELAY);
    big_spmc_info();
#endif

#if SPMC_SPARK2
    big_spark2_setldo(0, 0);
    big_spark2_core(linear_id,1);
#endif

#if SPMC_DVT
//     udelay(SPMC_DVT_UDELAY);
    while(!(mmio_read_32(SPM_CPU_RET_STATUS) & (1<<linear_id)));
    PRINTF_SPMC("core %d is in retention\n",linear_id);
    udelay(SPMC_DVT_UDELAY);
    big_spmc_info();
#endif

    return PSCI_E_SUCCESS;
}

int power_off_big(const unsigned int linear_id){
    unsigned int tmp;
    dummy=1;
    unsigned int x = read_mpidr_el1();
    PRINTF_SPMC("[%x] %s linear_id:%d big_on:%x\n",x,__FUNCTION__,linear_id,big_on);
    if(linear_id > 9){
        PRINTF_SPMC("The required Big core:%d is not existed\n",linear_id);
        return PSCI_E_NOT_SUPPORTED;
    }
    if(!(big_on & 1<<(linear_id-8))){
        PRINTF_SPMC("Big %d have been closed\n",linear_id);
        return PSCI_E_SUCCESS;
    }

	mmio_write_32(0x10222400, 0x1b);
	printf("debug monitor 0x10222404=%x\n",mmio_read_32(0x10222404));

    PRINTF_SPMC("Wait CPU%d's WFI\n",linear_id);
//SCOTT
    while(!(mmio_read_32(SPM_CPU_IDLE_STA2)&(1<<(linear_id+2))));
    PRINTF_SPMC("CPU%d is in WFI\n",linear_id);

	mmio_write_32(0x10222400, 0x1b);
	printf("debug monitor 0x10222404=%x\n",mmio_read_32(0x10222404));



#if FPGA
    tmp = mmio_read_32(PTP3_CPU0_SPMC) | (1<<1);// cpu0_sw_pwr_on _override_en
    mmio_write_32(PTP3_CPU0_SPMC, tmp);//De-assert PWR_ON_CPUx
    PRINTF_SPMC("%s waiting ACK\n");
    while((mmio_read_32(PTP3_CPU0_SPMC) & (1<<17));//PWR_ON_ACK_CPU0, wait for 0
    PRINTF_SPMC("%s got ACK\n");
    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) & ~(1<<0);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//assert CPUx_PWR_RST_B
#else
    //big_spark2_core(linear_id,0); //With the SPMC, the action should be unnecessary
    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) & ~(1<<2);
    PRINTF_SPMC("%s set %x to %x\n",__FUNCTION__,SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2),tmp);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//De-assert PWR_ON_CPUx
    PRINTF_SPMC("%s waiting ACK :%x\n",__FUNCTION__,SPM_CPU_PWR_STATUS);
    while((mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<(7-(linear_id-8)))));//PWR_ON_ACK_CPU0, wait for 0
    PRINTF_SPMC("%s got ACK\n",__FUNCTION__);
    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2)) & ~(1<<0);
    PRINTF_SPMC("%s set %x to %x\n",__FUNCTION__,SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2),tmp);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((linear_id-8)<<2), tmp);//assert CPUx_PWR_RST_B
#endif
    big_on &= ~(1<<(linear_id-8));
    PRINTF_SPMC("%s big_on:%x\n",__FUNCTION__,big_on);
    //no big core is online, turn off the cluster 3
    if(!big_on) {
        cci_disable_cluster_coherency(0x80000200);

        /* disable_scu(mpidr); */
        mmio_write_32(0x1022220C, mmio_read_32(0x1022220C) | ACINACTM | (1 << 0));

        power_off_cl3();
    }

    return PSCI_E_SUCCESS;
}


int power_on_cl3(void)
{
    unsigned int tmp;
    unsigned int output, i = 0;
    unsigned int init = 0;
//     unsigned long i;

	int caller = platform_get_core_pos(read_mpidr_el1());
//    flush_dcache_range(0x10222000,0x1000);
    PRINTF_SPMC("%s before top:%x c0:%x c1:%x\n",__FUNCTION__, big_spmc_status(0x10), big_spmc_status(0x01),big_spmc_status(0x02));
	//This part was moved to kernel
#if 0
    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<0);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release pwr_rst_b

    tmp = mmio_read_32(WDT_SWSYSRST) | 0x88000800;
    mmio_write_32(WDT_SWSYSRST, tmp);

    tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) & ~(0x3);
    mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

    tmp = (mmio_read_32(WDT_SWSYSRST) & ~(0x0800)) | 0x88000000;
    mmio_write_32(WDT_SWSYSRST, tmp);
#endif
    /**
    * All configuration must be placed after here!
    */
pll_retry:
    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<2);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//PWR_ON_CA15MCPUTOP

    udelay(2);

    while(!(mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<17)));//wait PWR_ON_ACK_CA15MCPUTOP, wait for 1
	if(!(mmio_read_32(PTP3_CPUTOP_SPMC) & (1<<17)))
	   printf("SPMC and SPM are dismatched\n");
	while(!(mmio_read_32(PTP3_CPUTOP_SPMC) & (1<<17))); //double check

#if 1
    // print PLL registers
    assert(mmio_read_32(0x102224A0) == 0x00FF1100);
    assert(mmio_read_32(0x102224A4) == 0xB9B13B14);
    assert(mmio_read_32(0x102224AC) == 0x01B10100);
    assert(mmio_read_32(0x102224B0) == 0x00AF00AF);
    assert(mmio_read_32(0x102224B4) == 0x00000010);
#endif

    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<4);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release clk_dis

    if (init != 0)
      mmio_write_32(0x102224A4, 0xA6800000); // Change PLL to 1GHz


    udelay(20);

    /* SW toggle armpll pos div before enable */
    mmio_write_32(ARMPLL_CON0, 0x00ff0100); //ARMPLL_CON0[0], toggle pos div = 0
    udelay(1);

    /* tmp = mmio_read_32(ARMPLL_CON0) | (1<<0);
    mmio_write_32(ARMPLL_CON0, tmp); //ARMPLL_CON0[0], Enable PLL */
    mmio_write_32(ARMPLL_CON0, 0x00ff0101); //ARMPLL_CON0[0], pos div = 0, Enable PLL
    udelay(10);

    /* SW toggle armpll pos div again */
    mmio_write_32(ARMPLL_CON0, 0x00ff1101); //ARMPLL_CON0[0], toggle pos div = 1
    udelay(30);

    /*
    *Enable abist_clk_en = 1'b1 to monitor this clock CLK26CALI_0[12] = 1'b1 (addr: 0x10000220)
    1. Select fqmeter input to ckgen_dbgmux. CLK_DBG_CFG[1:0] = 0 (addr: 0x1000010C)
    2. Select meter clock input. CLK_DBG_CFG[21:16] (addr: 0x1000010C)
    3. setup meter div. MISC_CFG_0[31:24] = 0 (addr: 0x10000104)
    4. Trigger freq meter. CLK26CALI_0[4] = 1(addr: 0x10000220)
    5. Read meter count CLK26CALI_1[15:0](addr: 0x10000224)
    6. Calculate measured freq. freq = (26 * cal_cnt) / 1024;
    */

	//try to get HW SEM
	mmio_write_32(HW_SEM_ENABLE_WORKAROUND_1axxx, 0x0b160001);
	do{
	  mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);
	}while(! (mmio_read_32(HW_SEM_WORKAROUND_1axxx) & 0x1));

	tmp = mmio_read_32(PLL_DIV_MUXL_SEL) | (1<<0);
	mmio_write_32(PLL_DIV_MUXL_SEL, tmp); //pll_div_mux1_sel = 01 = pll clock
	udelay(1);

	//set big clkdiv = 1
	tmp = ((mmio_read_32(0x1001a274) & 0xffffffe0) | 0x8);
	mmio_write_32(0x1001a274, tmp); 
	udelay(1);

	//Release HW SEM
	mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);

#if 1
    #define CLK_DBG_CFG		0x1000010C
    #define CLK26CALI_0		0x10000220
    #define CLK26CALI_1		0x10000224
    #define CLK_MISC_CFG_0	0x10000104
    #define ARMPLL_K1		0x1001A27C
    #define ARMDIV_MON_EN	0x1001A284

	//try to get HW SEM
	mmio_write_32(HW_SEM_ENABLE_WORKAROUND_1axxx, 0x0b160001);
	do{
	  mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);
	}while(! (mmio_read_32(HW_SEM_WORKAROUND_1axxx) & 0x1));

    // check freq meter
	mmio_write_32(ARMPLL_K1, 0x00000000);
	mmio_write_32(ARMDIV_MON_EN, 0xFFFFFFFF);
	udelay(1);
	//Release HW SEM
	mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);

    /*sel ckgen_cksw[0] and enable freq meter sel ckgen[13:8], 01:hd_faxi_ck*/
    //mmio_write_32(CLK_DBG_CFG, (37 << 8) | 0x01);
    tmp = mmio_read_32(CLK_DBG_CFG);
    mmio_write_32(CLK_DBG_CFG, (tmp & 0xFFC0FFFC) | (37 << 16));

    tmp = mmio_read_32(CLK_MISC_CFG_0);
//    mmio_write_32(CLK_MISC_CFG_0, (tmp & 0x00FFFFFF));    /*select divider?dvt set zero*/
    mmio_write_32(CLK_MISC_CFG_0, (tmp & 0x00FFFFFF)|0x01000000);

    mmio_write_32(CLK26CALI_0, 0x1000);
    mmio_write_32(CLK26CALI_0, 0x1010);

    /* wait frequency meter finish */
	i = 0;
    while (mmio_read_32(CLK26CALI_0) & 0x10) {
        udelay(1);
        i++;
        if (i > 100)
            break;
    }

    tmp = mmio_read_32(CLK26CALI_1) & 0xFFFF;
    output = ((tmp * 26000)) / 1024 * 2; /* Khz*/
    //printf("big armpll = %d Khz\n", output);

    // Workaround for ARMPLL issue
    if ((((output > 757500) || (output < 742500)) && (init == 0)) ||
	    (((output > 505000) || (output < 495000)) && (init != 0))) {
	  printf("big armpll = %d Khz, retry = %u.\n", output, ++init);
	  // Step1: Switch Clock Muxed to 26 MHz
	  //try to get HW SEM
	  mmio_write_32(HW_SEM_ENABLE_WORKAROUND_1axxx, 0x0b160001);
	  do{
		mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);
	  }while(! (mmio_read_32(HW_SEM_WORKAROUND_1axxx) & 0x1));

	  tmp = mmio_read_32(PLL_DIV_MUXL_SEL) & ~(0x3);
	  mmio_write_32(PLL_DIV_MUXL_SEL, tmp); //pll_div_mux1_sel = 00 = 26MHz
	  udelay(1);
	  //Release HW SEM
	  mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);

	  // Step2: Disable ARMPLL
	  tmp = mmio_read_32(ARMPLL_CON0) & ~(1<<0);
	  mmio_write_32(ARMPLL_CON0, tmp); //ARMPLL_CON0[0], Disable PLL for workaround
	  udelay(100);

	  // Step3: Power off Cluster2
	  tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<2);
	  mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//De-assert PWR_ON_CA15MCPUTOP
	  while((mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<17)));//wait PWR_ON_ACK_CA15MCPUTOP, wait for 0

	  // DoE 1.5: For External ISO re-assert
	  tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) | (0x2);
	  mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

	  udelay(100);

	  tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) & ~(0x2);
	  mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

	  udelay(240);


	  // Step4: Retry Power-on cluster2
	  goto pll_retry;
    }
	//try to get HW SEM
	  mmio_write_32(HW_SEM_ENABLE_WORKAROUND_1axxx, 0x0b160001);
	  do{
		mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);
	  }while(! (mmio_read_32(HW_SEM_WORKAROUND_1axxx) & 0x1));
	  mmio_write_32(ARMPLL_K1, 0xFFFFFFFF);
	  mmio_write_32(ARMDIV_MON_EN, 0x00000000);
	  udelay(1);
	  //Release HW SEM
	  mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);

#endif

    tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~((1<<2)|(1<<6)|(1<<10));
    mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
    DSB;
    tmp = (1<<2)|(1<<6)|(1<<10);
    while(mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & tmp);//wait all of them be 0*/
    //MP2_AXI_CONFIG, difference from MP0/1
    tmp = mmio_read_32(MISCDBG) & ~(0x1);
    mmio_write_32(MISCDBG, tmp);
    //CCI400, enable S5
    tmp = mmio_read_32(MP2_SNOOP_CTRL) | 0x3 | ((caller+1)<<4);
    mmio_write_32(MP2_SNOOP_CTRL, tmp);
	while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0
    PRINTF_SPMC("%s after top:%x c0:%x c1:%x\n",__FUNCTION__, big_spmc_status(0x10), big_spmc_status(0x01),big_spmc_status(0x02));
    udelay(2);

	//A setting for disabling clock shaper
	tmp = mmio_read_32(0x10222590) | 0x10000;
	mmio_write_32(0x10222590, tmp);
    PRINTF_SPMC("%s disabling clock shaper, REG 0x10222590:0x%x\n",__FUNCTION__, tmp);

    return PSCI_E_SUCCESS;
}

unsigned int power_off_cl3_dummy;
int power_off_cl3(void)
{
    PRINTF_SPMC("%s\n",__FUNCTION__);
    unsigned int tmp, mask;
	int caller = platform_get_core_pos(read_mpidr_el1());
    tmp = (mmio_read_32(MP2_SNOOP_CTRL) & ~0x3) | ((caller+1)<<4);
    mmio_write_32(MP2_SNOOP_CTRL, tmp);
	while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0
	assert((mmio_read_32(MP2_SNOOP_CTRL) & 0x3) == 0);
    
	tmp = mmio_read_32(MISCDBG) | (1<<0) |(1<<4);
    mmio_write_32(MISCDBG, tmp);


    PRINTF_SPMC("Wait CL3's WFI\n");
    while(!(mmio_read_32(SPM_CPU_IDLE_STA2)&(1<<(20))));
    PRINTF_SPMC("CL3 is in WFI\n");

	mask = (1<<2) | (1<<6) | (1<<10);
    tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) | mask;
    mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp); //bit[10]:pwrdnreqn

    while((mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & mask) != mask);
    //bit[10]:pwrdnackn waiting ack,wait for 1

	//try to get HW SEM
	mmio_write_32(HW_SEM_ENABLE_WORKAROUND_1axxx, 0x0b160001);
	do{
	  mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);
	}while(! (mmio_read_32(HW_SEM_WORKAROUND_1axxx) & 0x1));

    tmp = mmio_read_32(PLL_DIV_MUXL_SEL) & ~(1<<0);
    mmio_write_32(PLL_DIV_MUXL_SEL, tmp); //pll_div_mux1_sel = 26MHZ
	udelay(1);

	//Release HW SEM
	mmio_write_32(HW_SEM_WORKAROUND_1axxx, 0x1);


    tmp = mmio_read_32(ARMPLL_CON0) & ~(1<<0);
    mmio_write_32(ARMPLL_CON0, tmp); //ARMPLL_CON0[0], Disable PLL

    tmp = mmio_read_32(ARMPLL_CON0);//A dummy for workaround
    mmio_write_32((uintptr_t)&power_off_cl3_dummy, tmp);//dummy write, avoid optimization


    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<4);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release clk_dis

    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<2);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//De-assert PWR_ON_CA15MCPUTOP

    while((mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<17)));//wait PWR_ON_ACK_CA15MCPUTOP, wait for 0

    tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) | (0x2);
    mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

    tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<0);
    mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release pwr_rst_b

    return PSCI_E_SUCCESS;
}

#if SPMC_SW_MODE
static unsigned int select_spmc_base(int select, unsigned int *nb_srampd)
{
    unsigned int addr_spmc;
    switch(select)
    {
        case 0x1:
            addr_spmc = PTP3_CPU0_SPMC;
            if(nb_srampd)
                *nb_srampd = 16;
            break;
        case 0x2:
            addr_spmc = PTP3_CPU1_SPMC;
            if(nb_srampd)
                *nb_srampd = 16;
            break;
        case 0x10:
            addr_spmc = PTP3_CPUTOP_SPMC;
            if(nb_srampd)
                *nb_srampd = 32;
            break;
        default:
            PRINTF_SPMC("Should be not here\n");
            assert(0);
    }
    return addr_spmc;
}
/**
 * Before enable the SW sequenwce, all of the big cores must be turn offset.
 * SO the function cannot be called on a big core
 * HW-API:BigSPMCSwPwrSeqEn
 * select: 0x1:core0 0x2:core1 0x4:all 0x10:top
 *
 */
/*
   1. if Select invalid, exit/return error -1
    For Select
    2. Read  sw_fsm_override  & fsm_state_out
    3. if sw_fsm_override=0 & fsm_state_out=0
    1. Set all CPU SW state to OFF (no sequencing required)
    - sram_pd = 5'h00 for CPU & 6'h00 for TOP
    - sw_logic_*_pdb = 0
    - sw_sram_sleepb = 0
    - sw_iso = 1, sw_sram_isointb = 0
    - sw_hot_plug_reset = 1
    2. Write all cpu sw_fsm_override = 1
    3. Poll/wait for all cpu fsm_state_out = 1
    3. else return error -2 or -3
    "0: Ok
    -1: Invalid parameter
    -2: sw_fsm_override=1
    -3: fsm_state_out=1
    -4: Timeout occurred"

*/

int big_spmc_sw_pwr_seq_en(int select)
{

    unsigned int tmp, result, retry;
    unsigned int addr_spmc = select_spmc_base(select, 0);
    PRINTF_SPMC("SwPsqE Sel:0x%X Reg:0x%X\n", select, addr_spmc);//big sw power sequence error FSM

    if((mmio_read_32(addr_spmc) & FSM_STATE_OUT_MASK)==FSM_ON)
    {
        //FSM_out is not zero
        PRINTF_SPMC("SwPsqE ESt1\n");//big sw power sequence error FSM
        return -3; //-2: fsm_state_out=1

    }

    if((mmio_read_32(addr_spmc) & SW_FSM_OVERRIDE))
    {
        //FSM_out is not zero
        PRINTF_SPMC("SwPsqE EFO1\n");
        return -2; //-1: sw_fsm_override=1
    }


    if(select == 0x10){
        //Enable buck first
        tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<0);
        mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release pwr_rst_b

        tmp = mmio_read_32(WDT_SWSYSRST) | 0x88000800;
        mmio_write_32(WDT_SWSYSRST, tmp);

        tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) & ~(0x3);
        mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

        tmp = (mmio_read_32(WDT_SWSYSRST) & ~(0x0800)) | 0x88000000;
        mmio_write_32(WDT_SWSYSRST, tmp);

        //TOP
        tmp = mmio_read_32(addr_spmc) & ~((0x3f<<SRAMPD_OFFSET) | SW_LOGIC_PDB | SW_LOGIC_PRE1_PDB | SW_LOGIC_PRE2_PDB| SW_SRAM_SLEEPB | SW_SRAM_ISOINTB);
        tmp |= (SW_ISO | SW_HOT_PLUG_RESET);
        mmio_write_32(addr_spmc,tmp);

        tmp = mmio_read_32(addr_spmc) | SW_FSM_OVERRIDE;
        PRINTF_SPMC("W TOP_SPMC:%x\n",tmp);
        mmio_write_32(addr_spmc, tmp);
        tmp = mmio_read_32(addr_spmc);
        PRINTF_SPMC("RB TOP_SPMC:%x\n",tmp);
        result = 0;

        for(retry=10;retry>0;retry--){

            if((mmio_read_32(addr_spmc) & FSM_STATE_OUT_MASK)==FSM_ON){
                result = 1;
                break;
            }
            udelay(RETRY_TIME_USEC);
        }
        if(!result){
            PRINTF_SPMC("SwPsqE ETopTO\n");//TO=timeout
            return -4;//timeout
        }

//         while(!((mmio_read_32(PTP3_CPUTOP_SPMC) & FSM_STATE_OUT_MASK)==FSM_ON));

    }else if(select == 0x1 || select == 0x2){

        //core 0
        tmp = mmio_read_32(addr_spmc) & ~((0x1f<<SRAMPD_OFFSET) |SW_LOGIC_PDB | SW_LOGIC_PRE1_PDB | SW_LOGIC_PRE2_PDB| SW_SRAM_SLEEPB | SW_SRAM_ISOINTB);
        tmp |= (SW_ISO | SW_HOT_PLUG_RESET);
        mmio_write_32(addr_spmc, tmp);


        tmp = mmio_read_32(addr_spmc) | SW_FSM_OVERRIDE;
        mmio_write_32(addr_spmc, tmp);


//         while(!((mmio_read_32(PTP3_CPU0_SPMC) & FSM_STATE_OUT_MASK) == FSM_ON));
//         return 0;
        result = 0;
        for(retry=10;retry>0;retry--){

            if((mmio_read_32(addr_spmc) & FSM_STATE_OUT_MASK) == FSM_ON){
                result = 1;
                break;
            }
            udelay(RETRY_TIME_USEC);

        }
        if(!result){
            PRINTF_SPMC("SwPsqE ETO:%x\n",select);
            return -4;//timeout
        }
    }
    else
        return -1;

    return 0;
}

/**
 * HW-API: BigSPMCSwPwrOn
 * selectL 0x1: core 1, 0x2:core 2, 0xF:all cores , 0x10:TOP
 */
/*
"1. if Select invalid, exit/return error -1
2. if Select = CPU0, CPU1, or ALLCPU, read TOP SPMC Status. If not powered on, Power  on TOP first
For each Select:
3. Read sw_fsm_override & sw_pwr_on_override_en & pwr state
4. If already power on, continue to next Select.
5. If sw_fsm_override=0
    1. if sw_pwr_on_override=0
         .1. Write sw_pwr_on_override_en = 1
     2. write sw_pwr_on=1
6. else
        Sequence:
        1. sram_pd gray sequencing from 0 -> 11000
        2. sw_logic_pre1_pdb = 1
        3. Poll/wait logic_pre1_pdbo_all_on_ack = 1
        4. sw_logic_pre2_pdb = 1
        5. Poll/wait logic_pre2_pdbo_all_on_ack = 1
        6. sw_logic_pdb = 1
        7. Poll/wait logic_pdbo_all_on_ack = 1
        8. sw_sram_sleepb = 1
        9. sw_iso = 0, sw_sram_isointb = 1
        10. sw_hot_plug_reset = 0"

*/
char sw_pd_cmd[32]={0x01,0x03,0x02,0x06,0x07,0x05,0x04,0x0c,0x0d,0x0f,
                    0x0e,0x0a,0x0b,0x09,0x08,0x18,0x19,0x1b,0x1a,0x1e,
                    0x1f,0x1d,0x1c,0x14,0x15,0x17,0x16,0x12,0x13,0x11,
                    0x10,0x30};

int big_sw_on_seq(int select){
    PRINTF_SPMC("%s select:%x\n",__FUNCTION__,select);
    unsigned int addr_spmc, tmp;
    unsigned int nb_srampd;
    addr_spmc = select_spmc_base(select, &nb_srampd);
    if(!addr_spmc){
        PRINTF_SPMC("SwOn Inv");
        return -1;
    }
    if(mmio_read_32(addr_spmc) & SW_LOGIC_PDBO_ALL_ON_ACK){
        return 0; //turn on already
    }

    PRINTF_SPMC("SwOn ChkOR\n");

    if(!(mmio_read_32(addr_spmc) & SW_FSM_OVERRIDE)){
        tmp = mmio_read_32(addr_spmc);
        if(!( tmp & SW_PWR_ON_OVERRIDE_EN)){
            tmp |= SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(addr_spmc,tmp);
        }
        tmp = mmio_read_32(addr_spmc) | SW_PWR_ON;
        mmio_write_32(addr_spmc,tmp);
    }

    PRINTF_SPMC("SwOn HPRst\n");
    tmp = mmio_read_32(addr_spmc) | SW_HOT_PLUG_RESET;
    mmio_write_32(addr_spmc,tmp);

    PRINTF_SPMC("SwOn OrEn\n");
    tmp |= SW_PWR_ON_OVERRIDE_EN;
    mmio_write_32(addr_spmc,tmp);
    PRINTF_SPMC("SwOn PwrOn\n");

    tmp = mmio_read_32(addr_spmc) | SW_PWR_ON;
    mmio_write_32(addr_spmc,tmp);
    PRINTF_SPMC("SwOn ReRst&ClkDis\n");

    if(select == 0x1){
        tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((0)<<2)) | (1<<0);
        mmio_write_32(SPM_MP2_CPU0_PWR_CON+((0)<<2), tmp);//Release CPU0_PWR_RST_B
    }else if(select == 0x2){
    tmp = mmio_read_32(SPM_MP2_CPU0_PWR_CON+((1)<<2)) | (1<<0);
    mmio_write_32(SPM_MP2_CPU0_PWR_CON+((1)<<2), tmp);//Release CPU0_PWR_RST_B
    }
    else if(select == 0x10){
        tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<4);
        mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release clk_dis
        tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<0);
        mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release pwr_rst_b
    }
    else
        return -1;

    //power on SRAM
    int i;
    unsigned int orig_sram_pd = mmio_read_32(addr_spmc) & 0xFFFF03FF;

    PRINTF_SPMC("SwOn SRAM\n");
    for(i=0;i<nb_srampd;i++)
    {
        tmp = orig_sram_pd | (sw_pd_cmd[i]<<SRAMPD_OFFSET);
        mmio_write_32(addr_spmc,tmp);
    }
    PRINTF_SPMC("SwOn PRE1\n");
    tmp = mmio_read_32(addr_spmc) | SW_LOGIC_PRE1_PDB;
    mmio_write_32(addr_spmc,tmp);
    PRINTF_SPMC("SwOn WaitPRE1Ack\n");
    while(!(mmio_read_32(addr_spmc) & SW_LOGIC_PRE1_PDBO_ALL_ON_ACK));//wait for 1
    big_spmc_info();
    PRINTF_SPMC("SwOn PRE2\n");
    tmp = mmio_read_32(addr_spmc) | SW_LOGIC_PRE2_PDB;
    mmio_write_32(addr_spmc,tmp);
    PRINTF_SPMC("SwOn WaitPRE2Ack\n");
    while(!(mmio_read_32(addr_spmc) & SW_LOGIC_PRE2_PDBO_ALL_ON_ACK));//wait for 1
    big_spmc_info();
    PRINTF_SPMC("SwOn PDB\n");
    tmp = mmio_read_32(addr_spmc) | SW_LOGIC_PDB;
    mmio_write_32(addr_spmc,tmp);
    big_spmc_info();
    PRINTF_SPMC("SwOn WaitPDBAck\n");
    while(!(mmio_read_32(addr_spmc) & SW_LOGIC_PDBO_ALL_ON_ACK));//wait for 1
    big_spmc_info();
    PRINTF_SPMC("SwOn SLEEPB\n");
    tmp = mmio_read_32(addr_spmc) | SW_SRAM_SLEEPB;
    mmio_write_32(addr_spmc,tmp);
    big_spmc_info();
    PRINTF_SPMC("SwOn ISO\n");
    tmp = mmio_read_32(addr_spmc) & ~SW_ISO;
    tmp |= SW_SRAM_ISOINTB;
    mmio_write_32(addr_spmc,tmp);
    big_spmc_info();
    PRINTF_SPMC("SwOn HPDeRST\n");
    tmp = mmio_read_32(addr_spmc) & ~SW_HOT_PLUG_RESET;
    mmio_write_32(addr_spmc,tmp);
    big_spmc_info();

    if(select == 0x10){
        PRINTF_SPMC("SwOn EnPLL\n");
        tmp = mmio_read_32(ARMPLL_CON0) | (1<<0);
        mmio_write_32(ARMPLL_CON0, tmp); //ARMPLL_CON0[0], Enable PLL

        udelay(20);
        PRINTF_SPMC("SwOn SelPLL\n");
        tmp = mmio_read_32(PLL_DIV_MUXL_SEL) | (1<<0);
        mmio_write_32(PLL_DIV_MUXL_SEL, tmp); //pll_div_mux1_sel = 01 = pll clock
        big_spmc_info();
        PRINTF_SPMC("SwOn INFRA_TOPAXI_PROTECTEN1\n");
        tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~(1<<10);
        mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp); //bit[10]:pwrdnreqn
        big_spmc_info();

        PRINTF_SPMC("SwOn WaitAck\n");
        while((mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & (1<<10)));//bit[10]:pwrdnackn waiting ack,wait for 0
        big_spmc_info();
        PRINTF_SPMC("SwOn GotAck\n");
    }
    return 0;
}

int big_spmc_sw_pwr_on(int select){
    //power on a core before power on the TOP, we power on TOP automatically
    extern void bl31_on_entrypoint(void);
    PRINTF_SPMC("SwPsqOn Sel:%d on:%x\n",select,big_on);

    if(big_on & select)
        return PSCI_E_SUCCESS;

    switch(select){
        case 0x1:
        case 0x2:
#if SPMC_DVT
            big_spmc_info();
#endif
            big_spmc_sw_pwr_seq_en(0x10);
#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_sw_on_seq(0x10);//power on TOP

#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            mmio_write_32(0x10222208, 0xf<<16);//set BIG poweup on AARCH64
            unsigned add_idx = select-1;
            mmio_write_32(MP2_MISC_CONFIG_BOOT_ADDR_L(add_idx), (unsigned long)bl31_on_entrypoint);
            mmio_write_32(MP2_MISC_CONFIG_BOOT_ADDR_H(add_idx), 0);
            PRINTF_SPMC("mt_on_2, entry H:%x L:%x\n", mmio_read_32(MP2_MISC_CONFIG_BOOT_ADDR_H(add_idx)),mmio_read_32(MP2_MISC_CONFIG_BOOT_ADDR_L(add_idx)));
//            PRINTF_SPMC("Write 0x10208008=%d\n",8+add_idx);
//            mmio_write_32(0x10208008, 8+add_idx);//discussed with Scott

#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_spmc_sw_pwr_seq_en(select);

#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_sw_on_seq(select);

#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_on = big_on | select;
#if SPMC_SPARK2
            if(select == 0x1)
                big_spark2_setldo(0, 0);
            big_spark2_core(select+7,1);
#endif
            break;
        case 0xf:
            big_spmc_sw_pwr_seq_en(0x10);
            big_spmc_info();
            big_sw_on_seq(0x10);//power on TOP
            big_spmc_info();
            big_sw_on_seq(0x1);
            big_spmc_info();
#if SPMC_SPARK2
            big_spark2_setldo(0, 0);
            big_spark2_core(8,1);
#endif
            big_sw_on_seq(0x2);
            big_spmc_info();
#if SPMC_SPARK2
            big_spark2_core(9,1);
#endif
            break;
        case 0x10:
            big_spmc_sw_pwr_seq_en(0x10);
            big_spmc_info();
            big_sw_on_seq(0x10);
            big_spmc_info();
            break;
        default:
            return -1;
    }
    return PSCI_E_SUCCESS;
}

/**
 * BigSPMCSwPwrOff
 * selectL 0x1: core 1, 0x2:core 2, 0xF:all cores , 0x10:TOP
 */
/*
"1. if Select invalid, exit/return error -1
        ForSelect: (Do CPU's before TOP)
        2. Read sw_fsm_override & sw_pwr_on_override_en & pwr state
        3. If already power off, continue to next Select.
        4. If sw_fsm_override=0
                1. if sw_pwr_on_override=0
                    1. Write sw_pwr_on_override_en = 1
                2. write sw_pwr_on=0
        5. else
       Sequence:
                1. sw_iso=1, sw_sram_isointb=0
                2. sw_logic*_pdb=0, sw_sram_sleepb=0, sw_pd=6’h00
                3. Poll/wait for logic_*pdb_all_on_ack=0 & logic_pdbo_all_off_ack=1
                4. sw_hot_plug_reset=1"

*/
void big_sw_off_seq(int select){
    unsigned int addr_spmc, tmp;
    switch(select)
    {
        case 0x1:
            addr_spmc = PTP3_CPU0_SPMC;
            break;
        case 0x2:
            addr_spmc = PTP3_CPU1_SPMC;
            break;
        case 0x10:
            addr_spmc = PTP3_CPUTOP_SPMC;
            break;
        default:
            PRINTF_SPMC("Should be not here\n");
            assert(0);
    }
    if(!(mmio_read_32(addr_spmc) & FSM_STATE_OUT_MASK))
        return; //turn off already
    if(!(mmio_read_32(addr_spmc) & SW_FSM_OVERRIDE)){
        tmp = mmio_read_32(addr_spmc);
        if(!( tmp & SW_PWR_ON_OVERRIDE_EN)){
            tmp |= SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(addr_spmc,tmp);
        }
        tmp &= ~SW_PWR_ON;
        mmio_write_32(addr_spmc,tmp);
    }
    {
        if(select == 0x10){
            PRINTF_SPMC("Wait CL3's WFI\n");
            while(!(mmio_read_32(SPM_CPU_IDLE_STA2)&(1<<(20))));
            PRINTF_SPMC("CL3 is in WFI\n");
            tmp = mmio_read_32(PLL_DIV_MUXL_SEL) & ~(1<<0);
            mmio_write_32(PLL_DIV_MUXL_SEL, tmp); //pll_div_mux1_sel = 26MHZ
            tmp = mmio_read_32(ARMPLL_CON0) & ~(1<<0);
            mmio_write_32(ARMPLL_CON0, tmp); //ARMPLL_CON0[0], Disable PLL

        }else{
            PRINTF_SPMC("Wait CPU WFI mask:%x\n",select);
            while(!(mmio_read_32(SPM_CPU_IDLE_STA2)&(1<<(9+select))));
            PRINTF_SPMC("CPU is in WFI, mask:%x\n",select);
        }
        tmp = mmio_read_32(addr_spmc) | SW_ISO;
        tmp &= ~SW_SRAM_ISOINTB;
        mmio_write_32(addr_spmc,tmp);

        tmp = mmio_read_32(addr_spmc) & ~(SW_LOGIC_PRE1_PDB | SW_LOGIC_PRE2_PDB | SW_LOGIC_PDB | SW_SRAM_SLEEPB | (0x3f<<SRAMPD_OFFSET));
        mmio_write_32(addr_spmc,tmp);

        while((mmio_read_32(addr_spmc) & SW_LOGIC_PRE1_PDBO_ALL_ON_ACK));//wait for 0
        while((mmio_read_32(addr_spmc) & SW_LOGIC_PRE2_PDBO_ALL_ON_ACK));//wait for 0
        while(!(mmio_read_32(addr_spmc) & SW_LOGIC_PDBO_ALL_OFF_ACK));//wait for 1

        tmp = mmio_read_32(addr_spmc) | SW_HOT_PLUG_RESET;
        mmio_write_32(addr_spmc,tmp);

        if(select == 0x10){

            tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) & ~(1<<0);
            mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release pwr_rst_b

            tmp = mmio_read_32(SPM_MP2_CPUSYS_PWR_CON) | (1<<4);
            mmio_write_32(SPM_MP2_CPUSYS_PWR_CON, tmp);//Release

            tmp = mmio_read_32(SPM_CPU_EXT_BUCK_ISO) | (0x2);
            mmio_write_32(SPM_CPU_EXT_BUCK_ISO, tmp);//ISOLATE

        }

    }
}

int big_spmc_sw_pwr_off(int select){
    switch(select){
        case 0x1:
        case 0x2:
#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_sw_off_seq(select);

#if SPMC_DVT
            udelay(SPMC_DVT_UDELAY);
            big_spmc_info();
#endif

            big_on &= ~(1<<(select-1));
            PRINTF_SPMC("%s big_on:%x\n",__FUNCTION__,big_on);
            if(!big_on){
                big_sw_off_seq(0x10);
#if SPMC_DVT
                udelay(SPMC_DVT_UDELAY);
                big_spmc_info();
#endif
            }
            break;
        case 0xf:
            big_sw_off_seq(0x1);
            big_sw_off_seq(0x2);
            break;
        case 0x10:
            big_sw_off_seq(0x1);
            big_sw_off_seq(0x2);
            big_sw_off_seq(0x10);
            break;
        default:
            return -1;
    }
    return 0;

}

/**
 * BigSPMCSwPwrCntrlDisable
 * select : 0x1: core0, 0x2:core1 0xF:all 0x10:TOP
 * return : 0: Ok -1: Invalid Select value -2: Invalid request.

 */
/*
"1. if Select invalid, exit/return error -1
2. For Select, read sw_fsm_override.
3. If sw_fsm_override = 1
    1. return error -2
4. else
    1. Write all sw_pwr_on_override_en = 0"

*/

int big_spmc_sw_pwr_cntrl_disable(int select)
{
    unsigned int tmp;
    switch(select){
        case 0x1:
            if(mmio_read_32(PTP3_CPU0_SPMC) & SW_FSM_OVERRIDE){
                return -2;
            }
            else{
                tmp = mmio_read_32(PTP3_CPU0_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
                mmio_write_32(PTP3_CPU0_SPMC, tmp);
            }
            break;
        case 0x2:
            if(mmio_read_32(PTP3_CPU1_SPMC) & SW_FSM_OVERRIDE){
                return -2;
            }
            else{
                tmp = mmio_read_32(PTP3_CPU1_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
                mmio_write_32(PTP3_CPU1_SPMC, tmp);
            }
            break;
        case 0xf:
            if((mmio_read_32(PTP3_CPU0_SPMC) & SW_FSM_OVERRIDE)||
                (mmio_read_32(PTP3_CPU1_SPMC) & SW_FSM_OVERRIDE)){
                return -2;
            }
            tmp = mmio_read_32(PTP3_CPU0_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(PTP3_CPU0_SPMC, tmp);
            tmp = mmio_read_32(PTP3_CPU1_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(PTP3_CPU1_SPMC, tmp);
            break;
        case 0x10:
            if((mmio_read_32(PTP3_CPU0_SPMC) & SW_FSM_OVERRIDE)||
                (mmio_read_32(PTP3_CPU1_SPMC) & SW_FSM_OVERRIDE)){
                return -2;
            }
            tmp = mmio_read_32(PTP3_CPU0_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(PTP3_CPU0_SPMC, tmp);
            tmp = mmio_read_32(PTP3_CPU1_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(PTP3_CPU1_SPMC, tmp);
            tmp = mmio_read_32(PTP3_CPUTOP_SPMC) & ~SW_PWR_ON_OVERRIDE_EN;
            mmio_write_32(PTP3_CPUTOP_SPMC, tmp);
        default:
            return -1;
    }
    return 0;
}
#endif /* SPMC_SW_MODE */

/**
 * BigSPMCStatus
 * select : 0x1: core0, 0x2:core1 0x10:TOP
 */
int big_spmc_status(int select){
    unsigned int status;
    switch(select){
        case 0x1:
            status = mmio_read_32(PTP3_CPU0_SPMC);
            break;
        case 0x2:
            status = mmio_read_32(PTP3_CPU1_SPMC);
            break;
        case 0x10:
            status = mmio_read_32(PTP3_CPUTOP_SPMC);
            break;
        default:
            return -1;
    }
    return status;
}

//unsigned long long pwr_base = 0x10006000;	/* SPM_BASE */
//#define SPM_BASE                (0x10006000)

static void mcucfg_set_bit(unsigned long long reg_offset, unsigned int reg_bit, unsigned int value){
    unsigned int current_value;
    unsigned long long mcucfg_base = MCUCFG_BASE;
    if (value == 0) {
        current_value = *(unsigned int *)(reg_offset + mcucfg_base);
        current_value &= (~(1 << reg_bit));
        *(volatile unsigned int *)(reg_offset + mcucfg_base) = current_value;
    } else if (value == 1) {
        current_value = *(unsigned int *)(reg_offset + mcucfg_base);
        current_value |= (1 << reg_bit);
        *(volatile unsigned int *)(reg_offset + mcucfg_base)= current_value;
    }
//   dsb(ishst);
//   dsb(sy);
    __asm__("dsb ishst");
    __asm__("dsb sy");
}

unsigned int mcucfg_get_bit(unsigned long long reg_offset, unsigned int reg_bit)
{
    volatile unsigned int current_value;
    unsigned long long mcucfg_base = MCUCFG_BASE;
    current_value = *(unsigned int *)(reg_offset + mcucfg_base);

    return (((current_value) & (1 << reg_bit)) >> reg_bit);
}

static unsigned int pwr_get_bit(unsigned long long reg_offset, unsigned int reg_bit)
{
    volatile unsigned int current_value;
//     PRINTF_SPMC("%s reg:%x\n",__FUNCTION__,reg_offset + SPM_BASE);
    current_value = *(unsigned int *)(reg_offset + SPM_BASE);

    return (((current_value) & (1 << reg_bit)) >> reg_bit);
}

static void pwr_set_bit(unsigned long long reg_offset, unsigned int reg_bit, unsigned int value)
{
    unsigned int current_value;
//     PRINTF_SPMC("%s reg:%x\n",__FUNCTION__,reg_offset + SPM_BASE);
    if (value == 0) {
        current_value = *(unsigned int *)(reg_offset + SPM_BASE);
        current_value &= (~(1 << reg_bit));
        *(volatile unsigned int *)(reg_offset + SPM_BASE) = current_value;
    } else if (value == 1) {
        current_value = *(unsigned int *)(reg_offset + SPM_BASE);
        current_value |= (1 << reg_bit);
        *(volatile unsigned int *)(reg_offset + SPM_BASE)= current_value;
    }
}

void power_on_little_cl(unsigned int cl_idx){
	int i;
	unsigned long tmp;

	PRINTF_SPMC("%s cl_idx:%d\n",__FUNCTION__,cl_idx);

	unsigned int reg_offset = 0x210 + (cl_idx<<2);
	unsigned int seq_bit[10]={0, 4, 2, 3, 1, 8, 6, 5, 4, 0};
	unsigned int seq_value[10]={0, 1, 1, 1, 0, 0, 1, 0, 0, 1};

	for(i=0; i<10; i++){
		pwr_set_bit(reg_offset, seq_bit[i], seq_value[i]);

		if(i==2){
		  udelay(1);//Wait 200ns for charging CORE power
		}

		if(i==3){
			for(;;){
				if(pwr_get_bit(reg_offset, 2) & pwr_get_bit(reg_offset, 3))
					break;
			}
			udelay(100);
		}

		if(i==5){
			for(;;){
				//printk("SPM value=32\'h%08x\n", *(unsigned int *)(reg_offset + SPM_BASE));
				if(!pwr_get_bit(reg_offset, 12))
					break;
			}
			udelay(500);//at least 100us for ECO
		}

		if(i==6){
			udelay(1);
		}
	}

	PRINTF_SPMC("before INFRA_TOPAXI_PROTECTEN1 (0x%X):0x%08x\n",INFRA_TOPAXI_PROTECTEN1, mmio_read_32(INFRA_TOPAXI_PROTECTEN1));

	if(!cl_idx){
		tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~((1<<0)|(1<<4)|(1<<8)|(1<<11));
		mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
		DSB;
		tmp = (1<<0)|(1<<4)|(1<<8)|(1<<11);
		while(mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & tmp);//wait all of them be 0
		DSB;
		mcucfg_set_bit(0x002C, 4 ,0);//MP0_AXI_CONFIG
		tmp = mmio_read_32(0x10395000) | 0x3;
		mmio_write_32(0x10395000, tmp);
		while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0


	} else {
		tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~(1<<9);
		mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
		DSB;
		while(mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & (1<<9));

		tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~(1<<1);
		mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
		DSB;
		while(mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & (1<<1));

		tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) & ~(1<<5);
		mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
		DSB;
		while(mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & (1<<5));

		mcucfg_set_bit(0x022C, 4 ,0);//MP1_AXI_CONFIG

		tmp = mmio_read_32(0x10394000) | 0x3;
		mmio_write_32(0x10394000, tmp);
		while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0

	}

	PRINTF_SPMC("after INFRA_TOPAXI_PROTECTEN1 (0x%X):0x%08x\n",INFRA_TOPAXI_PROTECTEN1, mmio_read_32(INFRA_TOPAXI_PROTECTEN1));
}

void power_on_little(unsigned int id){
    if(id>7){
        PRINTF_SPMC("%s : wrong CPUID :%d\n",__FUNCTION__,id);
        return;
    }
    if(little_on & (1<<id)){
	PRINTF_SPMC("Little core:%d was turned on already\n");
	return;
    }
    unsigned int reg_offset = 0x220 + (id<<2);
    int i;
    unsigned int seq_bit[14]={16, 0, 4, 8, 9, 8, 2, 3, 16, 6, 1, 5, 4, 0};
    unsigned int seq_value[14]={0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1};
    for(i=0; i<14; i++){
        pwr_set_bit(reg_offset, seq_bit[i], seq_value[i]);
        if(i==3){
            for(;;){
                PRINTF_SPMC("MEM_PWR_ACK=%u\n", pwr_get_bit(reg_offset, 12));
                if(pwr_get_bit(reg_offset, 12))
                    break;
            }
            PRINTF_SPMC("Commnet out PWR_ACK\n");
        }
        if(i==5){
            for(;;){
                PRINTF_SPMC("MEM_PWR_ACK=%u\n", pwr_get_bit(reg_offset, 12));
                if(!pwr_get_bit(reg_offset, 12))
                    break;
            }
        }
        if(i==6){
            PRINTF_SPMC("Delay for PWR_ACK\n");
            udelay(1);
            while(!(mmio_read_32(SPM_CPU_PWR_STATUS) & (1<<(15-id)))); //wait mp0_cpu_pwr_ack==1
        }
        if(i==7){
            PRINTF_SPMC("Delay for PWR_ACK_2nd\n");
            udelay(1);//for safe,it can be removed
            while(!(mmio_read_32(SPM_CPU_PWR_STATUS_2ND) & (1<<(15-id)))); //wait mp0_cpu_pwr_ack==1
        }
        if(i==10){
            PRINTF_SPMC("Delay for memory power ready\n");
            udelay(1);
        }
    }
#if SPMC_SPARK2
    little_spark2_setldo(id);
    little_spark2_core(id,1);
#endif
    little_on |= (1<<id);
    PRINTF_SPMC("Little power on:0x%X\n",little_on);
}

void power_off_little(unsigned int id){
    PRINTF_SPMC("[%x] %s linear_id:%d\n",read_mpidr_el1(),__FUNCTION__,id);

    if(id>7){
        PRINTF_SPMC("%s : wrong CPUID :%d\n",__FUNCTION__,id);
        return;
    }
    if(!(little_on & (1<<id))){
	PRINTF_SPMC("%s : core :%d was turned off already\n",__FUNCTION__,id);
	return;
    }

    if(id<4)
        while(!(mmio_read_32(MP0_CA7L_MISC_CONFIG) & (1<<(24+id))));
    else
        while(!(mmio_read_32(MP1_CA7L_MISC_CONFIG) & (1<<(24+(id-4)))));

    unsigned int reg_offset = 0x220 + (id<<2);//offset in SPM group, checked!
    int i;
    /*
    unsigned int seq_bit[10]={1, 6, 5, 0, 4, 2, 3, 16, 8, 9};
    unsigned int seq_value[10]={1, 0, 1, 0, 1, 0, 0, 0, 1, 1};*/
    unsigned int seq_bit[8]={1, 6, 5, 0, 4, 2, 8, 9};
    unsigned int seq_value[8]={1, 0, 1, 0, 1, 0, 1, 1};

#if SPMC_SPARK2
    little_spark2_core(id, 0);

    //confirm the core is not in the retention mode, bit[x]:core x, wait 0
    PRINTF_SPMC("Watiing spark2 exiting\n");
    while(mmio_read_32(SPM_CPU_RET_STATUS) & (1<<id));
    PRINTF_SPMC("spark2 exiting\n");
    udelay(1);
#endif
    for(i=0; i<8; i++){
        if(i==5){
            //bit 2,3,16 should be set together
            unsigned int tmp = mmio_read_32(SPM_BASE +reg_offset);
            tmp &= ~( 1<<2 | 1<<3 |1<<16);
            mmio_write_32(SPM_BASE +reg_offset, tmp);
            PRINTF_SPMC("Insert PWR_ACK\n");
            udelay(2);
        }
        else
            pwr_set_bit(reg_offset, seq_bit[i], seq_value[i]);

        if(i==6){
            for(;;){
                PRINTF_SPMC("MEM_PWR_ACK=%u\n", pwr_get_bit(reg_offset, 12));
                if(pwr_get_bit(reg_offset, 12))
                    break;
            }
        }
    }

    little_on &= ~(1<<id);
    PRINTF_SPMC("Little power on:0x%X\n",little_on);
    if((!(little_on & 0xF0)) && id>=4)
      power_off_little_cl(1);
    else if((!(little_on & 0xF)) && (id<4))
      power_off_little_cl(0);
    else
      assert("BUG\n");
}

void power_off_little_cl(unsigned int cl_idx){
	int i;
	unsigned int tmp;

	PRINTF_SPMC("%s cl:%d\n",__FUNCTION__,cl_idx);

	unsigned int reg_offset = 0x210 + (cl_idx<<2);
	unsigned int seq_bit[8]={1, 5, 6, 8, 0, 4, 2, 3};
	unsigned int seq_value[8]={1, 1, 0, 1, 0, 1, 0, 0};

    if(!cl_idx){
        tmp = mmio_read_32(0x10395000) & ~0x3;
        mmio_write_32(0x10395000,tmp);
		while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0
        mcucfg_set_bit(0x002C, 4 ,1);//MP1_AXI_CONFIG
    }else{
        tmp = mmio_read_32(0x10394000) & ~0x3;
        mmio_write_32(0x10394000,tmp);
		while(mmio_read_32(MP2_SNOOP_STATUS) & 1); //wait to 0
        mcucfg_set_bit(0x022C, 4 ,1);//MP1_AXI_CONFIG

    }

    while((mmio_read_32(0x10006174)&(1<<(20+cl_idx))) != (1<<(20+cl_idx)));//wait MP1_STANDBYWFIL2 to high

    if(!cl_idx){
        tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) | ((1<<0)|(1<<4)|(1<<8)|(1<<11));
        mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
        PRINTF_SPMC("INFRA_TOPAXI_PROTECTEN1 (0x%X):0x%08x\n",INFRA_TOPAXI_PROTECTEN1, mmio_read_32(INFRA_TOPAXI_PROTECTEN1));
        DSB;
        tmp = (1<<0)|(1<<4)|(1<<8) |(1<<11);
        while((mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & tmp) != tmp);//wait all of them be 1
    }
    else
    {
        tmp = mmio_read_32(INFRA_TOPAXI_PROTECTEN1) | ((1<<1)|(1<<5)|(1<<9));
        mmio_write_32(INFRA_TOPAXI_PROTECTEN1, tmp);
        PRINTF_SPMC("INFRA_TOPAXI_PROTECTEN1 (0x%X):0x%08x\n",INFRA_TOPAXI_PROTECTEN1, mmio_read_32(INFRA_TOPAXI_PROTECTEN1));
        DSB;
        tmp = (1<<1)|(1<<5)|(1<<9);
        while((mmio_read_32(INFRA_TOPAXI_PROTEXTSTA3) & tmp) != tmp);//wait all of them be 1
    }
    DSB;
    for(i=0; i<8; i++){
        pwr_set_bit(reg_offset, seq_bit[i], seq_value[i]);
        if(i==3){
            for(;;){
                if(pwr_get_bit(reg_offset, 12)){
                    break;
                }
            }
        }
    }
    for(;;){
        if(!pwr_get_bit(reg_offset, 2))
            break;
    }
    for(;;){
        if(!pwr_get_bit(reg_offset, 3))
            break;
    }
    PRINTF_SPMC("end\n");
}

/*
"1. Convert mVolts to 6 bit value vretcntrl (exit/return -1 if out of range)
2. Set sparkvretcntrl
3. Write for all CPUs, sw_spark_en = 1"

*/
//switch 0:off 1:on
void big_spark2_setldo(unsigned int cpu0_amuxsel, unsigned int cpu1_amuxsel){
    unsigned int tmp;
    unsigned int sparkvretcntrl = 0x3f;
    PRINTF_SPMC("%s sparkvretcntrl=%x",__FUNCTION__,sparkvretcntrl);
    if(cpu0_amuxsel>7 || cpu1_amuxsel>7){
        return;
    }
    tmp =  cpu1_amuxsel<<9 | cpu0_amuxsel<<6 | sparkvretcntrl;
    mmio_write_32(SPARK2LDO,tmp);
}

int big_spark2_core(unsigned int core, unsigned int sw){
    unsigned int tmp;
    if(sw>1 || core <8 || core>9)
        return -1;
    PRINTF_SPMC("%s core:%d sw:%d\n",__FUNCTION__,core,sw);
    if(core==9){
        tmp = ((mmio_read_32(PTP3_CPU1_SPMC)>>1)<<1) | sw;
        PRINTF_SPMC("Write %x = %x\n",PTP3_CPU1_SPMC,tmp);
        mmio_write_32(PTP3_CPU1_SPMC, tmp);
    }else
    {
        tmp = ((mmio_read_32(PTP3_CPU0_SPMC)>>1)<<1) | sw;
        PRINTF_SPMC("Write %x = %x\n",PTP3_CPU0_SPMC,tmp);
        mmio_write_32(PTP3_CPU0_SPMC, tmp);
    }
    return 0;
}

int little_spark2_setldo(unsigned int core){
    if(core>7)
        return -1;
    unsigned long long base_vret;
    unsigned int offset, tmp, sparkvretcntrl = 0x3f;
    PRINTF_SPMC("%s sparkvretcntrl=%x",__FUNCTION__,sparkvretcntrl);
    if(core<4){
        offset = core;
        base_vret = CPUSYS0_SPARKVRETCNTRL;
    }
    else
    {
        offset = core-4;
        base_vret = CPUSYS1_SPARKVRETCNTRL;
    }
    tmp = (mmio_read_32(base_vret) & ~((0x3f) << (offset<<3))) | (sparkvretcntrl << (offset<<3));
    mmio_write_32(base_vret, tmp);
    return 0;
}

int little_spark2_core(unsigned int core, unsigned int sw){
    if(core>7 || sw >1)
        return -1;
    unsigned int offset, tmp;
    unsigned long long base_ctrl;
    if(core<4){
        offset = core;
        base_ctrl = CPUSYS0_SPARKEN;
    }
    else
    {
        offset = core-4;
        base_ctrl = CPUSYS1_SPARKEN;
    }

    tmp = (mmio_read_32(base_ctrl) & ~(1<<offset)) | (sw<<offset);
    mmio_write_32(base_ctrl, tmp);
    return 0;
}
