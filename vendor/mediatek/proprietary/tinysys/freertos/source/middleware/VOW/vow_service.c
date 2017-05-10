/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Header Files
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

#include "vow_service.h"
/*  Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <driver_api.h>
#include <interrupt.h>
#include <mt_reg_base.h>
#include <platform.h>
#include <dma.h>
#include <scp_sem.h>
#if (__VOW_GOOGLE_SUPPORT__)
#include "hotword_dsp_api.h"
#endif
#if (__VOW_MTK_SUPPORT__)
#include "vowAPI_SCP.h"
#endif
#include "pmic_wrap.h"
#include "feature_manager.h"
#include <vcore_dvfs.h>
#include "swVAD.h"
#include "mt_gpt.h"

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Global Variables
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

static struct {
    vow_status_t             vow_status;
    vow_mode_t               vow_mode;
    pmic_status_t            pmic_status;
//    event_t                  vow_event;
//    thread_t                 *vow_thread;
//    timer_t                  vow_timer;
//    short                    digi_gain;
    short                    read_idx;
    short                    write_idx;
    bool                     pre_learn;
    bool                     record_flag;
    bool                     record_firstbuf;
    bool                     dc_flag;
    bool                     dmic_low_power;
    bool                     dc_removal_enable;
    char                     dc_removal_order;
    bool                     periodic_enable;
    int                      drop_count;
    int                      drop_frame;
    int                      dc_count;
    int                      apreg_addr;
    int                      samplerate;
    int                      data_length;
    int                      noKeywordCount;

    vow_model_t              vow_model_event[VOW_MODEL_SIZE];
    vow_key_model_t          vow_model_speaker[VOW_MAX_SPEAKER_MODEL];

} vowserv;


short vowserv_buf_voice[VOW_LENGTH_VOICE] __attribute__ ((aligned (64)));

short vowserv_buf_speaker[VOW_LENGTH_SPEAKER] __attribute__ ((aligned (4)));

int dma_ch = VOW_DMA_CH;

int ipi_send_cnt = 0x00;

SemaphoreHandle_t xWakeVowSemphr;
SemaphoreHandle_t xVowMutex;

static void vow_task(void *pvParameters);

static void vow_getModel_init(void);
static void vow_getModel_fir(void);
static void vow_getModel_speaker(void);
static void vow_getModel_noise(void);

typedef void (*VOW_MODEL_HANDLER)(void);

static VOW_MODEL_HANDLER  _vow_func_table[] = {
    vow_getModel_init,
    vow_getModel_speaker,
    vow_getModel_noise,
    vow_getModel_fir,
};

#if PHASE_DEBUG
unsigned int test_cnt = 0;
#endif

unsigned int task_entry_cnt = 0;

#if TIMES_DEBUG
volatile unsigned int *ITM_CONTROL = (unsigned int *)0xE0000E80;
volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000;
volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004;
volatile unsigned int *DEMCR = (unsigned int *)0xE000EDFC;
unsigned int start_recog_time = 0;
unsigned int end_recog_time = 0;
unsigned int max_recog_time = 0;
unsigned int start_task_time = 0;
unsigned int end_task_time = 0;
unsigned int max_task_time = 0;
unsigned int max_task_entry_cnt = 0;
unsigned int error_data_amount = 0;
unsigned int error_r_idx = 0;
unsigned int error_w_idx = 0;
unsigned int swvad_flag = 0;

#define CPU_RESET_CYCLECOUNTER() \
do { \
*DEMCR = *DEMCR | 0x01000000; \
*DWT_CYCCNT = 0; \
*DWT_CONTROL = *DWT_CONTROL | 1 ; \
} while(0)

// Test Method
// CPU_RESET_CYCLECOUNTER();
// start_time = *DWT_CYCCNT;
#endif

/***********************************************************************************
** vow_init - Initial funciton in scp init
************************************************************************************/
void vow_init(void)
{
    int I;

    VOW_DBG("init\n\r");


    vowserv.vow_status      = VOW_STATUS_IDLE;
    vowserv.vow_mode        = VOW_MODE_SCP_VOW;
    vowserv.read_idx        = VOW_LENGTH_VOICE + 1;
    vowserv.write_idx       = 0;
    vowserv.pre_learn       = false;
    vowserv.record_flag     = false;
    vowserv.record_firstbuf = true;
    vowserv.dc_flag         = true;
    vowserv.dc_count        = 0;
    vowserv.drop_frame      = VOW_DROP_FRAME_FOR_DC;
    vowserv.apreg_addr      = 0;
    vowserv.dmic_low_power  = false;
    vowserv.samplerate      = VOW_SAMPLERATE_16K;
    vowserv.dc_removal_enable = true;
    vowserv.dc_removal_order = 5; // default
    vowserv.periodic_enable = true;

    for (I = 0; I < VOW_MODEL_SIZE; I++) {
        vowserv.vow_model_event[I].func   = _vow_func_table[I];
        vowserv.vow_model_event[I].addr   = 0;
        vowserv.vow_model_event[I].size   = 0;
        vowserv.vow_model_event[I].flag   = false;
    }

    for (I = 0; I < VOW_MAX_SPEAKER_MODEL; I++) {
        vowserv.vow_model_speaker[I].id      = -1;
        vowserv.vow_model_speaker[I].addr    = 0;
        vowserv.vow_model_speaker[I].size    = 0;
        vowserv.vow_model_speaker[I].enable  = false;
    }

    memset(vowserv_buf_speaker, 0, VOW_LENGTH_SPEAKER * sizeof(short));
    memset(vowserv_buf_voice, 0, VOW_LENGTH_VOICE * sizeof(short));

    vowserv.vow_model_event[VOW_MODEL_SPEAKER].data = (short*)vowserv_buf_speaker;

#if !VOW_LOCALSIM

    // Enable IRQ;
    request_irq(MAD_FIFO_IRQn, VOW_FIFO_IRQHandler, "MAD_FIFO");

    vow_ipi_init();

    // Switch VOW BUCK Voltage
    vow_pwrapper_write(BUCK_VOW_CON0, 0x001C);

    // Create Mutex
    xVowMutex = xSemaphoreCreateMutex();
    if (xVowMutex == NULL) {
        // The semaphore can not be used.
        VOW_DBG("Vow_M fail\n\r");
    }

    // Create Semaphore
    xWakeVowSemphr = xSemaphoreCreateCounting(1, 0);
    if (xWakeVowSemphr == NULL) {
        // The semaphore was created failly.
        // The semaphore can not be used.
        VOW_DBG("Vow_S Fail\n\r");
    }

    // Create vow_task
    kal_xTaskCreate(vow_task, "vow_task", 330, (void *)0, 3, NULL);

#if CFG_TESTSUITE_SUPPORT
    // Start the scheduler.
    vTaskStartScheduler();
#endif
#endif

}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        PMIC WRAPPER ACCESS
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
unsigned short vow_pwrapper_read(unsigned short addr)
{
    unsigned int rdata = 0;
    pwrap_scp_read((unsigned int)addr, (unsigned int *)&rdata);
    return (unsigned short)rdata;
}

