#include "boot_rom_sla_cb.h"

#if defined(SECURITY_VERIFY_CONNECTION_SUPPORT)

#include <windows.h>

typedef int (__stdcall *CALLBACK_SLA_CHALLENGE)
(
 void *usr_arg ,
 const unsigned char *p_challenge_in,
 unsigned int challenge_in_len,
 unsigned char **pp_challenge_out,
 unsigned int *p_challenge_out_len
 );


 typedef int (__stdcall *CALLBACK_SLA_CHALLENGE_END)
 (
 void *usr_arg,
 unsigned char *p_challenge_out
 );

void* sla_callbacks::hmodule = 0;

int32 sla_callbacks::sla_start(uint8* in, uint32 inlen, uint8** ppout, uint32* outlen)
{
   if(hmodule == 0)
   {
      hmodule = (void*)LoadLibrary("SLA_Challenge.dll");
   }

   CALLBACK_SLA_CHALLENGE cb = (CALLBACK_SLA_CHALLENGE)GetProcAddress((HMODULE)hmodule, "SLA_Challenge");
   if(cb != 0)
   {
      return cb(0, in, inlen, ppout, outlen);
   }
   else
   {
      return -1;
   }
}

int32 sla_callbacks::sla_end(uint8* out)
{
   int32 result = 0;
   if(hmodule == 0)
   {
      return -1;
   }

   CALLBACK_SLA_CHALLENGE_END cb = (CALLBACK_SLA_CHALLENGE_END)GetProcAddress((HMODULE)hmodule, "SLA_Challenge_END");
   if(cb != 0)
   {
      return cb(0, out);
   }
   else
   {
      result = -1;
   }

   if(hmodule != 0)
   {
      FreeLibrary((HMODULE)hmodule);
      hmodule = 0;
   }

   return result;
}

#else

int32 sla_callbacks::sla_start(uint8* in, uint32 inlen, uint8** ppout, uint32* outlen)
{
  return -1;
}

int32 sla_callbacks::sla_end(uint8* out)
{
   return -1;
}

#endif