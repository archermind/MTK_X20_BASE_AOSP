
#include <string.h>
#include "cmsis_os.h"


/* Convert from CMSIS type osPriority to FreeRTOS priority number */
static unsigned portBASE_TYPE makeFreeRtosPriority (osPriority priority)
{
	unsigned portBASE_TYPE fpriority = tskIDLE_PRIORITY;

	if (priority != osPriorityError) {
		fpriority += (priority - osPriorityIdle);
	}

	return fpriority;
}

#if (INCLUDE_vTaskPriorityGet == 1)
/* Convert from FreeRTOS priority number to CMSIS type osPriority */
static osPriority makeCmsisPriority (unsigned portBASE_TYPE fpriority)
{
	osPriority priority = osPriorityError;

	if ((fpriority - tskIDLE_PRIORITY) <= (osPriorityRealtime - osPriorityIdle)) {
		priority = (osPriority)((int)osPriorityIdle + (int)(fpriority - tskIDLE_PRIORITY));
	}

	return priority;
}
#endif


/* Determine whether we are in thread mode or handler mode. */
static int inHandlerMode (void)
{
	return __get_IPSR() != 0;
}

/**
* @brief  Initialize the RTOS Kernel for creating objects.
* @param  None
* @return status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osKernelInitialize shall be consistent in every CMSIS-RTOS.
*/
osStatus osKernelInitialize (void)
{
	return osOK;
}

/*********************** Kernel Control Functions *****************************/
/**
* @brief  Start the RTOS Kernel.
* @param  None
* @return status code that indicates the execution status of the function
* @note   MUST REMAIN UNCHANGED: \b osKernelStart shall be consistent in every CMSIS-RTOS.
*/
osStatus osKernelStart (void)
{
	vTaskStartScheduler();
	return osOK;
}

/**
* @brief  Get the RTOS kernel system timer counter
* @param  None
* @return RTOS kernel system timer as 32-bit value
* @note   MUST REMAIN UNCHANGED: \b osKernelSysTick shall be consistent in every CMSIS-RTOS.
*/
uint32_t osKernelSysTick(void)
{
	if (inHandlerMode()) {
		return xTaskGetTickCountFromISR();
	}
	else {
		return xTaskGetTickCount();
	}
}

/**
* @brief  Check if the RTOS kernel is already started.
* @param  None
* @return 0 RTOS is not started, 1 RTOS is started.
* @note  MUST REMAIN UNCHANGED: \b osKernelRunning shall be consistent in every CMSIS-RTOS.
*/
int32_t osKernelRunning(void)
{
#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
		return 0;
	else
		return 1;
#else
	return 0;
#endif

}



/*********************** Thread Management *****************************/

#if (osFeature_Signals != 0)
typedef struct os_thread_cb {
	const osThreadDef_t *thread_def;
	xTaskHandle handle;
	EventGroupHandle_t	signal;
	struct os_thread_cb *next;
} os_thread_cb_t;
static os_thread_cb_t *thread_list=NULL;
#endif

/**
* @brief  Create a thread and add it to Active Threads and set it to state READY.
* @param  thread_def    thread definition referenced with \ref osThread.
* @param  argument      pointer that is passed to the thread function as start argument.
* @return thread ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osThreadCreate shall be consistent in every CMSIS-RTOS.
*/
osThreadId osThreadCreate (const osThreadDef_t *thread_def, void *argument)
{
	xTaskHandle handle;
#if (osFeature_Signals != 0)
	os_thread_cb_t *pthread_cb=NULL;

	/* Create a CMSIS Thread Management Block */
	pthread_cb = (os_thread_cb_t*)pvPortMalloc(sizeof(os_thread_cb_t));
	if (pthread_cb == NULL) {
		return NULL;
	}

	memset(pthread_cb, 0, sizeof(os_thread_cb_t));
	pthread_cb->thread_def = thread_def;

	pthread_cb->signal = xEventGroupCreate();
	if (pthread_cb->signal == NULL) {
		vPortFree(pthread_cb);
		return NULL;
	}

#endif //osFeature_Signals is not 0

	xTaskCreate((pdTASK_CODE)thread_def->pthread,
	            "CMSIS",
	            (thread_def->stacksize + 3) >> 2,
	            argument,
	            makeFreeRtosPriority(thread_def->tpriority),
	            &handle);

#if (osFeature_Signals != 0)
	if (handle == NULL) {
		vEventGroupDelete(pthread_cb->signal);
		vPortFree(pthread_cb);
		return NULL;
	}

	pthread_cb->handle = handle;

	//insert into thread list
	if(thread_list==NULL)
		thread_list = pthread_cb;
	else {
		pthread_cb->next = thread_list;
		thread_list = pthread_cb;
	}

	return pthread_cb;
#else
	return handle;
#endif //osFeature_Signals is not 0

}