void vow_pwrapper_write(unsigned short addr, unsigned short data)
{
    pwrap_scp_write((unsigned int)addr, (unsigned int)data);
    vow_pwrapper_read(addr);//For sync bus data
}

unsigned short vow_pwrapper_read_bits(unsigned short addr, char bits, char len)
{
    unsigned short TargetBitField = ((0x1 << len) - 1) << bits;
    unsigned short CurrValue = vow_pwrapper_read(addr);
    return (CurrValue & TargetBitField) >> bits;
}

void vow_pwrapper_write_bits(unsigned short addr, unsigned short value, char bits, char len)
{
    unsigned short TargetBitField = ((0x1 << len) - 1) << bits;
    unsigned short TargetValue = (value << bits) & TargetBitField;
    unsigned short CurrValue;
    CurrValue = vow_pwrapper_read(addr);
    vow_pwrapper_write(addr, (short)((CurrValue & (~TargetBitField)) | TargetValue));

    return;
}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        PMIC State Control Functions
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

/***********************************************************************************
** vow_pmic_idleTophase1 - set pmic from idle to phase1 mode
************************************************************************************/
void vow_pmic_idleTophase1(void)
{
    VOW_LOG("IdleToP1\n\r");

    //VOW phase1 enable
    // Power down VOW source clock: 0=normal, 1=power down
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 15, 1);
    if (!vowserv.dmic_low_power) {
        // 0: not dmic_low_power --> 1.6MHz
        vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 14, 1);
    } else {
        // 1: dmic_low_power --> 800kHz
        vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 14, 1);
    }
    // VOW interrupt source select: 1=no bias irq source, 0=bias base irq source
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 4, 1);
    // Power on VOW: 1=enable VOW
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 11, 1);
}

/***********************************************************************************
** vow_pmic_phase2Tophase1 - set pmic from phase2 to phase1 mode
************************************************************************************/
void vow_pmic_phase2Tophase1(void)
{
    unsigned int rdata;
    VOW_LOG("P2toP1\n\r");

    // Disable VOW Power
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 11, 1);
    // Clear Periodic On/Off Interrupt flag
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 3, 1);

    //VOW disable FIFO
    rdata = DRV_Reg32(VOW_FIFO_EN);
    rdata &= (~0x3);
    DRV_WriteReg32(VOW_FIFO_EN, rdata);
    //VOW enable FIFO
    rdata |= 0x3;
    DRV_WriteReg32(VOW_FIFO_EN, rdata);

    // Bit[5] vow_intr_sw_mode = 0; Bit[4] vow_intr_sw_val = 0
    vow_pwrapper_write_bits(AFE_VOW_POSDIV_CFG0, 0, 4, 2);

    // S,N value reset: 1=reset to 'h64
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 2, 1);
    // S,N value reset: 0=keep last N
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 2, 1);
    // Reset Periodic On/Off Interrupt flag
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 3, 1);
    // Power on VOW: 1=enable VOW
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 11, 1);
}

