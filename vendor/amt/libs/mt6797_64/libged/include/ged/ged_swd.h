#ifndef __GED_SW_WATCH_DOG_H__
#define __GED_SW_WATCH_DOG_H__

#include "ged_type.h"
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

typedef	enum GED_SWD_FENCE_FROM_TAG
{
	GED_SWD_FENCE_FROM_QUEUE_BUFFER,
    GED_SWD_FENCE_FROM_DEQUEUE_BUFFER,
} GED_SWD_FENCE_FROM_TYPE;

//set szName as NULL for default name: ged-swd
GED_SWD_HANDLE ged_swd_create(int i32MaxQueueCount, const char *szName);

void ged_swd_destroy(GED_SWD_HANDLE hHandle);

GED_ERROR ged_swd_push_fence(GED_SWD_HANDLE hHandle, GED_SWD_FENCE_FROM_TYPE type, int fence);

GED_ERROR ged_boost_host_event(GED_SWD_HANDLE hHandle, bool bSwitch);

#if defined (__cplusplus)
}
#endif

#endif
