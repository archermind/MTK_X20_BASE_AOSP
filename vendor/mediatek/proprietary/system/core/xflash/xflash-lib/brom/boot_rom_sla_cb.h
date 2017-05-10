#ifndef __BOOT_ROM_SLA_CB__
#define __BOOT_ROM_SLA_CB__

#include "common/types.h"

#if defined(WIN32) || defined(_WIN32)
#  define SECURITY_VERIFY_CONNECTION_SUPPORT
#endif

 class sla_callbacks
 {
 private:
    static void* hmodule;
 public:
    static int sla_start(uint8* in, uint32 inlen, uint8** ppout, uint32* outlen);
    static int sla_end(uint8* out);
 };
#endif