/***********************************************************************************
** vow_force_phase2 - set pmic to phase2 mode by SW
************************************************************************************/
void vow_force_phase2(void)
{
    VOW_LOG("ForceP2\n\r");
    //Bit[5] vow_intr_sw_mode = 1; Bit[4] vow_intr_sw_val = 1
    vow_pwrapper_write_bits(AFE_VOW_POSDIV_CFG0, 3, 4, 2);
}

/***********************************************************************************
** vow_pmic_idle - set pmic to idle status
************************************************************************************/
void vow_pmic_idle(void)
{
    VOW_LOG("ToIdle\n\r");
    //VOW phase1 disable and intr clear
    // Power on VOW: 0=disable VOW
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 11, 1);
    // 1: dmic_low_power --> 800kHz
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 14, 1);
    // Bit[4] vow_intr_sw_val = 0
    vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 4, 1);
    // Power down VOW source clock: 0=normal, 1=power down
    vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 15, 1);
}

/***********************************************************************************
** vow_dc_removal - set dc removal function
************************************************************************************/
void vow_dc_removal(char enable)
{
    short data;

    if (enable == 0) {
        data = 0x0456;
    } else {
        data = 0x0405 | ((vowserv.dc_removal_order & 0x0f) << 4);
    }
    vow_pwrapper_write(AFE_VOW_HPF_CFG0, data);
}

/***********************************************************************************
** vow_periodic_on_off - set PMIC to control on/off periodically
************************************************************************************/
#if 0
void vow_periodic_on_off(char enable)
{
    if (enable == 0) {
        vow_pwrapper_write_bits(AFE_VOW_TOP, 0, 3, 1);
    } else {
        vow_pwrapper_write_bits(AFE_VOW_TOP, 1, 3, 1);

        /*  <Periodic ON>  */
        //32k_switch, [15]=0
        vow_pwrapper_write_bits(AFE_VOW_PERIODIC_CFG13, 0, 15, 1);
        //send 00 20BA 0000
        //vow_snrdet_periodic_cfg  = 1
        vow_pwrapper_write_bits(AFE_VOW_PERIODIC_CFG14, 1, 15, 1);
        //send 00 20BC 8000

        //Preamplon
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG2, 0x847B);
        //send 00 20A4 847B
        //preampldcprecharge (only use for AMIC DCC)
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG3, 0x847B);
        //send 00 20A6 847B
        //adclpwrup
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG4, 0x8625);
        //send 00 20A8 8625
        //glbvowlpwen
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG5, 0x8C40);
        //send 00 20AA 8C40
        //digmicen  (analog use for dmic)
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG6, 0x8C40);
        //send 00 20AC 8C40
        //pwdbmicbias0 (for DMIC & AMIC)
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG7, 0x83D7);
        //;;;send 00 20AE 83D7
        //pll_en
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG9, 0x847B);
        //send 00 20B2 847B
        //glb_pwrdn (need inverse)
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG10, 0xC3D7);
        //;;;send 00 20B4 C000
        //vow_on (for digital circuit)
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG11, 0x8666);
        //send 00 20B6 8666
        //dmic_on
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG12, 0x0C40);
        //send 00 20B8 0C40

        /*  <Periodic OFF>  */
        //preamplon
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG13, 0x1354);
        //send 00 20BA 1354
        //preampldcprecharge (only use for AMIC DCC + just traggle 1ms )
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG14, 0x849C);
        //send 00 20BC 849C
        //adclpwrup
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG15, 0x1354);
        //send 00 20BE 1354
        //glbvowlpwen
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG16, 0x187F);
        //send 00 20C0 187F
        //digmicen  (analog use for dmic)
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG17, 0x187F);
        //send 00 20C2 187F
        //pwdbmicbias0 (for DMIC & AMIC)
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG18, 0x1354);
        //;;;send 00 20C4 1354
        //pll_en
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG20, 0x1354);
        //send 00 20C8 1354
        //glb_pwrdn  (need inverse) //if no periodic MICBIAS0 then bypass this control!!!!
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG21, 0x1354);
        //;;;send 00 20CA 1354
        //vow_on
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG22, 0x1333);
        //send 00 20CC 1333
        //dmic_on
        //vow_pwrapper_write(AFE_VOW_PERIODIC_CFG23, 0x187F);
        //send 00 20CE 187F

        //Periodic Enable
        vow_pwrapper_write(AFE_VOW_PERIODIC_CFG0, 0x999A);
        //send 00 20A0 999A
    }
}
#endif

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        VOW Control Functions
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