/**
* @brief  Return the thread ID of the current running thread.
* @return thread ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osThreadGetId shall be consistent in every CMSIS-RTOS.
*/
osThreadId osThreadGetId (void)
{
#if (osFeature_Signals != 0)
#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )
	xTaskHandle handle = xTaskGetCurrentTaskHandle();
	os_thread_cb_t *pthread_cb=thread_list;
	while (pthread_cb!=NULL) {
		if (pthread_cb->handle == handle)
			return pthread_cb;
		pthread_cb = pthread_cb->next;
	}

	return NULL;
#else
	return NULL;
#endif
#else
#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )
	return xTaskGetCurrentTaskHandle();
#else
	return NULL;
#endif
#endif //osFeature_Signals is not 0
}

/**
* @brief  Terminate execution of a thread and remove it from Active Threads.
* @param   thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osThreadTerminate shall be consistent in every CMSIS-RTOS.
*/
osStatus osThreadTerminate (osThreadId thread_id)
{
#if (osFeature_Signals != 0)
#if (INCLUDE_vTaskDelete == 1)
	os_thread_cb_t *pthread_cb=thread_list;
	os_thread_cb_t *prev_node=NULL;
	while (pthread_cb!=NULL) {
		if (pthread_cb == thread_id) {
			//Delete thread form thread list
			if (pthread_cb == thread_list)
				thread_list = pthread_cb->next;
			else
				prev_node->next = pthread_cb->next;

			//Delete signal and thread
			vEventGroupDelete(pthread_cb->signal);
			vTaskDelete(pthread_cb->handle);
			vPortFree(pthread_cb);
			return osOK;
		}
		prev_node = pthread_cb;
		pthread_cb = pthread_cb->next;
	}
	return osErrorOS;
#else
	return osErrorOS;
#endif
#else
#if (INCLUDE_vTaskDelete == 1)
	vTaskDelete(thread_id);
	return osOK;
#else
	return osErrorOS;
#endif
#endif //osFeature_Signals is not 0
}

/**
* @brief  Pass control to next thread that is in state \b READY.
* @return status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osThreadYield shall be consistent in every CMSIS-RTOS.
*/
osStatus osThreadYield (void)
{
	taskYIELD();
	return osOK;
}

/**
* @brief   Change priority of an active thread.
* @param   thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
* @param   priority      new priority value for the thread function.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osThreadSetPriority shall be consistent in every CMSIS-RTOS.
*/
osStatus osThreadSetPriority (osThreadId thread_id, osPriority priority)
{
#if (INCLUDE_vTaskPrioritySet == 1)
#if (osFeature_Signals != 0)
	xTaskHandle handle = thread_id->handle;
	vTaskPrioritySet(handle, makeFreeRtosPriority(priority));
#else
	vTaskPrioritySet(thread_id, makeFreeRtosPriority(priority));
#endif
	return osOK;
#else
	return osErrorOS;
#endif
}

/**
* @brief   Get current priority of an active thread.
* @param   thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
* @return  current priority value of the thread function.
* @note   MUST REMAIN UNCHANGED: \b osThreadGetPriority shall be consistent in every CMSIS-RTOS.
*/
osPriority osThreadGetPriority (osThreadId thread_id)
{
#if (INCLUDE_vTaskPriorityGet == 1)
#if (osFeature_Signals != 0)
	xTaskHandle handle = thread_id->handle;
	return makeCmsisPriority(uxTaskPriorityGet(handle));
#else
	return makeCmsisPriority(uxTaskPriorityGet(thread_id));
#endif
#else
	return osPriorityError;
#endif
}

/*********************** Generic Wait Functions *******************************/
/**
* @brief   Wait for Timeout (Time Delay)
* @param   millisec      time delay value
* @return  status code that indicates the execution status of the function.
*/
osStatus osDelay (uint32_t millisec)
{
#if INCLUDE_vTaskDelay
	portTickType ticks = millisec / portTICK_RATE_MS;
	vTaskDelay(ticks ? ticks : 1);          /* Minimum delay = 1 tick */
	return osOK;
#else
	(void) millisec;
	return osErrorResource;
#endif
}

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0)) /* Generic Wait available */
/**
* @brief  Wait for Signal, Message, Mail, or Timeout
* @param   millisec  timeout value or 0 in case of no time-out
* @return  event that contains signal, message, or mail information or error code.
* @note   MUST REMAIN UNCHANGED: \b osWait shall be consistent in every CMSIS-RTOS.
*/
osEvent osWait (uint32_t millisec);

