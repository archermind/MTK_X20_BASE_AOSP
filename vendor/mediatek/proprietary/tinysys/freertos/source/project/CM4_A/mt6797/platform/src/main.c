/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <tinysys_config.h>

#include <scp_ipi.h>
#ifdef CFG_LOGGER_SUPPORT
#include <scp_logger.h>
#endif
#include <driver_api.h>
#include <mt_uart.h>
/* timestamp */
#include <utils.h>
#include <hal_i2c.h>
/*  console includes. */
#ifdef CFG_TESTSUITE_SUPPORT
#include <console.h>
#endif
#ifdef CFG_WDT_SUPPORT
#include "wdt.h"
#endif
#ifdef CFG_EINT_SUPPORT
#include <eint.h>
#endif
#ifdef CFG_VCORE_DVFS_SUPPORT
#include <vcore_dvfs.h>
#endif
#ifdef CFG_DMA_SUPPORT
#include <dma.h>
#endif
#include <unwind.h> // to support pc back trace dump
#ifdef CFG_CCCI_SUPPORT
extern void ccci_init(void);
#endif
#ifdef CFG_XGPT_SUPPORT
#include "mt_gpt.h"
#endif
#ifdef CFG_DWT_SUPPORT
#include "dwt.h"
#endif
#ifdef CFG_MPU_DEBUG_SUPPORT
#include "mpu.h"
#endif

/* Private functions ---------------------------------------------------------*/
static void prvSetupHardware(void);
/* Extern functions ---------------------------------------------------------*/
extern void platform_init(void);
extern void project_init(void);
extern void mt_init_dramc(void);
extern void scp_remap_init(void);
extern void mt_ram_dump_init(void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{

    unsigned char buf[] = "ready";

    /* init timestamp */
    timestamp_init();

    /* Configure the hardware, after this point, the UART can be used*/
    prvSetupHardware();

    /* start to use UART */
    printf("\n\r FreeRTOS %s(build:%s %s) \n\r", tskKERNEL_VERSION_NUMBER, __DATE__, __TIME__);
    platform_init();
    project_init();

    printf("send ready IPI \n");
#ifdef CFG_CTP_SUPPORT
    /* do not wait here */
    scp_ipi_send(IPI_SCP_READY, buf, sizeof(buf), 0, IPI_SCP2AP);
#else
    while (scp_ipi_send(IPI_SCP_READY, buf, sizeof(buf), 1, IPI_SCP2AP) != DONE);
#endif

#ifdef CFG_RAMDUMP_SUPPORT
    /*init ram dump*/
    mt_ram_dump_init();
#endif

#ifdef CFG_TESTSUITE_SUPPORT
    PRINTF_E("support TESTSUITE\n");
    console_start();
#endif

#ifdef CFG_WDT_SUPPORT
    /*put wdt init just before insterrupt enabled in vTaskStartScheduler()*/
    mtk_wdt_init();
#endif
    PRINTF_E("Scheduler start...\n");
    /* Start the scheduler. After this point, the interrupt is enabled*/
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for (;;);
}
/*-----------------------------------------------------------*/
/**
  * @brief  Hardware init
  * @param  None
  * @retval None
  */
static void prvSetupHardware(void)
{
    /* CIRQ enable all interrrupt from peripheral to CM4 */
    DRV_WriteReg32(0x400A2004, 0x03FFFFFF);
    /* set up debug level */
    set_debug_level(LOG_DEBUG);
    /* init UART before any printf function all */
    uart_init(UART_LOG_PORT, UART_LOG_BAUD_RATE);
    DRV_SetReg32(0xA0001A04, 0x1);
    PRINTF_D("0xA0001A04:0x%x\n", DRV_Reg32(0xA0001A04));
    /* init remap register before any dram access*/
    scp_remap_init();
    /* init IPI before any IPI all */
    scp_ipi_init();

#ifdef CFG_VCORE_DVFS_SUPPORT
    /* enable plaform wakeup source */
    scp_wakeup_src_setup(IPC0_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(IPC1_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(IPC2_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(IPC3_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(CIRQ_WAKE_CLK_CTRL, 0);
    scp_wakeup_src_setup(EINT_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(DMA_WAKE_CLK_CTRL, 0);
#endif

#ifdef CFG_LOGGER_SUPPORT
    /* init logger */
    send_log_info();
#endif
#ifdef CFG_MPU_DEBUG_SUPPORT
    /*enable mpu to protect dram*/
    dram_protector_init();
#endif
#ifdef CFG_DMA_SUPPORT
    /* init DMA */
    mt_init_dma();
#endif
    /* init I2C */
    i2c_hw_init();
#ifdef CFG_EINT_SUPPORT
    /* init EINT */
    mt_eint_init();
#endif
    /* gating auto save  */
    mt_init_dramc();

#ifdef CFG_VCORE_DVFS_SUPPORT
    dvfs_init();
#endif
#ifdef CFG_CCCI_SUPPORT
    ccci_init();
#endif
#ifdef CFG_XGPT_SUPPORT
    mt_platform_timer_init();
#endif

#ifdef CFG_DWT_SUPPORT
    /* init data watchpoint & trace */
    dwt_init();
#endif
    /*disable scp2spm apb timeout*/
    DRV_ClrReg32(0x400A0070, 0xC);
    PRINTF_D("0x400A0070:0x%x\n", DRV_Reg32(0x400A0070));
}

static uint32_t last_pc = 0x0;
static uint32_t current_pc = 0x0;
_Unwind_Reason_Code trace_func(_Unwind_Context *ctx, void *d)
{
    current_pc = _Unwind_GetIP(ctx);
    if (current_pc != last_pc) {
        PRINTF_E("0x%08x\n", current_pc);
        last_pc = current_pc;
        return _URC_NO_REASON;
    } else {
        PRINTF_E("===PC backtrace dump end===\n");
        taskDISABLE_INTERRUPTS();
        mtk_wdt_set_time_out_value(10);/*assign a small value to make ee sooner*/
        mtk_wdt_restart();
        while (1);
    }
}
void print_backtrace(void)
{
    int depth = 0;

    _Unwind_Backtrace(&trace_func, &depth);
}
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void vAssertCalled(char* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    PRINTF_E("Wrong parameters value: file %s on line %lu\r\n", file, line);
    PRINTF_E("==PC backtrace dump start==\n");
    print_backtrace();
    PRINTF_E("==PC backtrace dump end==\n");
    taskDISABLE_INTERRUPTS();
    mtk_wdt_set_time_out_value(10);/*assign a small value to make ee sooner*/
    mtk_wdt_restart();
    /* Infinite loop */
    while (1) {
    }
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, char *pcTaskName)
{
    /* If configCHECK_FOR_STACK_OVERFLOW is set to either 1 or 2 then this
    function will automatically get called if a task overflows its stack. */
    (void) pxTask;
    (void) pcTaskName;
    PRINTF_E("\n\r task:%s stack overflow \n\r", pcTaskName);
    for (;;);
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* If configUSE_MALLOC_FAILED_HOOK is set to 1 then this function will
    be called automatically if a call to pvPortMalloc() fails.  pvPortMalloc()
    is called automatically when a task, queue or semaphore is created. */
    PRINTF_E("\n\r malloc failed\n\r");
    PRINTF_D("Heap: free size/total size:%d/%d\n",xPortGetFreeHeapSize(),configTOTAL_HEAP_SIZE);
    configASSERT(0);
    for (;;);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
//  printf("\n\r Enter IDLE \n\r");
}

void vApplicationTickHook(void)
{
    //printf("\n\r Enter TickHook \n\r");
}