/***********************************************************************************
** vow_enable - enable VOW
************************************************************************************/
void vow_enable(void)
{
    int ret;
//    unsigned int rdata;

    VOW_LOG("+Enable_%x\n\r", vowserv.vow_status);
    if (vowserv.vow_status == VOW_STATUS_IDLE) {

        ret = vow_model_init(vowserv.vow_model_event[VOW_MODEL_SPEAKER].data);

        // To do: control PMIC
#if !VOW_LOCALSIM

        //VOW_LOG("AP side: %x\n\r", DRV_Reg32(0xA000542C));
        //Enable VOW FIFO
        DRV_WriteReg32(VOW_FIFO_EN, 0x0);
        if (vowserv.samplerate == VOW_SAMPLERATE_16K) {
            DRV_WriteReg32(VOW_RXIF_CFG0, 0x2A130003); //AMIC
        } else {
            DRV_WriteReg32(VOW_RXIF_CFG0, 0x2A130503); //AMIC
        }
        DRV_WriteReg32(VOW_RXIF_CFG1, 0x33180014);
        DRV_WriteReg32(VOW_FIFO_EN, 0x00000003);

#endif

        xSemaphoreTake(xVowMutex, portMAX_DELAY);
        vowserv.read_idx   = VOW_LENGTH_VOICE + 1;
        vowserv.write_idx  = 0;
        vowserv.data_length = 0;
        xSemaphoreGive(xVowMutex);
        DRV_WriteReg32(VOW_FIFO_DATA_THRES, (VOW_SAMPLE_NUM_IRQ + 1));

        vow_dc_removal(vowserv.dc_removal_enable);

//!        rdata = vow_pwrapper_read(AFE_VOW_TOP);
//!        if (((rdata & 0x8000) == 0x8000) && ((rdata & 0x0800) == 0x0000)) {
        vow_pmic_idleTophase1();
//!        } else {
//!          vow_pmic_phase2Tophase1();
//!        }
        if (vowserv.pre_learn) {
            VOW_LOG("Enable, prelearn\n\r");
            vow_force_phase2();
        }


        vowserv.drop_count = 0;
        vowserv.vow_status = VOW_STATUS_SCP_REG;

        // For SW VAD init
        initSwVad();
        vowserv.noKeywordCount = 0;
#if PHASE_DEBUG
        test_cnt = 0;
#endif

        task_entry_cnt = 0;
#if TIMES_DEBUG
        start_recog_time = 0;
        end_recog_time = 0;
        max_recog_time = 0;
        start_task_time = 0;
        end_task_time = 0;
        max_task_time = 0;
        error_data_amount = 0;
        error_r_idx = 0;
        error_w_idx = 0;
        CPU_RESET_CYCLECOUNTER();
#endif
        // set wakeup source
        scp_wakeup_src_setup(MAD_FIFO_WAKE_CLK_CTRL, 1);
        VOW_DBG("-Enable_%x_%x\n\r", vowserv.vow_status, ret);
    }
}

/***********************************************************************************
s** vow_disable - disable VOW
************************************************************************************/
void vow_disable(void)
{
    VOW_LOG("+Disable_%x\n\r", vowserv.vow_status);
    if (vowserv.vow_status != VOW_STATUS_IDLE) {
#if !VOW_LOCALSIM
        DRV_WriteReg32(VOW_FIFO_EN, 0x0);
#endif
        vow_pmic_idle();
        vow_dc_removal(false);
        vowserv.vow_status = VOW_STATUS_IDLE;
        xSemaphoreTake(xVowMutex, portMAX_DELAY);
        vowserv.read_idx   = VOW_LENGTH_VOICE + 1;
        vowserv.write_idx  = 0;
        vowserv.data_length = 0;
        xSemaphoreGive(xVowMutex);
        vowserv.record_firstbuf = true;


#if VOW_CTP_TEST_PMIC
        vowserv.SramBufAddr = 0xD0001000;
#endif
        // For SW VAD
        vowserv.noKeywordCount = 0;

#if PHASE_DEBUG
        test_cnt = 0;
#endif


        // Clear wakeup source
        scp_wakeup_src_setup(MAD_FIFO_WAKE_CLK_CTRL, 0);
        VOW_DBG("-Disable_%x\n\r", vowserv.vow_status);
    }
}

/***********************************************************************************
** vow_getModel_init - Get init model through DMA
************************************************************************************/
static void vow_getModel_init(void)
{

}

/***********************************************************************************
** vow_getModel_fir - Get FIR parameters through DMA
************************************************************************************/
static void vow_getModel_fir(void)
{

}

