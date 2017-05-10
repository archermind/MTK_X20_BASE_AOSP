/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 *
 * revised history:
 * 2015/06/26 (V0): Initial version (mtk09997)
 *
 */
#include "FreeRTOS.h"
#include "task.h"
#include "mtk_HeapGuard.h"

/* Define the linked list structure. This is used to link in-use blocks in order or their
	memory address
	Memory layout of each heap block
	---------------------------------
	FreeBlockLink_t
	---------------------------------
	User data
	---------------------------------
	InUseLink_t
	---------------------------------
*/

/* Defintion of linked list structure of real heap function,
   should sync with heapx_.c */
/* pNextFreeBlock will be head stamp here */
typedef struct BLOCK_LINK {
    struct BLOCK_LINK *pNextFreeBlock;
    size_t xBlockSize;
} FreeBlockLink_t;

/* Inuse heap block header struction definition */
typedef struct INUSE_LINK {
    UBaseType_t xFootStamp;
    FreeBlockLink_t *pNextInuseBlock;
    FreeBlockLink_t *pPrevInuseBlock;
    TaskHandle_t xTaskHandle;
    void *pvCallerReturnAddr;
} InUseLink_t;

/* (start/end) to maintain a inuse block linked list */
static FreeBlockLink_t *gpInuseStart = NULL;
static FreeBlockLink_t *gpInuseEnd = NULL;

/* The size of the structure placed at the beginning/end of each allocated memory
block must by correctly byte aligned. */
static const size_t gxInuseHeapHeaderSize =
    ((sizeof(InUseLink_t) +
      (portBYTE_ALIGNMENT - 1)) & ~portBYTE_ALIGNMENT_MASK);
static const size_t gxFreeBlockHeader =
    ((sizeof(FreeBlockLink_t) +
      (portBYTE_ALIGNMENT - 1)) & ~portBYTE_ALIGNMENT_MASK);

/*------------------------------------------------------------------------------*/
/* Data and macro definition */

/* NOTE!!! NOTE !!! NOTE !!! */
/* This macro should be sync with heap_x.c */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )
static size_t xBlockAllocatedBit = 0;
/* Real target functions which we wrap, linker will add prefix __real to target functions */
void *__real_pvPortMalloc(size_t xwanted_size);
void __real_vPortFree(void *pv);

#define hg_HEAD_STAMP_MAGIC	(0x3F3F3F3F)
#define hg_FOOT_STAMP_MAGIC	(0x4F4F4F4F)

/* Get FreeBlockLink_t address by user address translation */
#define hg_USERADDR_TO_FREELINK(UsrAddr) \
  ((FreeBlockLink_t *) (((uint8_t *) UsrAddr) - gxFreeBlockHeader))
/* Get user address by FreeBlockLink_t translation */
#define hg_FREELINK_TO_USERADDR(freelinkptr) \
  (void *)(((uint8_t *) freelinkptr) +	\
                                 gxFreeBlockHeader)
/* Set inuse heap block head stamp */
#define hg_SET_HEADSTAMP(pfree_block_link) {\
	pfree_block_link->pNextFreeBlock = (FreeBlockLink_t *)((uint32_t)pfree_block_link ^ hg_HEAD_STAMP_MAGIC); \
}
/* Get block size */
#define hg_GET_BLOCK_SIZE(pfree_block_link) (pfree_block_link->xBlockSize & ~(xBlockAllocatedBit))
/* Get user block size */
#define hg_GET_USER_BLOCK_SIZE(freelinkptr) \
	(hg_GET_BLOCK_SIZE(freelinkptr) -gxInuseHeapHeaderSize - gxFreeBlockHeader )

/* Get pointer of inuse heap block foot stamp */
#define hg_GET_INUSE_HEADER_ADDR(pfree_block_link) \
 	 ((InUseLink_t *) ((uint8_t *) pfree_block_link + \
      (pfree_block_link->xBlockSize & ~(xBlockAllocatedBit))- \
      gxInuseHeapHeaderSize))
/* Get pointer of inuse heap block by extra record xBlockSize */
#define hg_GET_INUSE_BkSize(pfree_block_link, BlkSize) \
 	 ((InUseLink_t *) ((uint8_t *) pfree_block_link + \
      (BlkSize & ~(xBlockAllocatedBit))- \
      gxInuseHeapHeaderSize))