#endif  /* Generic Wait available */

/***********************  Timer Management Functions ***************************/
/**
* @brief  Create a timer.
* @param  timer_def     timer object referenced with \ref osTimer.
* @param  type          osTimerOnce for one-shot or osTimerPeriodic for periodic behavior.
* @param  argument      argument to the timer call back function.
* @return  timer ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osTimerCreate shall be consistent in every CMSIS-RTOS.
*/
osTimerId osTimerCreate (const osTimerDef_t *timer_def, os_timer_type type, void *argument)
{
#if (configUSE_TIMERS == 1)
	return xTimerCreate((const char * const)"",
	                    1, // period should be filled when starting the Timer using osTimerStart
	                    (type == osTimerPeriodic) ? pdTRUE : pdFALSE,
	                    (void *) argument,
	                    (pdTASK_CODE)timer_def->ptimer);
#else
	return NULL;
#endif
}

/**
* @brief  Start or restart a timer.
* @param  timer_id      timer ID obtained by \ref osTimerCreate.
* @param  millisec      time delay value of the timer.
* @return  status code that indicates the execution status of the function
* @note   MUST REMAIN UNCHANGED: \b osTimerStart shall be consistent in every CMSIS-RTOS.
*/
osStatus osTimerStart (osTimerId timer_id, uint32_t millisec)
{
	osStatus result = osOK;
#if (configUSE_TIMERS == 1)
	portTickType ticks = millisec / portTICK_RATE_MS;
	if (ticks == 0)  ticks = 1;

	if (inHandlerMode()) {
		return osErrorISR;
	}

	if (xTimerChangePeriod(timer_id, ticks, 0) != pdPASS) {
		result = osErrorParameter;
	}

#else
	result = osErrorParameter;
#endif
	return result;
}

/**
* @brief  Stop a timer.
* @param  timer_id      timer ID obtained by \ref osTimerCreate
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osTimerStop shall be consistent in every CMSIS-RTOS.
*/
osStatus osTimerStop (osTimerId timer_id)
{
	osStatus result = osOK;
#if (configUSE_TIMERS == 1)

	if (inHandlerMode()) {
		return osErrorISR;
	}

	if( xTimerIsTimerActive( timer_id ) != pdFALSE ) {
		return osErrorResource;
	}

	if (xTimerStop(timer_id, 0) != pdPASS) {
		result = osErrorParameter;
	}

#else
	result = osErrorParameter;
#endif
	return result;
}

/**
* @brief  Delete a timer.
* @param  timer_id      timer ID obtained by \ref osTimerCreate
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osTimerCreate shall be consistent in every CMSIS-RTOS.
*/
osStatus  osTimerDelete (osTimerId timer_id)
{
#if (configUSE_TIMERS == 1)
	if (inHandlerMode())
		return osErrorISR;

	if (xTimerDelete(timer_id, portMAX_DELAY) == pdPASS)
		return osOK;
	else
		return osErrorParameter;
#endif
	return osErrorParameter;

}

/***************************  Signal Management ********************************/
#if (osFeature_Signals != 0)
#define SIGNAL_MASK	((1<<osFeature_Signals) - 1)
/**
* @brief  Set the specified Signal Flags of an active thread.
* @param  thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
* @param  signals       specifies the signal flags of the thread that should be set.
* @return  previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
* @note   MUST REMAIN UNCHANGED: \b osSignalSet shall be consistent in every CMSIS-RTOS.
*/
int32_t osSignalSet (osThreadId thread_id, int32_t signal)
{
	portBASE_TYPE taskWoken = pdFALSE;
	EventBits_t prev_flag;

	if (signal & (~SIGNAL_MASK)) {
		return (int32_t)0x80000000;
	}

	if (inHandlerMode()) {
		prev_flag = xEventGroupGetBitsFromISR(thread_id->signal);
		if (xEventGroupSetBitsFromISR(thread_id->signal, signal, &taskWoken) != pdTRUE)
			return (int32_t)0x80000000;
		else
			portEND_SWITCHING_ISR(taskWoken);
	}
	else {
		prev_flag = xEventGroupSetBits(thread_id->signal, signal);
	}
	return (int32_t)prev_flag;
}