/***********************************************************************************
** vow_getModel_speaker - Get speaker model informations through DMA
************************************************************************************/
static void vow_getModel_speaker(void)
{
    char *ptr8;
    int ret;
    (void)ret;

    VOW_DBG("GetModel S_%x %x %x\n\r", vowserv.vow_model_event[VOW_MODEL_SPEAKER].addr, vowserv.vow_model_event[VOW_MODEL_SPEAKER].size, (int)&vowserv_buf_speaker[0]);
    dvfs_enable_DRAM_resource(VOW_MEM_ID);
    dma_transaction_manual((uint32_t)&vowserv_buf_speaker[0], (uint32_t)vowserv.vow_model_event[VOW_MODEL_SPEAKER].addr, vowserv.vow_model_event[VOW_MODEL_SPEAKER].size, NULL, (uint32_t*)&dma_ch);
    dvfs_disable_DRAM_resource(VOW_MEM_ID);
    // To do: DMA Speaker Model into vowserv_buf_speaker
    ptr8 = (char*)vowserv_buf_speaker;

    VOW_DBG("check_%x %x %x %x %x %x\n\r", *(char*)&ptr8[0], *(char*)&ptr8[1], *(char*)&ptr8[2], *(char*)&ptr8[3], *(short*)&ptr8[160], *(int*)&ptr8[7960]);
}

/***********************************************************************************
** vow_getModel_noise - Get noise model informations through DMA
************************************************************************************/
static void vow_getModel_noise(void)
{

}

/***********************************************************************************
** vow_setapreg_addr - Get noise model informations through DMA
************************************************************************************/
void vow_setapreg_addr(int addr)
{
    unsigned int tcm_addr;
    tcm_addr = (unsigned int)&vowserv_buf_voice[0]; // SCP's address
    //(void)tcm_addr;
    vowserv.apreg_addr = ap_to_scp(addr); // AP's address
    VOW_DBG("SetAPREG_%x_%x\n\r", addr, tcm_addr);
}

/***********************************************************************************
** vow_gettcmreg_addr - Get noise model informations through DMA
************************************************************************************/
unsigned int vow_gettcmreg_addr(void)
{
    unsigned int tcm_addr;
    tcm_addr = (unsigned int)&vowserv_buf_voice[0]; // SCP's address
    return tcm_addr;
}

/***********************************************************************************
** vow_set_flag - Enable/Disable specific operations
************************************************************************************/
void vow_set_flag(vow_flag_t flag, short enable)
{
    switch(flag) {
        case VOW_FLAG_DEBUG:
            VOW_DBG("FgDebug_%x\n\r", enable);
            vowserv.record_flag = enable;
            break;
        case VOW_FLAG_PRE_LEARN:
            VOW_DBG("FgPreL_%x\n\r", enable);
            vowserv.pre_learn = enable;
            break;
        case VOW_FLAG_DMIC_LOWPOWER:
            VOW_DBG("FgDmic_%x\n\r", enable);
            vowserv.dmic_low_power = enable;
        default:
            break;
    }
}

/***********************************************************************************
** vow_setmode - handle set mode request
************************************************************************************/
void vow_setmode(vow_mode_t mode)
{
    VOW_LOG("SetMode,%x %x\n\r", vowserv.vow_mode, mode);
    vowserv.vow_mode = mode;
}

/***********************************************************************************
** vow_setModel - set model information
************************************************************************************/
vow_ipi_result vow_setModel(vow_event_info_t type, int id, int addr, int size)
{
    VOW_DBG("SetModel+ %x %x %x %x\n\r", type, id, addr, size);

    switch ((int)type) {

        case VOW_MODEL_SPEAKER:
            vowserv.vow_model_event[VOW_MODEL_SPEAKER].addr = ap_to_scp(addr);
            vowserv.vow_model_event[VOW_MODEL_SPEAKER].size = size;
            vowserv.vow_model_event[VOW_MODEL_SPEAKER].flag = true;
            break;

        default:
            break;
    }

    VOW_DBG("SetModel- %x %x %x %x\n\r", type, id, addr, size);

    xSemaphoreGive(xWakeVowSemphr);
    return VOW_IPI_SUCCESS;
}

/***********************************************************************************
** vow_model_init - Initialize SWIP with different model_type
************************************************************************************/
int vow_model_init(void *SModel)
{
    int ret;
    ret = -1;
#if (__VOW_GOOGLE_SUPPORT__)
    VOW_LOG("Google_M init\n\r");
    ret = GoogleHotwordDspInit((void *)SModel);
#elif (__VOW_MTK_SUPPORT__)
    VOW_LOG("MTK_M init\n\r");
    ret = TestingInit_Model((char *)SModel);
    TestingInit_Table(0);
#else
    VOW_ERR("Not Support\n\r");
#endif
    return ret;
}

