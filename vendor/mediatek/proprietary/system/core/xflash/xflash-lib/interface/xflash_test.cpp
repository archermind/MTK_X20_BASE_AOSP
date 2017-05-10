#include "xflash_api.h"
#include "error_code.h"
#include <string>
#include "transfer/comm_engine.h"

status_t LIB_API flashtool_test(char* class_name)
{
   status_t status = STATUS_OK;
   string sclass = class_name;
   if(sclass == "device_test")
   {
     //device_test::test();
   }
   else if(sclass == "comm_engine")
   {
      comm_engine::test();
   }
   else
   {

   }

   return status;
}