/**
* @brief  Clear the specified Signal Flags of an active thread.
* @param  thread_id  thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
* @param  signals    specifies the signal flags of the thread that shall be cleared.
* @return  previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
* @note   MUST REMAIN UNCHANGED: \b osSignalClear shall be consistent in every CMSIS-RTOS.
*/
int32_t osSignalClear (osThreadId thread_id, int32_t signal)
{
	EventBits_t prev_flag;

	if (signal & (~SIGNAL_MASK)) {
		return (int32_t)0x80000000;
	}

	if (inHandlerMode()) {
		prev_flag = xEventGroupClearBitsFromISR(thread_id->signal, signal);
	}
	else {
		prev_flag = xEventGroupClearBits(thread_id->signal, signal);
	}
	return (int32_t)prev_flag;
}


/**
* @brief  Wait for one or more Signal Flags to become signaled for the current \b RUNNING thread.
* @param  signals   wait until all specified signal flags set or 0 for any single signal flag.
* @param  millisec  timeout value or 0 in case of no time-out.
* @return  event flag information or error code.
* @note   MUST REMAIN UNCHANGED: \b osSignalWait shall be consistent in every CMSIS-RTOS.
*/
osEvent osSignalWait (int32_t signals, uint32_t millisec)
{
	os_thread_cb_t *pthread;
	osEvent ret;
	EventBits_t ret_bit;
	portTickType ticks = millisec / portTICK_RATE_MS;

	memset(&ret, 0, sizeof(osEvent));
	ret.status = osOK;

	if (inHandlerMode()) {
		ret.status = osErrorISR;
		return ret;
	}

	if (signals & (~SIGNAL_MASK)) {
		ret.status = osErrorValue;
		return ret;
	}

	pthread = osThreadGetId();
	if (signals == 0) { //Wait for any of bits set
		ret_bit = xEventGroupWaitBits( pthread->signal, SIGNAL_MASK,
		                               pdTRUE, pdFALSE, ticks );
		if (ret_bit != 0)
			ret.status = osEventSignal;
	}
	else {
		ret_bit = xEventGroupWaitBits( pthread->signal, signals,
		                               pdTRUE, pdTRUE, ticks );
		if (ret_bit == signals)
			ret.status = osEventSignal;
	}

	if ( (ret.status!=osEventSignal) && (millisec!=0) ) {
		ret.status = osEventTimeout;
	}
	else {
		ret.value.signals = ret_bit;
	}
	return ret;
}
#else
int32_t osSignalSet (osThreadId thread_id, int32_t signal);
int32_t osSignalClear (osThreadId thread_id, int32_t signal);
osEvent osSignalWait (int32_t signals, uint32_t millisec);
#endif

/****************************  Mutex Management ********************************/
/**
* @brief  Create and Initialize a Mutex object
* @param  mutex_def     mutex definition referenced with \ref osMutex.
* @return  mutex ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osMutexCreate shall be consistent in every CMSIS-RTOS.
*/
osMutexId osMutexCreate (const osMutexDef_t *mutex_def)
{
#if ( configUSE_MUTEXES == 1)
	return xSemaphoreCreateMutex();
#else
	return NULL;
#endif
}

/**
* @brief Wait until a Mutex becomes available
* @param mutex_id      mutex ID obtained by \ref osMutexCreate.
* @param millisec      timeout value or 0 in case of no time-out.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMutexWait shall be consistent in every CMSIS-RTOS.
*/
osStatus osMutexWait (osMutexId mutex_id, uint32_t millisec)
{
	osStatus ret;
	portTickType ticks;

	if (inHandlerMode()) {
		return osErrorISR;
	}

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	ticks = 0;
	if (millisec == osWaitForever) {
		ticks = portMAX_DELAY;
	}
	else if (millisec != 0) {
		ticks = millisec / portTICK_RATE_MS;
		if (ticks == 0) {
			ticks = 1;
		}
	}

	if (xSemaphoreTake(mutex_id, ticks) != pdTRUE) {
		if ( millisec !=0 )
			ret = osErrorTimeoutResource;
		else
			ret = osErrorResource;
	}
	else {
		ret = osOK;
	}

	return ret;
}

/**
* @brief Release a Mutex that was obtained by \ref osMutexWait
* @param mutex_id      mutex ID obtained by \ref osMutexCreate.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMutexRelease shall be consistent in every CMSIS-RTOS.
*/
osStatus osMutexRelease (osMutexId mutex_id)
{
	osStatus ret = osOK;

	if (inHandlerMode()) {
		return osErrorISR;
	}

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (xSemaphoreGive(mutex_id) != pdTRUE) {
		ret = osErrorResource;
	}
	return ret;
}