/***********************************************************************************
** vow_keyword_Recognize - use SWIP to recognize keyword with different model_type
************************************************************************************/
int vow_keyword_recognize(void *sample_input, int num_sample, int *ret_info)
{

    int ret = -1;
#if (__VOW_GOOGLE_SUPPORT__)
    ret = GoogleHotwordDspProcess(sample_input, num_sample, ret_info);
    if (ret == 1) {
        // Reset in order to resume detection.
        //VOW_LOG("RECOGNIZE OK!!: %x\n\r", *ret_info);
        GoogleHotwordDspReset();
    } else { // ret = 0
        // call MTK SW VAD, check if in "no-sound" place
        vowserv.noKeywordCount++;
        if (returnToPhase1BySwVad((short *)sample_input, vowserv.noKeywordCount)) {
            ret = 2; // Become no sound, back to phase1
        }
    }
#elif (__VOW_MTK_SUPPORT__)
    (void)num_sample;
    ret = onTesting((short *)sample_input, ret_info); // 0: not thing, 1: pass, 2: fail(no-sound)
    if (ret == 0) {
        vowserv.noKeywordCount++;
#if TIMES_DEBUG
        swvad_flag = 1;
#endif
        if (returnToPhase1BySwVad((short *)sample_input, vowserv.noKeywordCount)) {
            ret = 2; // Become no sound, back to phase1
        }
#if TIMES_DEBUG
        swvad_flag = 0;
#endif
    }
    if ((ret == 1) || (ret == 2)) {
        resetSwVadPara();
        vowserv.noKeywordCount = 0;
    }
#else
    VOW_ERR("Not Support\n\r");
#endif
    return ret;
}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        VOW Main Task
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/


/***********************************************************************************
** vow_task - Process VOW Recognition
************************************************************************************/

