/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Header Files
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#ifndef _VOW_SERVICE_H
#define _VOW_SERVICE_H

//#include <sys/types.h>
//#include <ctype.h>
#define VOW_DEBUG 0
#define VOW_ERR(fmt, args...)    PRINTF_I("[vow] ERR: "fmt, ##args)
#define VOW_DBG(fmt, args...)    PRINTF_D("[vow] DBG: "fmt, ##args)
#if VOW_DEBUG
#define VOW_LOG(fmt, args...)    PRINTF_I("[vow] LOG: "fmt, ##args)
#else
#define VOW_LOG(fmt, args...)
#endif

#ifndef false
#define false   0
#endif

#ifndef true
#define true    1
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef bool
typedef unsigned char  bool;
#endif
/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Options
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#define VOW_EARLYPORTING 0
#define VOW_LOCALSIM 0
#define VOW_CTP_TEST_PMIC 0
#define VOW_CTP_TEST_SWIP 0
#define VOW_FINAL 1
#define TIMES_DEBUG 1

#if VOW_DEBUG
#define PHASE_DEBUG 1
#else
#define PHASE_DEBUG 0
#endif

#define __VOW_GOOGLE_SUPPORT__ 0
#define __VOW_MTK_SUPPORT__ 1
/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Options
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
//#if !VOW_LOCALSIM
// #include <platform/mt_typedefs.h>
//#endif


/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Definitions
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

//=============================================================================================
//                 VOW Definitions
//=============================================================================================
#define VOW_MAX_SPEAKER_MODEL             0xA
#define VOW_LENGTH_INIT                   0x2//0x3800
#define VOW_LENGTH_SPEAKER                0x4A00//0x1200
//#define VOW_LENGTH_GOOGLE_SPEAKER         0xA8E4
#define VOW_LENGTH_NOISE                  0x2
#define VOW_LENGTH_FIR                    0x2
#define VOW_LENGTH_VOICE                  0x280
#define VOW_LENGTH_VOICE_HALF             0x140
#define VOW_LENGTH_VOWREG                 0x00A0
#define VOW_PRELEARN_FRAME                0x80
#define VOW_DC_CALIBRATION_FRAME          0xC8
#define VOW_DROP_FRAME                    0x28
#define VOW_DROP_FRAME_FOR_DC             0xC8

#define VOW_SAMPLE_NUM_IRQ                (VOW_LENGTH_VOWREG >> 1)
#define VOW_SAMPLE_NUM_FRAME              VOW_LENGTH_VOWREG

#define VOW_RESULT_NOTHING                0x0
#define VOW_RESULT_PASS                   0x1
#define VOW_RESULT_FAIL                   0x2
#define VOW_RESULT_CHANGE_MODEL           0x3

#define VOW_IPI_TIMEOUT                   100//1ms

//=============================================================================================
//                 SCP Registers
//=============================================================================================
//#define CFGREG_SCP_BASE                     0xD0000000
//#define CFGREG_GENERAL_REG1                 (CFGREG_SCP_BASE + 0x30)

#define VOW_FIFO_BASE                       (0x400A1000)
#define VOW_FIFO_EN                         (VOW_FIFO_BASE + 0x00)  // VIF_FIFO_EN
#define VOW_FIFO_STATUS                     (VOW_FIFO_BASE + 0x04)  // VIF_FIFO_STATUS
#define VOW_FIFO_DATA                       (VOW_FIFO_BASE + 0x08)  // VIF_RXDATA
#define VOW_FIFO_DATA_THRES                 (VOW_FIFO_BASE + 0x0C)  // VIF_FIFO_THRESHOLD
#define VOW_FIFO_IRQ_ACK                    (VOW_FIFO_BASE + 0x10)  // VIF_IRQ_STATUS
#define VOW_RXIF_CFG0                       (VOW_FIFO_BASE + 0x14)  // VIF_RXIF_CFG0
#define VOW_RXIF_CFG1                       (VOW_FIFO_BASE + 0x18)  // VIF_RXIF_CFG1
#define VOW_RXIF_CFG2                       (VOW_FIFO_BASE + 0x1C)  // VIF_RXIF_CFG2
#define VOW_RXIF_OUT                        (VOW_FIFO_BASE + 0x20)
#define VOW_RXIF_STATUS                     (VOW_FIFO_BASE + 0x24)

/*
#define SCP_CLK_CTRL_BASE                   0xD0001000
#define SCP_CLK_SEL                         (SCP_CLK_CTRL_BASE + 0x00)
#define SCP_CLK_EN                          (SCP_CLK_CTRL_BASE + 0x04)
#define SCP_CLK_SAFE_ACK                    (SCP_CLK_CTRL_BASE + 0x08)
#define SCP_HIGH_CLK_VAL                    (SCP_CLK_CTRL_BASE + 0x18)
#define SCP_CG_CTRL                         (SCP_CLK_CTRL_BASE + 0x30)
*/
//=============================================================================================
//                 PMIC Registers
//=============================================================================================
#define PMIC_SYS_TOP_REG_BASE                0x0000
#define PMIC_AUDIO_SYS_TOP_REG_BASE          0x2000

