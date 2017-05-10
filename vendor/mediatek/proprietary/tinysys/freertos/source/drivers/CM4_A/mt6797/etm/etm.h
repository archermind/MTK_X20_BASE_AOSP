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

#ifndef __ETM_H__
#define __ETM_H__

/* Core Debug */
#define CoreDebug_DEMCR_TRCENA  (1 << 24)     /*!< DEMCR TRCENA enable          */
#define ITM_TCR_BUSY            (1 << 23)     /*!< ITM Busy                     */
#define ITM_TCR_TBUSID          16            /*!< ITM TraceBusID offset        */
#define ITM_TCR_TS_GLOBAL_128   (0x01 << 10)  /*!< Timestamp every 128 cycles   */
#define ITM_TCR_TS_GLOBAL_8192  (0x10 << 10)  /*!< Timestamp every 8192 cycles  */
#define ITM_TCR_TS_GLOBAL_ALL   (0x11 << 10)  /*!< Timestamp all                */
#define ITM_TCR_SWOENA          (1 << 4)      /*!< ITM Enables asynchronous-specific usage model*/
#define ITM_TCR_TXENA           (1 << 3)      /*!< ITM Enable hardware event packet
                                                    emission to the TPIU from the DWT */
#define ITM_TCR_SYNCENA         (1 << 2)      /*!< ITM Enable synchronization packet
                                                   transmission for a synchronous TPIU*/
#define ITM_TCR_TSENA           (1 << 1)      /*!< ITM Enable differential timestamps */
#define ITM_TCR_ITMENA          (1 << 0)      /*!< ITM enable                   */
#define ITM_TER_STIM0           (1 << 0)      /*!< ITM stimulus0 enable         */
#define ITM_TER_STIM1           (1 << 1)      /*!< ITM stimulus1 enable         */
#define ITM_TER_STIM2           (1 << 2)      /*!< ITM stimulus2 enable         */
#define DWT_CTRL_CYCEVTENA      (1 << 22)     /*!< DWT Periodic packet enable   */
#define DWT_CTRL_FOLDEVTENA     (1 << 21)     /*!< DWT fold event enable        */
#define DWT_CTRL_LSUEVTENA      (1 << 20)     /*!< DWT lsu event enable         */
#define DWT_CTRL_SLEEPEVTENA    (1 << 19)     /*!< DWT sleep event enabel       */
#define DWT_CTRL_EXCEVTENA      (1 << 18)     /*!< DWT exception event enable   */
#define DWT_CTRL_CPIEVTENA      (1 << 17)     /*!< DWT CPI event enable         */
#define DWT_CTRL_EXCTRCENA      (1 << 16)     /*!< DWT Exception trace enable   */
#define DWT_CTRL_PCSAMPLENA     (1 << 12)     /*!< DWT Periodic event PC select */
#define DWT_CTRL_SYNCTAP24      (1 << 10)     /*!< DWT Synch packet tap at 24   */
#define DWT_CTRL_SYNCTAP26      (2 << 10)     /*!< DWT Synch packet tap at 26   */
#define DWT_CTRL_SYNCTAP28      (3 << 10)     /*!< DWT Synch packet tap at 28   */
#define DWT_CTRL_CYCTAP         (1 << 9)      /*!< DWT POSTCNT tap select       */
#define DWT_CTRL_POSTPRESET_BITS     1        /*!< DWT POSTCNT reload offset    */
#ifndef DWT_CTRL_CYCCNTENA
    #define DWT_CTRL_CYCCNTENA           1        /*!< DWT Cycle counter enable   */
#endif
/* DWT Function, EMITRANGE=0, CYCMATCH = 0 */
#define DWT_FUNC_SAMP_PC        0x1           /*!< DWT Func: PC Sample Packet   */
#define DWT_FUNC_SAMP_DATA      0x2           /*!< DWT Func: Data Value Packet  */
#define DWT_FUNC_SAMP_PC_DATA   0x3           /*!< DWT Func: PC and Data Packets*/
#define DWT_FUNC_PC_WPT         0x4           /*!< DWT Func: PC Watchpoint      */
#define DWT_FUNC_TRIG_PC        0x8           /*!< DWT Func: PC to CMPMATCH(ETM)*/
#define DWT_FUNC_TRIG_RD        0x9           /*!< DWT Func: Daddr(R)to CMPMATCH*/
#define DWT_FUNC_TRIG_WR        0xA           /*!< DWT Func: Daddr(W)to CMPMATCH*/
#define DWT_FUNC_TRIG_RW        0xB           /*!< DWT Func: Daddr to CMPMATCH  */