static void vow_task(void *pvParameters)
{
    int I;
    int ret, RetInfo; //, PreambleMs;
    int w_idx, r_idx, data_amount;
    int LoopAgain = 0;
#if TIMES_DEBUG
    unsigned int Temp_recog_time = 0;
    unsigned int Temp_task_time = 0;
#endif

    while (1) {

        if (LoopAgain == 1) {
            //VOW_DBG("LoopAg\n\r");

            task_entry_cnt++;
            if (task_entry_cnt > 5) {
                vTaskDelay(4);
            }
#if TIMES_DEBUG
            if (task_entry_cnt > max_task_entry_cnt) {
                max_task_entry_cnt = task_entry_cnt;
            }
#endif
            LoopAgain = 0;
        } else {
            xSemaphoreTake(xWakeVowSemphr, portMAX_DELAY);
            task_entry_cnt = 0;
        }
        //VOW_LOG("vow_task\n\r");


        for (I = 0; I < VOW_MODEL_SIZE; I++) {
            if (vowserv.vow_model_event[I].flag) {
                vowserv.vow_model_event[I].func();
                vowserv.vow_model_event[I].flag = false;
                vow_ipi_sendmsg(SCP_IPIMSG_SET_VOW_MODEL_ACK, 0, VOW_IPI_SUCCESS, 0, 1);
            }
        }

        if ((vowserv.vow_status != VOW_STATUS_IDLE) && (vowserv.data_length >= VOW_SAMPLE_NUM_FRAME)) {
#if PHASE_DEBUG
            test_cnt++;
            if (test_cnt == 1)
                VOW_LOG("==P2==\n\r");
#endif

#if TIMES_DEBUG
            start_task_time = *DWT_CYCCNT;
#endif
            //get_wakelock(KEEP_WAKE_VOW);
            xSemaphoreTake(xVowMutex, portMAX_DELAY);
            if (vowserv.read_idx >= VOW_LENGTH_VOICE) {
                vowserv.read_idx = 0;
            }
            // Get current read_index
            r_idx = vowserv.read_idx;
            // Update read_index
            if (vowserv.read_idx < VOW_LENGTH_VOICE) {
                vowserv.read_idx += VOW_SAMPLE_NUM_FRAME;
            }
            if (vowserv.data_length >= VOW_SAMPLE_NUM_FRAME) {
                vowserv.data_length -= VOW_SAMPLE_NUM_FRAME;
            }
            xSemaphoreGive(xVowMutex);

            //vow_turn_on_clk_sys(true);

            //checktime = delay_get_current_tick();
            //vow_print(INFO,"[MD32 VOW]before onTesting+ :%x\n",checktime);

            if (vowserv.record_flag) {
                dvfs_enable_DRAM_resource(VOW_MEM_ID);
                dma_transaction_manual((uint32_t)vowserv.apreg_addr, (uint32_t)&vowserv_buf_voice[r_idx], (VOW_LENGTH_VOWREG << 1), NULL, (uint32_t*)&dma_ch);
                dvfs_disable_DRAM_resource(VOW_MEM_ID);
                ipi_send_cnt = 0;
                while (vow_ipi_sendmsg(SCP_IPIMSG_VOW_DATAREADY, 0, VOW_IPI_SUCCESS, 0, 1) == false) {
                    udelay(1000);  // delay 1ms
                    ipi_send_cnt++;
                    if (ipi_send_cnt > VOW_IPI_TIMEOUT) {
                        VOW_DBG("IPI SEND FAIL, BYPASS:0x%x, %d\n\r", SCP_IPIMSG_VOW_DATAREADY, ipi_send_cnt);
                        break;
                    }
                }
                if (ipi_send_cnt > 0) {
                    VOW_LOG("Timeout_cnt:%d\n\r", ipi_send_cnt);
                }
            }

#if VOW_FINAL

#if TIMES_DEBUG
            start_recog_time = *DWT_CYCCNT;
#endif
            ret = vow_keyword_recognize((void *)&vowserv_buf_voice[r_idx], VOW_LENGTH_VOWREG, &RetInfo);
#if TIMES_DEBUG
            end_recog_time = *DWT_CYCCNT;
#endif
            if (ret == VOW_RESULT_PASS) {
                VOW_LOG("Recog Ok: %x\n\r", RetInfo);
                //VOW_LOG("WakeToP1\n\r");
#if PHASE_DEBUG
                VOW_LOG("P2_cnt:%d\n\r", test_cnt);
                test_cnt = 0;
#endif
                vow_pmic_phase2Tophase1();

                ipi_send_cnt = 0;
                while (vow_ipi_sendmsg(SCP_IPIMSG_VOW_RECOGNIZE_OK, &RetInfo, VOW_IPI_SUCCESS, 4, 1) == false) {
                    udelay(1000);  // delay 1ms
                    ipi_send_cnt++;
                    if (ipi_send_cnt > VOW_IPI_TIMEOUT) {
                        VOW_DBG("IPI SEND FAIL, BYPASS:0x%x, %d\n\r", SCP_IPIMSG_VOW_RECOGNIZE_OK, ipi_send_cnt);
                        break;
                    }
                }
                if (ipi_send_cnt > 0) {
                    VOW_LOG("Timeout_cnt:%d\n\r", ipi_send_cnt);
                }
            } else if (ret == VOW_RESULT_FAIL) {
                if (vowserv.record_flag == false) {
                    VOW_LOG("RejToP1\n\r");
#if PHASE_DEBUG
                    VOW_LOG("P2_cnt:%d\n\r", test_cnt);
                    test_cnt = 0;
#endif
                    vow_pmic_phase2Tophase1();

                }

            } else {
                //VOW_DBG("not thing: %x\n\r", RetInfo);
            }
            //PreambleMs = GoogleHotwordDspGetMaximumAudioPreambleMs();
            //VOW_LOG("PreambleMs: %x\n\r", PreambleMs);
#else
            (void)ret;
            VOW_LOG("Bypass\n");
            RetInfo = 0; // reg ok
            (void)RetInfo;
            vow_ipi_sendmsg(SCP_IPIMSG_VOW_RECOGNIZE_OK, &RetInfo, VOW_IPI_SUCCESS, 4, 1);
#endif
            //release_wakelock(KEEP_WAKE_VOW);
            //checktime = delay_get_current_tick();
            //vow_print(INFO,"[MD32 VOW]finish onTesting- :%x\n",checktime);
            xSemaphoreTake(xVowMutex, portMAX_DELAY);
            w_idx = vowserv.write_idx; // updated
            r_idx = vowserv.read_idx;
            xSemaphoreGive(xVowMutex);
            //VOW_LOG("R: %d/ W: %d\n\r", r_idx, w_idx);
            if (r_idx > w_idx) {
                data_amount = (VOW_LENGTH_VOICE - r_idx + w_idx);
            } else {
                data_amount = w_idx - r_idx;
            }
            if (data_amount >= VOW_SAMPLE_NUM_FRAME) {
                LoopAgain = 1;
            }
#if TIMES_DEBUG
            if (data_amount > VOW_LENGTH_VOICE) {
                // r_idx, w_idx are wrong
                error_data_amount = data_amount;
                error_r_idx = r_idx;
                error_w_idx = w_idx;
            }
#endif

#if TIMES_DEBUG
            end_task_time = *DWT_CYCCNT;
            if (end_recog_time > start_recog_time) {
                Temp_recog_time = (end_recog_time - start_recog_time);
            } else {
                Temp_recog_time = max_recog_time; // for DWT_CYCCNT overflow, giveup this time
            }
            if (Temp_recog_time > max_recog_time) {
                max_recog_time = Temp_recog_time;
            }
            if (Temp_recog_time > 5600000) { //about 50msec = 50000 * 112
                VOW_ERR("ERROR!!! Recognition too long: %d\n\r", Temp_recog_time);
            }
            if (end_task_time > start_task_time) {
                Temp_task_time = (end_task_time - start_task_time);
            } else {
                Temp_task_time = max_task_time; // for DWT_CYCCNT overflow, giveup this time
            }
            if (Temp_task_time > max_task_time) {
                max_task_time = Temp_task_time;
            }
#endif
        }
    }
}