#define BUCK_VOW_CON0                        0x0416
#define AUDENC_ANA_CON12                     0x0D06
#define INT_STATUS0                          0x02AE
#define TOP_CKPDN_CON0_CLR                   0x023C
#define GPIO_MODE                            0x60D0
#define INT_CON0                             0x0296
#define TOP_CKPDN_CON0                       0x0238
#define AFE_VOW_TOP                          (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x70)
#define AFE_VOW_TGEN_CFG0                    (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x8A)
#define AFE_VOW_POSDIV_CFG0                  (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x8C)
#define AFE_VOW_HPF_CFG0                     (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x8E)
#define AFE_VOW_CFG0                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x72)
#define AFE_VOW_CFG1                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x74)
#define AFE_VOW_CFG2                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x76)
#define AFE_VOW_CFG3                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x78)
#define AFE_VOW_CFG4                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x7A)
#define AFE_VOW_CFG5                         (PMIC_AUDIO_SYS_TOP_REG_BASE + 0x7C)
#define AFE_VOW_PERIODIC_CFG0                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xA0)
#define AFE_VOW_PERIODIC_CFG1                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xA2)
#define AFE_VOW_PERIODIC_CFG2                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xA4)
#define AFE_VOW_PERIODIC_CFG3                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xA6)
#define AFE_VOW_PERIODIC_CFG4                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xA8)
#define AFE_VOW_PERIODIC_CFG5                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xAA)
#define AFE_VOW_PERIODIC_CFG6                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xAC)
#define AFE_VOW_PERIODIC_CFG7                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xAE)
#define AFE_VOW_PERIODIC_CFG8                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xB0)
#define AFE_VOW_PERIODIC_CFG9                (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xB2)
#define AFE_VOW_PERIODIC_CFG10               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xB4)
#define AFE_VOW_PERIODIC_CFG11               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xB6)
#define AFE_VOW_PERIODIC_CFG12               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xB8)
#define AFE_VOW_PERIODIC_CFG13               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xBA)
#define AFE_VOW_PERIODIC_CFG14               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xBC)
#define AFE_VOW_PERIODIC_CFG15               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xBE)
#define AFE_VOW_PERIODIC_CFG16               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xC0)
#define AFE_VOW_PERIODIC_CFG17               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xC2)
#define AFE_VOW_PERIODIC_CFG18               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xC4)
#define AFE_VOW_PERIODIC_CFG19               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xC6)
#define AFE_VOW_PERIODIC_CFG20               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xC8)
#define AFE_VOW_PERIODIC_CFG21               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xCA)
#define AFE_VOW_PERIODIC_CFG22               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xCC)
#define AFE_VOW_PERIODIC_CFG23               (PMIC_AUDIO_SYS_TOP_REG_BASE + 0xCE)


//=============================================================================================
//                 DMA Registers
//=============================================================================================
/*
#define AP_DMA_BASE                          0xE1000000
#define AP_DMA_HIF1_BASE                     (AP_DMA_BASE + 0x100)
#define AP_DMA_HIF_1_INT_EN                  (AP_DMA_HIF1_BASE + 0x04)
#define AP_DMA_HIF_1_EN                      (AP_DMA_HIF1_BASE + 0x08)
#define AP_DMA_HIF_1_CON                     (AP_DMA_HIF1_BASE + 0x18)
#define AP_DMA_HIF_SRC_ADDR                  (AP_DMA_HIF1_BASE + 0x1C)
#define AP_DMA_HIF_1_DST_ADDR                (AP_DMA_HIF1_BASE + 0x20)
#define AP_DMA_HIF_1_LEN                     (AP_DMA_HIF1_BASE + 0x24)

#define DMA_CFG                              0xD00580A4
#define DMA_DATA                             0xD00583F0
*/
/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Enumrations & Data Structures
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
typedef struct {
    void   (*func)(void);
    int    addr;
    int    size;
    short  *data;
    bool   flag;
} vow_model_t;

typedef struct {
    int     id;
    int     addr;
    int     size;
    bool    enable;
} vow_key_model_t;

typedef enum vow_event_info_t {
    VOW_MODEL_INIT = 0,
    VOW_MODEL_SPEAKER,
    VOW_MODEL_NOISE,
    VOW_MODEL_FIR,
    // Add before this
    VOW_MODEL_SIZE
} vow_event_info_t;

typedef enum vow_ipi_result {
    VOW_IPI_SUCCESS = 0,
    VOW_IPI_CLR_SMODEL_ID_NOTMATCH,
    VOW_IPI_SET_SMODEL_NO_FREE_SLOT,
} vow_ipi_result;


/* MD32/AP IPI Message ID */
#define SCP_IPI_AUDMSG_BASE 0x5F00
#define AP_IPI_AUDMSG_BASE   0x7F00
#define CLR_SPEAKER_MODEL_FLAG    0x3535