/* Get next free block link list node */
#define hg_GET_NEXT_LINK(pfreeblockptr) \
	((hg_GET_INUSE_HEADER_ADDR(pfreeblockptr))->pNextInuseBlock)
/* Get previous free block link list node */
#define hg_GET_PREV_LINK(pfreeblockptr) \
	((hg_GET_INUSE_HEADER_ADDR(pfreeblockptr))->pPrevInuseBlock)
/* Set inuse heap block foot stamp */
#define hg_SET_INUSE_FOOTSTAMP(pFreelinkptr) {\
	hg_GET_INUSE_HEADER_ADDR(pFreelinkptr)->xFootStamp = ((uint32_t)hg_GET_INUSE_HEADER_ADDR(pFreelinkptr) ^ hg_FOOT_STAMP_MAGIC); \
}
/* Get head stamp */
#define hg_GET_HEAD_STAMP(pfree_blk_link)	(pfree_blk_link->pNextFreeBlock)
/* Get foot stamp */
#define hg_GET_FOOT_STAMP(pfree_blk_link)	(hg_GET_INUSE_HEADER_ADDR(pfree_blk_link)->xFootStamp)
/* Get TCB */
#define hg_GET_TASK_HANDLE(pfree_blk_link)	(hg_GET_INUSE_HEADER_ADDR(pfree_blk_link)->xTaskHandle)
/* Set TCB */
#define hg_SET_TASK_HANDLE(pfree_blk_link, handle) {\
	hg_GET_INUSE_HEADER_ADDR(pfree_blk_link)->xTaskHandle = (TaskHandle_t)handle; \
}
/* Check head stamp */
#define hg_CHECK_HEAD_STAMP(pfree_block_link) \
	 (hg_GET_HEAD_STAMP(pfree_block_link) == (FreeBlockLink_t *)((uint32_t)pfree_block_link ^ hg_HEAD_STAMP_MAGIC))
/* Check foot stamp */
#define hg_CHECK_FOOT_STAMP(pInusePtr) \
	(pInusePtr->xFootStamp == ((uint32_t)pInusePtr ^ hg_FOOT_STAMP_MAGIC))
/* Set caller return address, for addrtoline routine to discover caller related information */
#define hg_SET_CALLER_ADDR(pFreelinkptr, link_reg_addr) { \
	hg_GET_INUSE_HEADER_ADDR(pFreelinkptr)->pvCallerReturnAddr = (void *) link_reg_addr; \
}
/* Get caller return address */
#define hg_GET_CALLER_ADDR(pFreelinkptr) \
	(hg_GET_INUSE_HEADER_ADDR(pFreelinkptr)->pvCallerReturnAddr)

/* Heap block monitor related defintion */
/* For specified heap debug porpose */
#define MAX_HEAP_MONITOR_NUM	(8)
/* global heap instance counts for heap monitor */
uint32_t gHPMonCnt;

typedef struct HEAP_MONITOR {
    FreeBlockLink_t *pHeapAddr;
    size_t xBkSize;
    TaskHandle_t xTaskHandle;
    void *pvCallerAddr;
    TaskHandle_t xPrevTask;
    void *pvPrevCallerAddr;
} HeapMonitor_t;

HeapMonitor_t gHPMon[MAX_HEAP_MONITOR_NUM];

/* ----------------------------------------------------------------------------*/
/* Private functions */

/* Insert inuse heap block */

static void prvInsertBlockIntoInUsedList(FreeBlockLink_t *
                                         pxBlockToInsert);
static void prvRemoveBlockFromInUsedList(FreeBlockLink_t * pBlockToRemove);
static mtk_HeapGuard_cb_func pfnHeapGuardCallBack = NULL;
/* Callback function when memory alloc, free and some error exceptions */
#define HG_Callback( HGInfo, UserAddr, BlkSize, xTCB, CallerAddr, xprevTCB, prevCallerAddr, Status) {	\
	if (pfnHeapGuardCallBack != NULL) {	\
		HGInfo.pUserAddr = UserAddr;	\
		HGInfo.xBlockSize = BlkSize;	\
		HGInfo.pxTCB = xTCB;			\
		HGInfo.pvCallerAddr = CallerAddr;	\
		HGInfo.pvPrevCallerAddr = prevCallerAddr;	\
		HGInfo.pxPrevTCB = xprevTCB;	\
		HGInfo.status = Status;	\
		pfnHeapGuardCallBack(&HGInfo); \
	}\
}

