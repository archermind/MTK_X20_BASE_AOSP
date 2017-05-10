#pragma once

#include "xflash_struct.h"
#include "common/common_include.h"
#include <boost/thread/recursive_mutex.hpp>

class connection
{
public:
   connection(void);
   ~connection(void);
public:
   static HSESSION create_session(HSESSION prefer = INVALID_HSESSION_VALUE);
   static status_t destroy_session(HSESSION hs);
   static status_t connect_brom(HSESSION hs, string port_name, string auth, string cert);
   static status_t download_loader(HSESSION hs,
                                string preloader_name);
   static status_t connect_loader(HSESSION hs, string port_name);
   static status_t skip_loader_stage(HSESSION hs);
   static status_t waitfor_com(char8** filter, uint32 count, STD_::string& port_name);

   static status_t device_control(HSESSION hs,
                                    uint32 ctrl_code,
                                    void* inbuffer,
                                    uint32 inbuffer_size,
                                    void* outbuffer,
                                    uint32 outbuffer_size,
                                    uint32* bytes_returned);

private:
   static boost::recursive_mutex com_mtx;
};
