#ifndef __GED_H__
#define __GED_H__

#include <utils/Timers.h>
#include "ged_log.h"
#include "ged_swd.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef enum
{
    GED_BOOST_GPU_FREQ_LEVEL_MAX = 100
} GED_BOOST_GPU_FREQ_LEVEL;


GED_HANDLE ged_create(void);

void ged_destroy(GED_HANDLE hGed);

GED_ERROR ged_boost_gpu_freq(GED_HANDLE hGed, GED_BOOST_GPU_FREQ_LEVEL eLevel);

GED_BOOL ged_check_null_driver_enable(GED_HANDLE hGed);

void ged_update_null_driver_enable(GED_HANDLE hGed);

GED_ERROR ged_notify_sw_vsync(GED_HANDLE hGed, GED_DVFS_UM_QUERY_PACK* psQueryData);

GED_ERROR ged_dvfs_probe(GED_HANDLE hGed, int pid);

GED_ERROR ged_dvfs_um_return(GED_HANDLE hGed, unsigned long gpu_tar_freq, bool bFallback);

GED_ERROR ged_query_info( GED_HANDLE hGed, GED_INFO eType, size_t size, void* retrieve);

GED_ERROR ged_event_notify(GED_HANDLE hGed, GED_DVFS_VSYNC_OFFSET_SWITCH_CMD eEvent, bool bSwitch);

// provide vsync_period in ns
GED_ERROR ged_vsync_calibration(GED_HANDLE hGed, int i32Delay, unsigned long  nsVsync_period);
GED_ERROR ged_vsync_notify(GED_HANDLE hGed, unsigned long  msVsync_period);



#if defined (__cplusplus)
}
#endif

#endif