/**
* @brief Delete a Mutex that was created by \ref osMutexCreate.
* @param mutex_id  mutex ID obtained by \ref osMutexCreate.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMutexDelete shall be consistent in every CMSIS-RTOS.
*/
osStatus osMutexDelete (osMutexId mutex_id)
{
	if (inHandlerMode()) {
		return osErrorISR;
	}

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	vSemaphoreDelete(mutex_id);
	return osOK;
}

/********************  Semaphore Management Functions **************************/

#if (defined (osFeature_Semaphore)  &&  (osFeature_Semaphore != 0))

/**
* @brief Create and Initialize a Semaphore object used for managing resources
* @param semaphore_def semaphore definition referenced with \ref osSemaphore.
* @param count         number of available resources.
* @return  semaphore ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osSemaphoreCreate shall be consistent in every CMSIS-RTOS.
*/
osSemaphoreId osSemaphoreCreate (const osSemaphoreDef_t *semaphore_def, int32_t count)
{
	(void) semaphore_def;
	osSemaphoreId sema;

	if (count == 1) {
		vSemaphoreCreateBinary(sema);
		return sema;
	}

#if (configUSE_COUNTING_SEMAPHORES == 1 )
	return xSemaphoreCreateCounting(count, count);
#else
	return NULL;
#endif
}

/**
* @brief Wait until a Semaphore token becomes available
* @param  semaphore_id  semaphore object referenced with \ref osSemaphore.
* @param  millisec      timeout value or 0 in case of no time-out.
* @return  number of available tokens, or -1 in case of incorrect parameters.
* @note   MUST REMAIN UNCHANGED: \b osSemaphoreWait shall be consistent in every CMSIS-RTOS.
*/
int32_t osSemaphoreWait (osSemaphoreId semaphore_id, uint32_t millisec)
{
	portTickType ticks;

	if (semaphore_id == NULL) {
		return -1;
	}

	ticks = 0;
	if (millisec == osWaitForever) {
		ticks = portMAX_DELAY;
	}
	else if (millisec != 0) {
		ticks = millisec / portTICK_RATE_MS;
		if (ticks == 0) {
			ticks = 1;
		}
	}

	if (inHandlerMode()) {
		portBASE_TYPE taskWoken = pdFALSE;
		if (xSemaphoreTakeFromISR( semaphore_id, &taskWoken ) != pdPASS)
			return -1;
		else
			portEND_SWITCHING_ISR(taskWoken);
	}
	else {
		if (xSemaphoreTake(semaphore_id, ticks) != pdTRUE) {
			return -1;
		}
	}

	return 0;
}

/**
* @brief Release a Semaphore token
* @param  semaphore_id  semaphore object referenced with \ref osSemaphore.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osSemaphoreRelease shall be consistent in every CMSIS-RTOS.
*/
osStatus osSemaphoreRelease (osSemaphoreId semaphore_id)
{
	osStatus ret = osOK;
	portBASE_TYPE taskWoken = pdFALSE;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	if (inHandlerMode()) {
		if (xSemaphoreGiveFromISR(semaphore_id, &taskWoken) != pdTRUE) {
			ret = osErrorResource;
		}
		else {
			portEND_SWITCHING_ISR(taskWoken);
		}
	}
	else {
		if (xSemaphoreGive(semaphore_id) != pdTRUE) {
			ret = osErrorResource;
		}
	}

	return ret;
}

/**
* @brief Delete a Semaphore that was created by \ref osSemaphoreCreate.
* @param  semaphore_id  semaphore object referenced with \ref osSemaphore.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osSemaphoreDelete shall be consistent in every CMSIS-RTOS.
*/
osStatus osSemaphoreDelete (osSemaphoreId semaphore_id)
{
	if (inHandlerMode()) {
		return osErrorISR;
	}

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	vSemaphoreDelete(semaphore_id);
	return osOK;
}

#endif    /* Use Semaphores */

/*******************   Memory Pool Management Functions  ***********************/

#if (defined (osFeature_Pool)  &&  (osFeature_Pool != 0))

//TODO
//This is a primitive and inefficient wrapper around the existing FreeRTOS memory management.
//A better implementation will have to modify heap_x.c!


