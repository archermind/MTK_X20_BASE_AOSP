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

#include "stdio.h"
#include "stdarg.h"

#include "FreeRTOS.h"
#include "task.h"
#include "utils.h"

typedef struct
{
	uint32_t numOfTasks;
	TaskStatus_t *pxTaskStatusArray;
} TaskList_t;

static traceBufferType trace_buffer;

void trace_init(void)
{
	trace_buffer.head = 0;
	trace_buffer.tail = 0;
}

void add_trace(traceRtosId event, uint32_t tid1, uint32_t priority1,
				uint32_t state1, uint32_t tid2, uint32_t priority2)
{
	portENTER_CRITICAL();

	if ((trace_buffer.tail + 1) % MAX_TRACE_ITEMS == trace_buffer.head) {
		dump_trace();
	}
	trace_buffer.tail = (trace_buffer.tail + 1) % MAX_TRACE_ITEMS;
	trace_buffer.events[trace_buffer.tail].time = timestamp_get_ns()/1000;
	trace_buffer.events[trace_buffer.tail].event = event;
	trace_buffer.events[trace_buffer.tail].tid1 = tid1;
	trace_buffer.events[trace_buffer.tail].priority1 = priority1;
	trace_buffer.events[trace_buffer.tail].state1 = state1;
	trace_buffer.events[trace_buffer.tail].tid2 = tid2;
	trace_buffer.events[trace_buffer.tail].priority2 = priority2;

	portEXIT_CRITICAL();
}

void build_task_list(TaskList_t *pTaskList)
{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize;

	/* Take a snapshot of the number of tasks in case it changes while this
	function is executing. */
	uxArraySize = uxTaskGetNumberOfTasks();

	/* Allocate an array index for each task. */
	pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

	if( pxTaskStatusArray != NULL ) {
		/* Generate the (binary) data. */
		uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );
	}

	pTaskList->numOfTasks = uxArraySize;
	pTaskList->pxTaskStatusArray = pxTaskStatusArray;
}

void build_task_lookup_table(TaskList_t *pTaskList, TaskStatus_t ***pTaskLookupTable)
{
	uint32_t i, maxTaskNumber=0;

	for (i = 0; i < pTaskList->numOfTasks; i++) {
		if ( pTaskList->pxTaskStatusArray[i].xTaskNumber > maxTaskNumber) {
			maxTaskNumber = pTaskList->pxTaskStatusArray[i].xTaskNumber;
		}
	}

	/* Unique tid. Some slots are unused (0 and those for deleted tasks) */
	*pTaskLookupTable = pvPortMalloc( (maxTaskNumber+ 1) * sizeof(TaskStatus_t*));

	if ( pTaskLookupTable != NULL) {

		/* Initialize the mapping table */
		for (i = 0; i <= maxTaskNumber; i++) {
			(*pTaskLookupTable)[i] = NULL;
		}

		/* Populate the task lookup table with tid as the key */
		for (i = 0; i < pTaskList->numOfTasks; i++) {
			(*pTaskLookupTable)[pTaskList->pxTaskStatusArray[i].xTaskNumber] =
			&pTaskList->pxTaskStatusArray[i];
		}
	}
}

void dump_trace(void)
{
	TaskList_t taskList;
	const char *taskName1, *taskName2;
	TaskStatus_t **taskLookupTable;
	uint32_t i;
	const char* linux_state[5] = { "R", "R", "S", "D", "Z" };

	/* build task list */
	build_task_list(&taskList);

	/* taskLookupTable[tid] points to the task */
	build_task_lookup_table(&taskList, &taskLookupTable);

	for (i = 0; i < MAX_TRACE_ITEMS; i++) {

		if (taskLookupTable[trace_buffer.events[i].tid1] == NULL)
			continue;
		taskName1 = taskLookupTable[trace_buffer.events[i].tid1]->pcTaskName;

		if (taskLookupTable[trace_buffer.events[i].tid2] == NULL)
			continue;
		taskName2 = taskLookupTable[trace_buffer.events[i].tid2]->pcTaskName;

		PRINTF(TAG_RTOS, "<%s>-%d [000]  %llu.%06lu: sched_switch: prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s ==> next_comm=%s next_pid=%d next_prio=%d\n\r",
				taskName1,
				trace_buffer.events[i].tid1,
				trace_buffer.events[i].time/1000000,
				(uint32_t) trace_buffer.events[i].time%1000000,
				taskName1,
				trace_buffer.events[i].tid1,
				trace_buffer.events[i].priority1,
				linux_state[trace_buffer.events[i].state1],
				taskName2,
				trace_buffer.events[i].tid2,
				trace_buffer.events[i].priority2);
	}
	vPortFree( taskLookupTable );
	vPortFree( taskList.pxTaskStatusArray );
}
