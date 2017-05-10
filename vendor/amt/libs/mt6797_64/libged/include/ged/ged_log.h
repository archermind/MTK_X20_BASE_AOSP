#ifndef __GED_LOG_H__
#define __GED_LOG_H__

#include "ged_type.h"

#if defined (__cplusplus)
extern "C" {
#endif

GED_LOG_HANDLE ged_log_connect(const char* pszName);

void ged_log_disconnect(GED_LOG_HANDLE hLog);

GED_ERROR ged_log_print(GED_LOG_HANDLE hLog, const char *fmt, ...);

/* print with tpt (tpt = time, pid, and tid) */
GED_ERROR ged_log_tpt_print(GED_LOG_HANDLE hLog, const char *fmt, ...);

GED_ERROR ged_log_reset(GED_LOG_HANDLE hLog);

#if defined (__cplusplus)
}
#endif

#endif