/* Insert a block of memory that is being used */
static void prvInsertBlockIntoInUsedList(FreeBlockLink_t * pBlockToInsert)
{
    FreeBlockLink_t *pBlk;
    InUseLink_t *pInuseLink;
    if (gpInuseStart == NULL) {
        /* The initial first node in linked list */
        gpInuseStart = gpInuseEnd = pBlockToInsert;
        pInuseLink = hg_GET_INUSE_HEADER_ADDR(pBlockToInsert);
        pInuseLink->pNextInuseBlock = pInuseLink->pPrevInuseBlock = NULL;
    } else {
        for (pBlk = gpInuseStart; pBlk; pBlk = hg_GET_NEXT_LINK(pBlk)) {
            if (pBlk > pBlockToInsert) {
                if (hg_GET_PREV_LINK(pBlk) == NULL) {
                    /* In the first node */
                    hg_GET_NEXT_LINK(pBlockToInsert) = pBlk;
                    hg_GET_PREV_LINK(pBlk) = pBlockToInsert;
                    hg_GET_PREV_LINK(pBlockToInsert) = NULL;
                    gpInuseStart = pBlockToInsert;
                } else {
                    hg_GET_NEXT_LINK(hg_GET_PREV_LINK(pBlk)) =
                        pBlockToInsert;
                    hg_GET_NEXT_LINK(pBlockToInsert) = pBlk;
                    hg_GET_PREV_LINK(pBlockToInsert) =
                        hg_GET_PREV_LINK(pBlk);
                    hg_GET_PREV_LINK(pBlk) = pBlockToInsert;
                }
                break;
            }
        }
        /* The last node in link list */
        if (pBlk == NULL) {
            hg_GET_PREV_LINK(pBlockToInsert) = gpInuseEnd;
            hg_GET_NEXT_LINK(pBlockToInsert) = NULL;
            hg_GET_NEXT_LINK(gpInuseEnd) = pBlockToInsert;
            gpInuseEnd = pBlockToInsert;
        }
    }
}

/* Remove a block of memory from inuse linked list*/
static void prvRemoveBlockFromInUsedList(FreeBlockLink_t * pBlockToRemove)
{
    FreeBlockLink_t *pBlk;
    for (pBlk = gpInuseStart; pBlk; pBlk = hg_GET_NEXT_LINK(pBlk)) {
        if (pBlk == pBlockToRemove) {
            if (hg_GET_PREV_LINK(pBlk) == NULL) {
                /* First node in linked list */
                gpInuseStart = hg_GET_NEXT_LINK(pBlk);
                hg_GET_PREV_LINK(gpInuseStart) = NULL;
                hg_GET_NEXT_LINK(pBlk) = NULL;
            } else if (hg_GET_NEXT_LINK(pBlk) == NULL) {
                /* last node in linked list */
                gpInuseEnd = hg_GET_PREV_LINK(pBlk);
                hg_GET_NEXT_LINK(gpInuseEnd) = NULL;
                hg_GET_PREV_LINK(pBlk) = NULL;
            } else {
                hg_GET_NEXT_LINK(hg_GET_PREV_LINK(pBlk)) =
                    hg_GET_NEXT_LINK(pBlk);
                hg_GET_PREV_LINK(hg_GET_NEXT_LINK(pBlk)) =
                    hg_GET_PREV_LINK(pBlk);
                hg_GET_NEXT_LINK(pBlk) = NULL;
                hg_GET_PREV_LINK(pBlk) = NULL;
            }
        }
    }
}

