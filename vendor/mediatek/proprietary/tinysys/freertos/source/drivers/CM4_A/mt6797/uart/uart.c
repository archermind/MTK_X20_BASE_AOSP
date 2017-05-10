/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "main.h"
/*  Kernel includes. */
#include "FreeRTOS.h"
#include <driver_api.h>
#include <mt_uart.h>
#include <tinysys_config.h>
#include <vcore_dvfs.h>
#ifdef CFG_LOGGER_SUPPORT
#include <scp_logger.h>
#endif

uint8_t RxByte;         /*Byte de recepcion temporal*/
#ifdef NO_USE_UART
void WriteDebugByte(unsigned int ch)
{
}
#else
void WriteDebugByte(unsigned int ch)
{
    unsigned int LSR;

#ifdef USING_APUART_PORT
    /* when msg is print out via AP uart
     * if no infra clock, do nothing*/
    if (check_infra_state() == 0) {
        return;
    }
#endif
    while (1) {
        LSR = INREG32(UART_LSR(UART_LOG_PORT));
        if (LSR & UART_LSR_THRE) {
            OUTREG32(UART_THR(UART_LOG_PORT), (unsigned int)ch);
            break;
        }
    }
}
#endif


#ifdef NO_USE_UART
int ReadDebugByte(void)
{
    return 0;
}
#else
int ReadDebugByte(void)
{
    unsigned int ch;
    unsigned int LSR;

#ifdef USING_APUART_PORT
    /* when msg is print out via AP uart
     * if no infra clock, do nothing*/
    if (check_infra_state() == 0) {
        return DEBUG_SERIAL_READ_NODATA;
    }
#endif
    LSR = INREG32(UART_LSR(UART_LOG_PORT));
    if (LSR & UART_LSR_DR) {
        ch = (unsigned int)INREG32(UART_RBR(UART_LOG_PORT));
    } else {
        ch = DEBUG_SERIAL_READ_NODATA;
    }

    return (int)ch;
}
#endif

/*-----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */
/*-----------------------------------------------------------*/

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE {
    WriteDebugByte(ch);
    return ch;
}

/**
  * @brief  Retargets the C library getc function to the USART.
  * @param  None
  * @retval None
  */
GETCHAR_PROTOTYPE {
    RxByte = ReadDebugByte();
    return RxByte;
}
#ifdef NO_USE_UART
void uart_init(unsigned int g_uart, unsigned int speed)
{

}
#else
void uart_init(unsigned int g_uart, unsigned int speed)
{
    unsigned int byte;
    unsigned int highspeed;
    unsigned int quot, divisor, remainder;
    unsigned int uartclk;
    unsigned short data, high_speed_div, sample_count, sample_point;
    unsigned int tmp_div;

#ifdef USING_APUART_PORT
    UART_SET_BITS(CLK_AP_CG_CTRL_CLR, CLK_AP_UART1_CLK);
#else
#ifdef CONFIG_UART_PIN_MODE
    // DRV_WriteReg32(0xA00054D0,(DRV_Reg32(0xA00054D0)&0xFF0FF0FF)|0x00200200); //pin234 237
    DRV_WriteReg32(0xA00054F0, (DRV_Reg32(0xA00054F0) & 0xFFFF00FF) | 0x00006600); //pin250 251
#endif
    UART_SET_BITS(CLK_CG_CTRL, UART0_CLK | UART1_CLK | UART0_RST | UART1_RST); /* enable clock and release reset*/
#endif
    //OUTREG32(CLK_CG_CTRL, 0x5FF); /* FIXME */
    UART_SET_BITS(UART_FCR(g_uart), UART_FCR_FIFO_INIT); /* clear fifo */
    OUTREG32(UART_LCR(g_uart), UART_NONE_PARITY | UART_WLS_8 | UART_1_STOP);

    uartclk = UART_SRC_CLK;
    if (speed <= 115200) {
        highspeed = 0;
        quot = 16;
    } else {
        highspeed = 3;
        quot = 1;
    }

    if (highspeed < 3) { /*0~2*/
        /* Set divisor DLL and DLH    */
        divisor   =  uartclk / (quot * speed);
        remainder =  uartclk % (quot * speed);

        if (remainder >= (quot / 2) * speed)
            divisor += 1;

        OUTREG32(UART_HIGHSPEED(g_uart), highspeed);
        byte = INREG32(UART_LCR(g_uart));     /* DLAB start */
        OUTREG32(UART_LCR(g_uart), (byte | UART_LCR_DLAB));
        OUTREG32(UART_DLL(g_uart), (divisor & 0x00ff));
        OUTREG32(UART_DLH(g_uart), ((divisor >> 8) & 0x00ff));
        OUTREG32(UART_LCR(g_uart), byte);      /* DLAB end */
    } else {
        data = (unsigned short)(uartclk / speed);
        high_speed_div = (data >> 8) + 1; // divided by 256

        tmp_div = uartclk / (speed * high_speed_div);
        divisor = (unsigned short)tmp_div;

        remainder = (uartclk) % (high_speed_div * speed);
        /*get (sample_count+1)*/
        if (remainder >= ((speed) * (high_speed_div)) >> 1)
            divisor = (unsigned short)(tmp_div + 1);
        else
            divisor = (unsigned short)tmp_div;

        sample_count = divisor - 1;

        /*get the sample point*/
        sample_point = (sample_count - 1) >> 1;

        /*configure register*/
        OUTREG32(UART_HIGHSPEED(g_uart), highspeed);

        byte = INREG32(UART_LCR(g_uart));      /* DLAB start */
        OUTREG32(UART_LCR(g_uart), (byte | UART_LCR_DLAB));
        OUTREG32(UART_DLL(g_uart), (high_speed_div & 0x00ff));
        OUTREG32(UART_DLH(g_uart), ((high_speed_div >> 8) & 0x00ff));
        OUTREG32(UART_SAMPLE_COUNT(g_uart), sample_count);
        OUTREG32(UART_SAMPLE_POINT(g_uart), sample_point);
        OUTREG32(UART_LCR(g_uart), byte);      /* DLAB end */
    }
}
#endif