typedef enum vow_ipi_msgid_t {
    //AP to SCP MSG
    AP_IPIMSG_VOW_ENABLE = AP_IPI_AUDMSG_BASE,
    AP_IPIMSG_VOW_DISABLE,
    AP_IPIMSG_VOW_SETMODE,
    AP_IPIMSG_VOW_APREGDATA_ADDR,
    AP_IPIMSG_VOW_DATAREADY_ACK,
    AP_IPIMSG_SET_VOW_MODEL,
    AP_IPIMSG_VOW_SETGAIN,
    AP_IPIMSG_VOW_SET_FLAG,
    AP_IPIMSG_VOW_RECOGNIZE_OK_ACK,
    AP_IPIMSG_VOW_CHECKREG,

    //SCP to AP MSG
    SCP_IPIMSG_VOW_ENABLE_ACK = SCP_IPI_AUDMSG_BASE,
    SCP_IPIMSG_VOW_DISABLE_ACK,
    SCP_IPIMSG_VOW_SETMODE_ACK,
    SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK,
    SCP_IPIMSG_VOW_DATAREADY,
    SCP_IPIMSG_SET_VOW_MODEL_ACK,
    SCP_IPIMSG_VOW_SETGAIN_ACK,
    SCP_IPIMSG_SET_FLAG_ACK,
    SCP_IPIMSG_VOW_RECOGNIZE_OK,
} vow_ipi_msgid_t;

typedef enum vow_status_t {
    VOW_STATUS_IDLE = 0,
    VOW_STATUS_SCP_REG,
    VOW_STATUS_AP_REG
} vow_status_t;

typedef enum vow_mode_t {
    VOW_MODE_SCP_VOW = 0,
    VOW_MODE_VOICECOMMAND,
    VOW_MODE_MULTIPLE_KEY,
    VOW_MODE_MULTIPLE_KEY_VOICECOMMAND
} vow_mode_t;

typedef enum pmic_status_t {
    PMIC_STATUS_OFF = 0,
    PMIC_STATUS_PHASE1,
    PMIC_STATUS_PHASE2
} pmic_status_t;

typedef enum vow_flag_t {
    VOW_FLAG_DEBUG = 0,
    VOW_FLAG_PRE_LEARN,
    VOW_FLAG_DMIC_LOWPOWER,
} vow_flag_t;

typedef enum samplerate_t {
    VOW_SAMPLERATE_16K = 0,
    VOW_SAMPLERATE_32K
} samplerate_t;

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Macors
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#if VOW_LOCALSIM
#define vow_print fprintf
#else
#if VOW_DEBUG
#define vow_print dprintf
#else
#define vow_print(...)
#endif
#endif


#define ReadREG(_addr, _value) ((_value) = *(volatile unsigned int *)(_addr) )
#define WriteREG(_addr, _value) (*(volatile unsigned int *)(_addr) = (_value))
#define ReadREG16(_addr, _value) ((_value) = *(volatile unsigned short *)(_addr) )
#define WriteREG16(_addr, _value) (*(volatile unsigned short *)(_addr) = (_value))

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Extern
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
void vow_init(void);
void vow_enable(void);
void vow_disable(void);
void vow_set_flag(vow_flag_t flag, short enable);
void vow_setapreg_addr(int addr);
unsigned int vow_gettcmreg_addr(void);
void vow_setmode(vow_mode_t mode);
vow_ipi_result vow_setModel(vow_event_info_t type, int id, int addr, int size);
void vow_ipi_init(void);
int vow_ipi_sendmsg(vow_ipi_msgid_t id, void *buf, vow_ipi_result result, unsigned int size, unsigned int wait);
void vow_ipi_handler(int id, void * data, unsigned int len);
int vow_model_init(void *SModel);
int vow_keyword_recognize(void *sample_input, int num_sample, int *ret_info);
void VOW_FIFO_IRQHandler(void);
void vow_pmic_idleTophase1(void);
void vow_pmic_phase2Tophase1(void);
void vow_force_phase2(void);
void vow_pmic_idle(void);
void vow_dc_removal(char enable);
unsigned short vow_pwrapper_read(unsigned short addr);
void vow_pwrapper_write(unsigned short addr, unsigned short data);
unsigned short vow_pwrapper_read_bits(unsigned short addr, char bits, char len);
void vow_pwrapper_write_bits(unsigned short addr, unsigned short value, char bits, char len);
//void vow_dma_handler(void* test);
/*
void vow_init(const struct app_descriptor *app);
void vow_ipi_handler(int id, void * data, unsigned int len);
bool vow_ipi_sendmsg(vow_ipi_msgid_t id, void *buf, vow_ipi_result result, unsigned int size, unsigned int wait);
void vow_setmode(vow_mode_t mode);
void vow_enable();
void vow_disable();
void vow_setGain(short gain);
vow_ipi_result vow_setModel(vow_event_info_t type, int id, int addr, int size);
void vow_setapreg_addr(int addr);
void vow_set_flag(vow_flag_t flag, short enable);
enum handler_return vow_isr_mic();
void vow_dma_rx(int src_addr, int dest_addr, int len);
unsigned short vow_pwrapper_read(unsigned short addr);
*/
#endif //_VOW_SERVICE_H