/* Heap Guard error handler help to dump heap block information */
static BaseType_t prvErrorHandler(FreeBlockLink_t * pBlk,
                                  uint32_t u32Status)
{
    BaseType_t nRet = pdFALSE;
    uint32_t i;
    HeapGuardCallBackInfo_t CallbackInfo;
    // FreeBlockLink_t* pPrevBlk;

    u32Status &= (HG_PROCESS_MASK);
    for (i = 0; i < gHPMonCnt; i++) {
        if (gHPMon[i].pHeapAddr == pBlk) {
            u32Status |= HG_STATUS_RECOVER;
            HG_Callback(CallbackInfo,
                        hg_FREELINK_TO_USERADDR(pBlk),
                        gHPMon[i].xBkSize,
                        gHPMon[i].xTaskHandle,
                        gHPMon[i].pvCallerAddr,
                        gHPMon[i].xPrevTask,
                        gHPMon[i].pvPrevCallerAddr, (u32Status));

            nRet = pdTRUE;
            break;
        }
    }
    if (nRet == pdFALSE) {
        u32Status |=
            (HG_HEAD_STAMP_ERROR | HG_FOOT_STAMP_ERROR | HG_STATUS_ERROR);
        HG_Callback(CallbackInfo,
                    hg_FREELINK_TO_USERADDR(pBlk),
                    0, NULL, NULL, NULL, NULL, (u32Status));
    }
    return nRet;
}

/* Involve callback function to dump heap block information */
static void prvDumpHeapBlock(FreeBlockLink_t * pBlk)
{
    HeapGuardCallBackInfo_t CallbackInfo;
    FreeBlockLink_t *pPrevLink;

    if (hg_CHECK_HEAD_STAMP(pBlk)) {
        if (hg_CHECK_FOOT_STAMP(hg_GET_INUSE_HEADER_ADDR(pBlk))) {
            pPrevLink = hg_GET_PREV_LINK(pBlk);
            HG_Callback(CallbackInfo,
                        hg_FREELINK_TO_USERADDR(pBlk),
                        hg_GET_USER_BLOCK_SIZE(pBlk),
                        hg_GET_TASK_HANDLE(pBlk),
                        hg_GET_CALLER_ADDR(pBlk),
                        pPrevLink ==
                        NULL ? NULL : hg_GET_TASK_HANDLE(pPrevLink),
                        // hg_GET_TASK_HANDLE(pPrevLink),
                        hg_GET_CALLER_ADDR(pPrevLink),
                        (HG_TRAVERSE_STAGE | HG_STATUS_SUCCESS));
        } else {
            prvErrorHandler(pBlk,
                            (HG_TRAVERSE_STAGE | HG_FOOT_STAMP_ERROR |
                             HG_STATUS_ERROR));

        }
    } else {
        prvErrorHandler(pBlk,
                        (HG_TRAVERSE_STAGE | HG_HEAD_STAMP_ERROR |
                         HG_STATUS_ERROR));
    }
}



/* ********************************************************************** */
/* Wrap functions for heap guard */
/* pvPortMalloc wrap function */
void *__wrap_pvPortMalloc(size_t xwanted_size)
{
    void *pvreturn;
    void *vLinkRegAddr;
    FreeBlockLink_t *pBk;
    FreeBlockLink_t *pPrevBk;
    HeapGuardCallBackInfo_t CallbackInfo;

    /* Obtain the return address of caller from link register */
    // __asm volatile ("mov %0, lr":"=r" (vLinkRegAddr));
    vLinkRegAddr = __builtin_return_address(0);
    vTaskSuspendAll();
    /* Set heap block allocated bit, NOTE: it should sync with heap_x.c */
    if (xBlockAllocatedBit == 0) {
        xBlockAllocatedBit =
            ((size_t) 1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1);
    }
    /* Increase additional size to store HeapGuard related info */
    xwanted_size += gxInuseHeapHeaderSize;
    /* Call real pvPortMalloc */
    pvreturn = __real_pvPortMalloc(xwanted_size);
    if (pvreturn != NULL) {
        /* Setup free heap block header */
        pBk = hg_USERADDR_TO_FREELINK(pvreturn);
        /* Assign heap header stamp */
        hg_SET_HEADSTAMP(pBk);
        hg_SET_TASK_HANDLE(pBk, xTaskGetCurrentTaskHandle());
        hg_SET_CALLER_ADDR(pBk, vLinkRegAddr);
        hg_SET_INUSE_FOOTSTAMP(pBk);
        /* Insert inuse heap block in link-list */
        prvInsertBlockIntoInUsedList(pBk);
        pPrevBk = hg_GET_PREV_LINK(pBk);
        HG_Callback(CallbackInfo,
                    hg_FREELINK_TO_USERADDR(pBk),
                    hg_GET_USER_BLOCK_SIZE(pBk),
                    hg_GET_TASK_HANDLE(pBk),
                    hg_GET_CALLER_ADDR(pBk),
                    pPrevBk == NULL ? NULL : hg_GET_TASK_HANDLE(pPrevBk),
                    pPrevBk == NULL ? NULL : hg_GET_CALLER_ADDR(pPrevBk),
                    (HG_ALLOCATE_STAGE | HG_STATUS_SUCCESS));
    } else {
        HG_Callback(CallbackInfo,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL, NULL, (HG_ALLOCATE_STAGE | HG_STATUS_ERROR));
    }
    /* Set HeapGuard related information */
    xTaskResumeAll();
    return pvreturn;
}