#define DWT_CTRL_POSTPRESET_10  0xA

/* ETM */
#define ETM_CR_PWRDN                1         /*!< ETM Power Down                */
#define ETM_CR_STALLPROC        (1 << 7)      /*!< ETM Stall Processor           */
#define ETM_CR_BRANCH_OUTPUT    (1 << 8)      /*!< ETM Branch Broadcast          */
#define ETM_CR_DEBUG            (1 << 9)      /*!< ETM Debug Request control     */
#define ETM_CR_PROGBIT          (1 << 10)     /*!< ETM ProgBit                   */
#define ETM_CR_ETMEN            (1 << 11)     /*!< ETM ProgBit                   */
#define ETM_CR_TSEN             (1 << 28)     /*!< ETM Timestamp Enable          */
#define ETM_SR_OVERFLOW             1         /*!< ETM Overflow Status           */
#define ETM_SR_PROGBIT          (1 << 1)      /*!< ETM Progbit Status            */
#define ETM_SR_SSTOP            (1 << 2)      /*!< ETM Start/Stop Status         */
#define ETM_SR_TRIGGER          (1 << 3)      /*!< ETM Triggered Status          */
#define ETM_SCR_FIFOFULL        (1 << 8)      /*!< ETM System FIFOFILL Support   */
#define ETM_EVT_A                   0         /*!< ETM Event A offset            */
#define ETM_EVT_NOTA            (1 << 14)     /*!< ETM Event fn Not(A)           */
#define ETM_EVT_AANDB           (2 << 14)     /*!< ETM Event fn (A)and(B)        */
#define ETM_EVT_NOTAANDB        (3 << 14)     /*!< ETM Event fn (A)and(not(B))   */
#define ETM_EVT_NOTAANDNOTB     (4 << 14)     /*!< ETM Event fn (not(A))and(not(B))*/
#define ETM_EVT_AORB            (5 << 14)     /*!< ETM Event fn (A)or(B)         */
#define ETM_EVT_NOTAORB         (6 << 14)     /*!< ETM Event fn (not(A))or(B)    */
#define ETM_EVT_NOTAORNOTB      (7 << 14)     /*!< ETM Event fn (not(A))or(not(B))*/
#define ETM_EVT_RESB                   7     /*!< ETM Event B offset            */
#define ETM_EVT_DWT0               0x20      /*!< ETM Event select DWT0         */
#define ETM_EVT_DWT1               0x21      /*!< ETM Event select DWT1         */
#define ETM_EVT_DWT2               0x22      /*!< ETM Event select DWT2         */
#define ETM_EVT_DWT3               0x23      /*!< ETM Event select DWT3         */
#define ETM_EVT_COUNT1             0x40      /*!< ETM Event select Counter1 at zero */
#define ETM_EVT_SSTOP              0x5F      /*!< ETM Event select Start/Stop   */
#define ETM_EVT_EXTIN0             0x60      /*!< ETM Event select ExtIn0       */
#define ETM_EVT_EXTIN1             0x61      /*!< ETM Event select ExtIn1       */
#define ETM_EVT_TRUE               0x6F      /*!< ETM Event select Always True  */
#define ETM_TECR1_USE_SS           (1 << 25) /*!< ETM TraceEnable Start/Stop enable */
#define CS_UNLOCK                  0xC5ACCE55UL

#define ETM_TESSEICR_ICE0          0x1       /*!< ETM Start/Stop from DWT0      */
#define ETM_TESSEICR_ICE1          0x2       /*!< ETM Start/Stop from DWT1      */
#define ETM_TESSEICR_ICE2          0x4       /*!< ETM Start/Stop from DWT2      */
#define ETM_TESSEICR_ICE3          0x8       /*!< ETM Start/Stop from DWT3      */

#define ETM_TESSEICR_STOP          16        /*!< ETM Start/Stop Stop offset    */

/* TPIU */
#define TPIU_PIN_TRACEPORT         0       /*!< TPIU Selected Pin Protocol Trace Port */
#define TPIU_PIN_MANCHESTER        1       /*!< TPIU Selected Pin Protocol Manchester */
#define TPIU_PIN_NRZ               2       /*!< TPIU Selected Pin Protocol NRZ (uart) */