/**
* @brief Create and Initialize a memory pool
* @param  pool_def      memory pool definition referenced with \ref osPool.
* @return  memory pool ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osPoolCreate shall be consistent in every CMSIS-RTOS.
*/
osPoolId osPoolCreate (const osPoolDef_t *pool_def)
{
	osPoolId thePool;
	int itemSize = 4 * ((pool_def->item_sz + 3) / 4);
	uint32_t i;

	/* First have to allocate memory for the pool control block. */
	thePool = pvPortMalloc(sizeof(os_pool_cb_t));
	if (thePool) {
		thePool->pool_sz = pool_def->pool_sz;
		thePool->item_sz = itemSize;
		thePool->currentIndex = 0;

		/* Memory for markers */
		thePool->markers = pvPortMalloc(pool_def->pool_sz);
		if (thePool->markers) {
			/* Now allocate the pool itself. */
			thePool->pool = pvPortMalloc(pool_def->pool_sz * itemSize);

			if (thePool->pool) {
				for (i = 0; i < pool_def->pool_sz; i++) {
					thePool->markers[i] = 0;
				}
			}
			else {
				vPortFree(thePool->markers);
				vPortFree(thePool);
				thePool = NULL;
			}
		}
		else {
			vPortFree(thePool);
			thePool = NULL;
		}
	}

	return thePool;
}

/**
* @brief Allocate a memory block from a memory pool
* @param pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
* @return  address of the allocated memory block or NULL in case of no memory available.
* @note   MUST REMAIN UNCHANGED: \b osPoolAlloc shall be consistent in every CMSIS-RTOS.
*/
void *osPoolAlloc (osPoolId pool_id)
{
	int dummy = 0;
	void *p = NULL;
	uint32_t i;
	uint32_t index;

	if (inHandlerMode()) {
		dummy = portSET_INTERRUPT_MASK_FROM_ISR();
	}
	else {
		vPortEnterCritical();
	}

	for (i = 0; i < pool_id->pool_sz; i++) {
		index = pool_id->currentIndex + i;
		if (index >= pool_id->pool_sz) {
			index = 0;
		}

		if (pool_id->markers[index] == 0) {
			pool_id->markers[index] = 1;
			p = (void *)((uint32_t)(pool_id->pool) + (index * pool_id->item_sz));
			pool_id->currentIndex = index;
			break;
		}
	}

	if (inHandlerMode()) {
		portCLEAR_INTERRUPT_MASK_FROM_ISR(dummy);
	}
	else {
		vPortExitCritical();
	}

	return p;
}

/**
* @brief Allocate a memory block from a memory pool and set memory block to zero
* @param  pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
* @return  address of the allocated memory block or NULL in case of no memory available.
* @note   MUST REMAIN UNCHANGED: \b osPoolCAlloc shall be consistent in every CMSIS-RTOS.
*/
void *osPoolCAlloc (osPoolId pool_id)
{
	void *p = osPoolAlloc(pool_id);

	if (p != NULL)
	{
		memset(p, 0, sizeof(pool_id->pool_sz));
	}

	return p;
}

/**
* @brief Return an allocated memory block back to a specific memory pool
* @param  pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
* @param  block         address of the allocated memory block that is returned to the memory pool.
* @return  status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osPoolFree shall be consistent in every CMSIS-RTOS.
*/
osStatus osPoolFree (osPoolId pool_id, void *block)
{
	uint32_t index;

	if (pool_id == NULL) {
		return osErrorParameter;
	}

	if (block == NULL) {
		return osErrorValue;
	}

	if (block < pool_id->pool) {
		return osErrorValue;
	}

	index = (uint32_t)block - (uint32_t)(pool_id->pool);
	if (index % pool_id->item_sz) {
		return osErrorParameter;
	}

	index = index / pool_id->item_sz;
	if (index >= pool_id->pool_sz) {
		return osErrorParameter;
	}

	pool_id->markers[index] = 0;

	return osOK;
}


#endif   /* Use Memory Pool Management */

/*******************   Message Queue Management Functions  *********************/

#if (defined (osFeature_MessageQ)  &&  (osFeature_MessageQ != 0)) /* Use Message Queues */

/**
* @brief Create and Initialize a Message Queue
* @param queue_def     queue definition referenced with \ref osMessageQ.
* @param  thread_id     thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
* @return  message queue ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osMessageCreate shall be consistent in every CMSIS-RTOS.
*/
osMessageQId osMessageCreate (const osMessageQDef_t *queue_def, osThreadId thread_id)
{
	(void) thread_id;
	return xQueueCreate(queue_def->queue_sz, sizeof(void *));
}