/* Add error handle for mtk_HeapGuard_InuseBlockDbgInfo() */
static BaseType_t CheckHeaderAddrValid(FreeBlockLink_t * pTargetChkBlk)
{
    FreeBlockLink_t *pBlk;
    BaseType_t nRet = pdFALSE;
    for (pBlk = gpInuseStart; pBlk; pBlk = hg_GET_NEXT_LINK(pBlk)) {
        if (pTargetChkBlk == pBlk) {
            nRet = pdTRUE;
            break;
        }
    }
    return nRet;
}

/* vPortFree wrap function */
void __wrap_vPortFree(void *pv)
{
    FreeBlockLink_t *pBk, *pPrevBk;
    HeapGuardCallBackInfo_t CallbackInfo;
    if (NULL != pv) {
        vTaskSuspendAll();
        pBk = hg_USERADDR_TO_FREELINK(pv);
        // pInuse = hg_GET_INUSE_HEADER_ADDR(pBk);
        /* check head and foot stamp */
        if (CheckHeaderAddrValid(pBk) == pdTRUE) {
            if (!hg_CHECK_HEAD_STAMP(pBk)) {
                prvErrorHandler(pBk,
                                (HG_FREE_STAGE | HG_HEAD_STAMP_ERROR |
                                 HG_STATUS_ERROR));
            } else if (!hg_CHECK_FOOT_STAMP(hg_GET_INUSE_HEADER_ADDR(pBk))) {
                prvErrorHandler(pBk,
                                (HG_FREE_STAGE | HG_FOOT_STAMP_ERROR |
                                 HG_STATUS_ERROR));
            } else {
                pPrevBk = hg_GET_PREV_LINK(pBk);
                HG_Callback(CallbackInfo,
                            hg_FREELINK_TO_USERADDR(pBk),
                            hg_GET_USER_BLOCK_SIZE(pBk),
                            hg_GET_TASK_HANDLE(pBk),
                            hg_GET_CALLER_ADDR(pBk),
                            pPrevBk ==
                            NULL ? NULL : hg_GET_TASK_HANDLE(pPrevBk),
                            pPrevBk ==
                            NULL ? NULL : hg_GET_CALLER_ADDR(pPrevBk),
                            (HG_FREE_STAGE | HG_STATUS_SUCCESS));
                prvRemoveBlockFromInUsedList(pBk);
                pBk->pNextFreeBlock = NULL;
                __real_vPortFree(pv);
            }
            mtk_HeapGuard_RemoveHeapMonitorInfo(pv);
        }
        xTaskResumeAll();
    }
}



/* Traverse function for inuse heap block link list */
void mtk_HeapGuard_InuseBlockDbgInfo(void *pTargetHeapAddr)
{
    FreeBlockLink_t *pBlk;

    vTaskSuspendAll();
    if (pTargetHeapAddr == NULL) {
        for (pBlk = gpInuseStart; pBlk; pBlk = hg_GET_NEXT_LINK(pBlk)) {
            prvDumpHeapBlock(pBlk);
        }
    } else {
        pBlk = hg_USERADDR_TO_FREELINK(pTargetHeapAddr);
        if (CheckHeaderAddrValid(pBlk) == pdTRUE) {
            prvDumpHeapBlock(pBlk);
        }
    }
    xTaskResumeAll();
}

