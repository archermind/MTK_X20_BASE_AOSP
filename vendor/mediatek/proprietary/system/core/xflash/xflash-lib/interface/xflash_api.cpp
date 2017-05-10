#include "xflash_api.h"
#include "error_code.h"
#include "logic/kernel.h"
#include "logic/logic.h"
#include "logic/connection.h"
#include "interface/rc.h"

int LIB_API xflash_startup()
{
   return kernel::startup();
}

void LIB_API xflash_cleanup()
{
   kernel::cleanup();
   return;
}

void LIB_API xflash_set_log_level(enum logging_level level)
{
   kernel::set_log_level(level);
   return;
}

status_t LIB_API xflash_get_scatter_info(char8* scatter_file_name, scatter_file_info_t* scatter_file)
{
   CALL_LOGI;
   return logic::get_scatter_info(scatter_file_name, scatter_file);
}

status_t LIB_API xflash_waitfor_com(char8** filter, uint32 count, char8* port_name)
{
   CALL_LOGI;
   string sport_name;
   status_t status = connection::waitfor_com(filter, count, sport_name);
   if(SUCCESSED(status))
   {
      memcpy(port_name, sport_name.c_str(), sport_name.size());
   }

   return status;
}

HSESSION LIB_API xflash_create_session()
{
   CALL_LOGI;
   return connection::create_session();
}

status_t LIB_API xflash_destroy_session(HSESSION hs)
{
   CALL_LOGI;
   return connection::destroy_session(hs);
}

status_t LIB_API xflash_connect_brom(HSESSION hs, char8* port_name, char8* auth, char8* cert)
{
   CALL_LOGI;
   return connection::connect_brom(hs, port_name?port_name:"", auth?auth:"", cert?cert:"");
}

status_t LIB_API xflash_download_loader(HSESSION hs,
                                      char8* loader_name)
{
   CALL_LOGI;
   //in case empty pointer exception when cast to string.
   return connection::download_loader(hs, loader_name?loader_name:"");
}

status_t LIB_API xflash_connect_loader(HSESSION hs, char8* port_name)
{
   CALL_LOGI;
   //in case empty pointer exception when cast to string.
   return connection::connect_loader(hs, port_name?port_name:"");
}

status_t LIB_API xflash_skip_loader_stage(HSESSION hs)
{
   CALL_LOGI;
   return connection::skip_loader_stage(hs);
}

status_t LIB_API xflash_download_partition(HSESSION hs, struct op_part_list_t* flist, uint32 count,
                         struct callbacks_struct_t* callbacks)
{
   CALL_LOGI;

   return logic::download_partition(hs, flist, count, callbacks);

}

status_t LIB_API xflash_partition_execution(HSESSION hs, const char* part_name)
{
   CALL_LOGI;
   if(part_name == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }
   std::string name = part_name;
   return logic::jump_to_partition(hs, name);
}

status_t LIB_API xflash_device_control(HSESSION hs,
                                          enum device_control_code ctrl_code,
                                          pvoid inbuffer,
                                          uint32 inbuffer_size,
                                          pvoid outbuffer,
                                          uint32 outbuffer_size,
                                          uint32* bytes_returned)
{
   CALL_LOGI;
   return connection::device_control(hs, (uint32)ctrl_code, inbuffer, inbuffer_size,
      outbuffer, outbuffer_size, bytes_returned);
}

status_t LIB_API xflash_get_lib_info(dll_info_t* info)
{
   CALL_LOGI;
   if(info == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }

   info->version = (char8*)DLL_VERSION;
   info->release_type = (char8*)DLL_RELEASE_TYPE;
   info->build_data = (char8*)__DATE__;
   info->change_list = (char8*)DLL_CHANGE_LIST;
   info->builder = (char8*)DLL_BUILDER;

   return STATUS_OK;
}