/**
* @brief Put a Message to a Queue.
* @param  queue_id  message queue ID obtained with \ref osMessageCreate.
* @param  info      message information.
* @param  millisec  timeout value or 0 in case of no time-out.
* @return status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMessagePut shall be consistent in every CMSIS-RTOS.
*/
osStatus osMessagePut (osMessageQId queue_id, uint32_t info, uint32_t millisec)
{
	portBASE_TYPE taskWoken = pdFALSE;
	portTickType ticks;
	osStatus ret = osErrorOS;

	if (queue_id == NULL) {
		return osErrorParameter;
	}

	ticks = millisec / portTICK_RATE_MS;
	if (ticks == 0) {
		ticks = 1;
	}

	if (inHandlerMode()) {
		if (xQueueSendFromISR(queue_id, &info, &taskWoken) == pdTRUE) {
			portEND_SWITCHING_ISR(taskWoken);
			ret = osOK;
		}
	}
	else {
		if (xQueueSend(queue_id, &info, ticks) == pdTRUE) {
			ret = osOK;
		}
	}

	if (ret != osOK) {
		if (millisec != 0)
			ret = osErrorTimeoutResource;
		else
			ret = osErrorResource;
	}

	return ret;
}

/**
* @brief Get a Message or Wait for a Message from a Queue.
* @param  queue_id  message queue ID obtained with \ref osMessageCreate.
* @param  millisec  timeout value or 0 in case of no time-out.
* @return event information that includes status code.
* @note   MUST REMAIN UNCHANGED: \b osMessageGet shall be consistent in every CMSIS-RTOS.
*/
osEvent osMessageGet (osMessageQId queue_id, uint32_t millisec)
{
	portBASE_TYPE taskWoken;
	portTickType ticks;
	osEvent ret;

	ret.def.message_id = queue_id;
	ret.status = osOK;

	if (queue_id == NULL) {
		ret.status = osErrorParameter;
		return ret;
	}

	taskWoken = pdFALSE;

	ticks = 0;
	if (millisec == osWaitForever) {
		ticks = portMAX_DELAY;
	}
	else if (millisec != 0) {
		ticks = millisec / portTICK_RATE_MS;
		if (ticks == 0) {
			ticks = 1;
		}
	}

	if (inHandlerMode()) {
		millisec = 0;
		if (xQueueReceiveFromISR(queue_id, &ret.value.v, &taskWoken) == pdTRUE) {
			/* We have mail */
			ret.status = osEventMessage;
			portEND_SWITCHING_ISR(taskWoken);
		}
	}
	else {
		if (xQueueReceive(queue_id, &ret.value.v, ticks) == pdTRUE) {
			/* We have mail */
			ret.status = osEventMessage;
		}
	}

	if ( (ret.status!=osEventMessage) && (millisec!=0) ) {
		ret.status = osEventTimeout;
	}

	return ret;
}

#endif     /* Use Message Queues */

/********************   Mail Queue Management Functions  ***********************/

#if (defined (osFeature_MailQ)  &&  (osFeature_MailQ != 0))  /* Use Mail Queues */

/**
* @brief Create and Initialize mail queue
* @param  queue_def     reference to the mail queue definition obtain with \ref osMailQ
* @param   thread_id     thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
* @return mail queue ID for reference by other functions or NULL in case of error.
* @note   MUST REMAIN UNCHANGED: \b osMailCreate shall be consistent in every CMSIS-RTOS.
*/
osMailQId osMailCreate (const osMailQDef_t *queue_def, osThreadId thread_id)
{
	os_mailQ_cb_t *pcb;
	osSemaphoreDef_t sema;
	osPoolDef_t pool_def = {queue_def->queue_sz, queue_def->item_sz};

	(void) thread_id;

	/* Create a mail queue control block */
	pcb = pvPortMalloc(sizeof(os_mailQ_cb_t));
	if (pcb == NULL) {
		return NULL;
	}

	pcb->queue_def = queue_def;

	/* Create a queue in FreeRTOS */
	pcb->handle = xQueueCreate(queue_def->queue_sz, sizeof(void *));
	if (pcb->handle == NULL) {
		vPortFree(pcb);
		return NULL;
	}

	pcb->sema = osSemaphoreCreate (&sema, queue_def->queue_sz);
	if (pcb->sema == NULL) {
		vQueueDelete(pcb->handle);
		vPortFree(pcb);
		return NULL;
	}

	/* Create a mail pool */
	pcb->pool = osPoolCreate(&pool_def);
	if (pcb->pool == NULL) {
		osSemaphoreDelete(pcb->sema);
		vQueueDelete(pcb->handle);
		vPortFree(pcb);
		return NULL;
	}

	return pcb;
}

