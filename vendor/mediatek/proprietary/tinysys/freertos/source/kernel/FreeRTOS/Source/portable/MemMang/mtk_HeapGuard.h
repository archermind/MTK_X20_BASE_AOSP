#ifndef _MTK_HEAPGUARD_HEADER_
#define _MTK_HEAPGUARD_HEADER_

/* Heap Guard callback function related defintion,
   User can provide callback function to dump debug message
*/

/* Callback function status code*/
/* Processing stage code */
#define HG_PROCESS_MASK			(0x000000FF)
#define HG_ALLOCATE_STAGE		(0x00000001)
#define HG_FREE_STAGE			(0x00000002)
#define HG_TRAVERSE_STAGE		(0x00000004)
/* Major status code */
#define HG_MAJOR_STS_MASK		(0xFF000000)
#define HG_STATUS_SUCCESS		(0x80000000)
#define HG_STATUS_ERROR			(0x40000000)
#define HG_STATUS_RECOVER		(0x20000000)
/* Minor status code */
#define HG_MINOR_STS_MASK		(0x0000FF00)
#define HG_HEAD_STAMP_ERROR		(0x00000100)
#define HG_FOOT_STAMP_ERROR		(0x00000200)
/* Data struction defintion for callback function*/
/**
	*pUserAddr 			: 	Start address of heap user space
	xBlockSize 			: 	Total size of heap chunk (includes stamp and link-list overhead)
	*pvCallerAddr		:	Caller address of current heap chunck
	*pxTCB				:	TCB for current task
	*pvPrevCallerAddr	:	Caller address of previous heap chunk
	*pxPrevTCB			:	TCB for the task of previous heap chunk
	status				:	return status
	*/
typedef struct HEAP_GUARD_CALLBACK_INFO {
    void *pUserAddr;
    size_t xBlockSize;
    void *pvCallerAddr;
    TaskHandle_t *pxTCB;
    void *pvPrevCallerAddr;
    TaskHandle_t *pxPrevTCB;
    uint32_t status;
} HeapGuardCallBackInfo_t;

/* call back function defintion */
typedef void (*mtk_HeapGuard_cb_func) (HeapGuardCallBackInfo_t * pInfo);

/* Public API */
/* Dump debug information of inuse heap chunk (heap address, size, caller address/TCB of
   current and previious heap chunk. If pTargetHeapAddr is NULL, this function will do traverse for
   whole link list */
void mtk_HeapGuard_InuseBlockDbgInfo(void *pTargetHeapAddr);
/**
	Heap monitor information, pass user start address of heap to
	insert/remove heap monitor information
	NOTE monitor information will also be removed when user call
	vPortFree() to free heap chunk */
BaseType_t mtk_HeapGuard_InsertHeapMonitorInfo(void *pTargetHeapAddr);
void mtk_HeapGuard_RemoveHeapMonitorInfo(void *pTargetAddr);

/**
	Public API to setup callback function, heap guard module will call
	callback function when allocate/free heap chunk, the callback function
	will also be involved when error occured (heap currupted)
	*/
void mtk_HeapGuard_SetupCallback(mtk_HeapGuard_cb_func pfnCallback);

#endif                          // _MTK_HEAPGUARD_HEADER_