/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        ISR Handler
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

/***********************************************************************************
** VOW_FIFO_IRQHandler - Mic Data Buffer Handler
************************************************************************************/
void VOW_FIFO_IRQHandler(void)
{
    int data_amount, I, data;
    int w_idx;
    int r_idx;
    static BaseType_t xHigherPriorityTaskWoken;

    xHigherPriorityTaskWoken = pdFALSE;
    /*----------------------------------------------------------------------
    //  Check Status
    ----------------------------------------------------------------------*/
    if (vowserv.vow_status == VOW_STATUS_IDLE) {
        //VOW_LOG("mic_idle\n\r");
        for (I = 0; I < VOW_SAMPLE_NUM_IRQ; I++ ) {
            data = DRV_Reg32(VOW_FIFO_DATA);
        }
        data = DRV_Reg32(VOW_FIFO_IRQ_ACK);
        (void) data;
        //return INT_RESCHEDULE;
        return;
    }
    if (vowserv.drop_count < vowserv.drop_frame) {
        //VOW_LOG("drop\n\r");
        for (I = 0; I < VOW_SAMPLE_NUM_IRQ; I++) {
            data = DRV_Reg32(VOW_FIFO_DATA);
        }
        vowserv.drop_count++;
        data = DRV_Reg32(VOW_FIFO_IRQ_ACK);
        (void) data;
        //return INT_RESCHEDULE;
        return;
    }

    xSemaphoreTakeFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    w_idx = vowserv.write_idx;
    r_idx = vowserv.read_idx;
    xSemaphoreGiveFromISR(xVowMutex, &xHigherPriorityTaskWoken);

    if ((w_idx == r_idx) && (vowserv.data_length == VOW_LENGTH_VOICE)) {
        for (I = 0; I < VOW_SAMPLE_NUM_IRQ; I++) {
            data = DRV_Reg32(VOW_FIFO_DATA);
        }
        xSemaphoreGiveFromISR(xWakeVowSemphr, &xHigherPriorityTaskWoken);
        data = DRV_Reg32(VOW_FIFO_IRQ_ACK);
        (void) data;
        //return INT_RESCHEDULE;
        return;
    }

    /*----------------------------------------------------------------------
    //  Read From VOW FIFO and do memcpy
    ----------------------------------------------------------------------*/
    xSemaphoreTakeFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    w_idx = vowserv.write_idx;
    xSemaphoreGiveFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    for (I = w_idx; I < (w_idx + VOW_SAMPLE_NUM_IRQ); I++) {  // VOW_SAMPLE_NUM_IRQ = 80 (5msec sample)
        vowserv_buf_voice[I] = DRV_Reg32(VOW_FIFO_DATA);
    }

    xSemaphoreTakeFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    vowserv.write_idx += VOW_SAMPLE_NUM_IRQ;
    if (vowserv.write_idx >= VOW_LENGTH_VOICE) {
        vowserv.write_idx = 0;
    }
    vowserv.data_length += VOW_SAMPLE_NUM_IRQ;
    xSemaphoreGiveFromISR(xVowMutex, &xHigherPriorityTaskWoken);

    /*----------------------------------------------------------------------
    //  Calculate data amount and trigger VOW task if necessary
    ----------------------------------------------------------------------*/
    xSemaphoreTakeFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    w_idx = vowserv.write_idx; // updated
    r_idx = vowserv.read_idx;
    xSemaphoreGiveFromISR(xVowMutex, &xHigherPriorityTaskWoken);
    if (r_idx > w_idx) {
        data_amount = (VOW_LENGTH_VOICE - r_idx + w_idx);
    } else {
        data_amount = w_idx - r_idx;
    }

    if (data_amount >= VOW_SAMPLE_NUM_FRAME) {  // VOW_SAMPLE_NUM_FRAME = 160 (10msec sample)
        xSemaphoreGiveFromISR(xWakeVowSemphr, &xHigherPriorityTaskWoken);
    }

    data = DRV_Reg32(VOW_FIFO_IRQ_ACK);
    (void) data;

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    return;
}

/***********************************************************************************
** VOW_DMA_IRQHandler - For DMA use
************************************************************************************/
/*
void vow_dma_handler(void* test)
{
    mt_free_gdma(dma_ch);
    dvfs_disable_DRAM_resource(VOW_MEM_ID);
}
*/