/**
* @brief Allocate a memory block from a mail
* @param  queue_id      mail queue ID obtained with \ref osMailCreate.
* @param  millisec      timeout value or 0 in case of no time-out.
* @return pointer to memory block that can be filled with mail or NULL in case error.
* @note   MUST REMAIN UNCHANGED: \b osMailAlloc shall be consistent in every CMSIS-RTOS.
*/
void *osMailAlloc (osMailQId queue_id, uint32_t millisec)
{
	void *p;

	if (queue_id == NULL) {
		return NULL;
	}

	if (osSemaphoreWait (queue_id->sema, millisec) < 0) {
		return NULL;
	}

	p = osPoolAlloc(queue_id->pool);

	return p;
}

/**
* @brief Allocate a memory block from a mail and set memory block to zero
* @param  queue_id      mail queue ID obtained with \ref osMailCreate.
* @param  millisec      timeout value or 0 in case of no time-out.
* @return pointer to memory block that can be filled with mail or NULL in case error.
* @note   MUST REMAIN UNCHANGED: \b osMailCAlloc shall be consistent in every CMSIS-RTOS.
*/
void *osMailCAlloc (osMailQId queue_id, uint32_t millisec)
{
	void *p;

	if (queue_id == NULL) {
		return NULL;
	}

	if (osSemaphoreWait (queue_id->sema, millisec) < 0) {
		return NULL;
	}

	p = osPoolCAlloc(queue_id->pool);

	return p;
}

/**
* @brief Put a mail to a queue
* @param  queue_id      mail queue ID obtained with \ref osMailCreate.
* @param  mail          memory block previously allocated with \ref osMailAlloc or \ref osMailCAlloc.
* @return status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMailPut shall be consistent in every CMSIS-RTOS.
*/
osStatus osMailPut (osMailQId queue_id, void *mail)
{
	portBASE_TYPE taskWoken;

	if (queue_id == NULL) {
		return osErrorParameter;
	}

	if (mail == NULL) {
		return osErrorValue;
	}

	taskWoken = pdFALSE;

	if (inHandlerMode()) {
		if (xQueueSendFromISR(queue_id->handle, &mail, &taskWoken) != pdTRUE) {
			return osErrorOS;
		}
		else
			portEND_SWITCHING_ISR(taskWoken);
	}
	else {
		if (xQueueSend(queue_id->handle, &mail, 0) != pdTRUE) {
			return osErrorOS;
		}
	}

	return osOK;
}

/**
* @brief Get a mail from a queue
* @param  queue_id   mail queue ID obtained with \ref osMailCreate.
* @param millisec    timeout value or 0 in case of no time-out
* @return event that contains mail information or error code.
* @note   MUST REMAIN UNCHANGED: \b osMailGet shall be consistent in every CMSIS-RTOS.
*/
osEvent osMailGet (osMailQId queue_id, uint32_t millisec)
{
	portBASE_TYPE taskWoken;
	portTickType ticks;
	osEvent ret;

	ret.def.mail_id = queue_id;
	ret.status = osOK;

	if (queue_id == NULL) {
		ret.status = osErrorParameter;
		return ret;
	}

	taskWoken = pdFALSE;

	ticks = 0;
	if (millisec == osWaitForever) {
		ticks = portMAX_DELAY;
	}
	else if (millisec != 0) {
		ticks = millisec / portTICK_RATE_MS;
		if (ticks == 0) {
			ticks = 1;
		}
	}

	if (inHandlerMode()) {
		millisec = 0;
		if (xQueueReceiveFromISR(queue_id->handle, &ret.value.p, &taskWoken) == pdTRUE) {
			/* We have mail */
			ret.status = osEventMail;
			portEND_SWITCHING_ISR(taskWoken);
		}
	}
	else {
		if (xQueueReceive(queue_id->handle, &ret.value.p, ticks) == pdTRUE) {
			/* We have mail */
			ret.status = osEventMail;
		}
	}

	if ( (ret.status!=osEventMail) && (millisec!=0) ) {
		ret.status = osEventTimeout;
	}

	return ret;
}


/**
* @brief Free a memory block from a mail
* @param  queue_id mail queue ID obtained with \ref osMailCreate.
* @param  mail     pointer to the memory block that was obtained with \ref osMailGet.
* @return status code that indicates the execution status of the function.
* @note   MUST REMAIN UNCHANGED: \b osMailFree shall be consistent in every CMSIS-RTOS.
*/
osStatus osMailFree (osMailQId queue_id, void *mail)
{
	if (queue_id == NULL) {
		return osErrorParameter;
	}

	if (mail == NULL) {
		return osErrorValue;
	}

	osPoolFree(queue_id->pool, mail);

	osSemaphoreRelease (queue_id->sema);

	return osOK;
}

#endif  /* Use Mail Queues */


