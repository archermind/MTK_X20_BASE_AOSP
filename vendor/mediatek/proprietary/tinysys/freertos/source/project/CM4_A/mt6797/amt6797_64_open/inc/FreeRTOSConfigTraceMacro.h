#ifndef FREERTOS_CONFIG_TRACE_MACRO_H
#define FREERTOS_CONFIG_TRACE_MACRO_H

#define traceTASK_CREATE(xTask) printf("\n\r task:%s was created(traceTASK_CREATE()) \n\r",xTask->pcTaskName);

#endif /* FREERTOS_CONFIG_TRACE_MACRO_H */
