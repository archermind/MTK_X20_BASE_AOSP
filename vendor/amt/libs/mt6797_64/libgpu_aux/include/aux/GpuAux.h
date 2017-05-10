
#ifndef __GPUAUX_H__
#define __GPUAUX_H__

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <system/window.h>
#include <system/graphics.h>

__BEGIN_DECLS

struct GPUAUXContext;
typedef struct GPUAUXContext* GPUAUXContextHandle;

int GpuAuxIsSupportFormat(android_native_buffer_t * buf);

GPUAUXContextHandle GpuAuxCreateContext(int outputFormat, int numBuffers);

int GpuAuxPrepareBuffer(GPUAUXContextHandle ctx, android_native_buffer_t* srcBuffer);

int GpuAuxGetCurrentIdx(GPUAUXContextHandle ctx);
android_native_buffer_t* GpuAuxGetCurrentAuxBuffer(GPUAUXContextHandle ctx);
android_native_buffer_t* GpuAuxGetCurrentSourceBuffer(GPUAUXContextHandle ctx);

int GpuAuxDoConversionIfNeed(GPUAUXContextHandle ctx);

int GpuAuxDestoryContext(GPUAUXContextHandle ctx);

__END_DECLS

#endif