/*
	Description:
	Remove Heap monitor information from gHPMon array
*/
void mtk_HeapGuard_RemoveHeapMonitorInfo(void *pTargetAddr)
{
    uint32_t i;
    FreeBlockLink_t *pFreeBlkLink;

    vTaskSuspendAll();
    pFreeBlkLink = hg_USERADDR_TO_FREELINK(pTargetAddr);
    if (CheckHeaderAddrValid(pFreeBlkLink) == pdTRUE) {
        if (gHPMonCnt > 0) {
            for (i = 0; i < gHPMonCnt; i++) {
                if (gHPMon[i].pHeapAddr == pFreeBlkLink) {
                    for (; (i + 1) < gHPMonCnt; i++) {
                        gHPMon[i].pHeapAddr = gHPMon[i + 1].pHeapAddr;
                        gHPMon[i].xBkSize = gHPMon[i + 1].xBkSize;
                        gHPMon[i].xTaskHandle = gHPMon[i + 1].xTaskHandle;
                        gHPMon[i].xPrevTask = gHPMon[i + 1].xPrevTask;
                        gHPMon[i].pvPrevCallerAddr =
                            gHPMon[i + 1].pvPrevCallerAddr;
                    }
                    gHPMonCnt--;
                }
            }
        }
    }
    xTaskResumeAll();
}

/*
	Description:
   	Setup dedicate heap monitor information (include user address and user require size),
	when foot stamp is still correct, it can help to dump caller return address even
	though head stamp and heap block size is corrupted
	Return: pdTRUE for success, pdFALSE for other
*/
BaseType_t mtk_HeapGuard_InsertHeapMonitorInfo(void *pTargetAddr)
{
    uint32_t i;
    FreeBlockLink_t *pPrevLink;
    BaseType_t ret = pdFALSE;

    vTaskSuspendAll();
    if (CheckHeaderAddrValid(hg_USERADDR_TO_FREELINK(pTargetAddr)) ==
        pdTRUE) {
        if (gHPMonCnt < MAX_HEAP_MONITOR_NUM) {
            for (i = 0; i < gHPMonCnt; i++) {
                if (gHPMon[i].pHeapAddr ==
                    hg_USERADDR_TO_FREELINK(pTargetAddr)) {
                    break;
                }
            }
            if (i == gHPMonCnt) {
                gHPMon[i].pHeapAddr = hg_USERADDR_TO_FREELINK(pTargetAddr);
                gHPMon[i].xBkSize =
                    hg_GET_BLOCK_SIZE(hg_USERADDR_TO_FREELINK
                                      (pTargetAddr));
                gHPMon[i].xTaskHandle =
                    hg_GET_TASK_HANDLE((hg_USERADDR_TO_FREELINK
                                        (pTargetAddr)));
                gHPMon[i].pvCallerAddr =
                    hg_GET_CALLER_ADDR(hg_USERADDR_TO_FREELINK
                                       (pTargetAddr));
                // pPrevHeapAddr = hg_GET_PRE_FREELINK
                pPrevLink =
                    hg_GET_PREV_LINK(hg_USERADDR_TO_FREELINK(pTargetAddr));
                if (pPrevLink != NULL) {
                    gHPMon[i].xPrevTask = hg_GET_TASK_HANDLE(pPrevLink);
                    gHPMon[i].pvPrevCallerAddr =
                        hg_GET_CALLER_ADDR(pPrevLink);
                } else {
                    gHPMon[i].xPrevTask = NULL;
                    gHPMon[i].pvPrevCallerAddr = NULL;
                }

                gHPMonCnt++;
                ret = pdTRUE;
            }
        }
    }
    xTaskResumeAll();
    return ret;
}

/* Setup callback function*/
void mtk_HeapGuard_SetupCallback(mtk_HeapGuard_cb_func pfnCallback)
{
    if (pfnCallback != NULL) {
        pfnHeapGuardCallBack = pfnCallback;
    }
}