/* memory mapping structure for ETM */
typedef struct
{
  __IO uint32_t CR;                          /*!< ETM Control Register                      */
  __I  uint32_t CCR;                         /*!< ETM Configuration Code Register           */
  __IO uint32_t TRIGGER;                     /*!< ETM Trigger Event Register                */
       uint32_t RESERVED0;
  __IO uint32_t SR;                          /*!< ETM Status Register                       */
  __I  uint32_t SCR;                         /*!< ETM System Configuration Register         */
       uint32_t RESERVED1[2];
  __IO uint32_t TEEVR;                       /*!< ETM Trace Enable Event Register           */
  __IO uint32_t TECR1;                       /*!< ETM Trace Enable Control 1 Register       */
       uint32_t RESERVED2X;
  __IO uint32_t FFLR;                        /*!< ETM Fifo Full Level Register              */
       uint32_t RESERVED2[68];
  __IO uint32_t CNTRLDVR1;                   /*!< ETM Counter1 Reload Register              */
       uint32_t RESERVED3[39];
  __I  uint32_t SYNCFR;                      /*!< ETM Sync Frequency Register               */
  __I  uint32_t IDR;                         /*!< ETM ID Register                           */
  __I  uint32_t CCER;                        /*!< ETM Configuration Code Extention Register */
       uint32_t RESERVED4;
  __IO uint32_t TESSEICR;                    /*!< ETM Trace Enable Start Stop EICE Register */
       uint32_t RESERVED5;
  __IO uint32_t TSEVR;                       /*!< ETM Timestamp Event Register              */
       uint32_t RESERVED6;
  __IO uint32_t TRACEIDR;                    /*!< ETM Trace ID Register                     */
       uint32_t RESERVED7[68];
  __I  uint32_t PDSR;                        /*!< ETM Power Down Status Register            */
       uint32_t RESERVED8[754];
  __I  uint32_t ITMISCIN;                    /*!< ETM Integration Misc In Register          */
       uint32_t RESERVED9;
  __IO uint32_t ITTRIGOUT;                   /*!< ETM Integration Trigger Register          */
       uint32_t RESERVED10;
  __I  uint32_t ITATBCR2;                    /*!< ETM Integration ATB2 Register             */
       uint32_t RESERVED11;
  __IO uint32_t ITATBCR0;                    /*!< ETM Integration ATB0 Register             */
       uint32_t RESERVED12;
  __IO uint32_t ITCTRL;                      /*!< ETM Integration Mode Control Register     */
       uint32_t RESERVED13[39];
  __IO uint32_t CLAIMSET;                    /*!< ETM Claim Set Register                    */
  __IO uint32_t CLAIMCLR;                    /*!< ETM Claim Clear Register                  */
       uint32_t RESERVED14[2];
  __IO uint32_t LAR;                         /*!< ETM Lock Access Register                  */
  __I  uint32_t LSR;                         /*!< ETM Lock Status Register                  */
  __I  uint32_t AUTHSTATUS;                  /*!< ETM Authentication Status Register        */
       uint32_t RESERVED15[4];
  __I  uint32_t DEVTYPE;                     /*!< ETM Device Type Register                  */
  __I  uint32_t PID4;                        /*!< CoreSight register  */
  __I  uint32_t PID5;                        /*!< CoreSight register  */
  __I  uint32_t PID6;                        /*!< CoreSight register  */
  __I  uint32_t PID7;                        /*!< CoreSight register  */
  __I  uint32_t PID0;                        /*!< CoreSight register  */
  __I  uint32_t PID1;                        /*!< CoreSight register  */
  __I  uint32_t PID2;                        /*!< CoreSight register  */
  __I  uint32_t PID3;                        /*!< CoreSight register  */
  __I  uint32_t CID0;                        /*!< CoreSight register  */
  __I  uint32_t CID1;                        /*!< CoreSight register  */
  __I  uint32_t CID2;                        /*!< CoreSight register  */
  __I  uint32_t CID3;                        /*!< CoreSight register  */
} ETM_Type;

/* Memory mapping of Cortex-M4 Hardware */

#define ETM_BASE            (0xE0041000)                              /*!< ETM Base Address                     */
#define ETM                 ((ETM_Type *)           ETM_BASE)         /*!< ETM configuration struct             */
#define CoreDebug           ((CoreDebug_Type *)     CoreDebug_BASE)   /*!< Core Debug configuration struct      */

#endif/*!__ETM_H__*/
