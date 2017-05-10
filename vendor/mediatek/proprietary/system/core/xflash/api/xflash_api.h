#ifndef __XFLASH_API__
#define __XFLASH_API__

#if defined(_WIN32)
   #if defined(XFLASH_EXPORTS)
       #define LIB_EXPORT __declspec(dllexport)
   #else
       #define LIB_EXPORT __declspec(dllimport)
   #endif
#else
   #if defined(XFLASH_EXPORTS)
       #define LIB_EXPORT __attribute__ ((visibility("default")))
   #else
       #define LIB_EXPORT
   #endif

   #define __stdcall
#endif

#define LIB_API __stdcall

#include "xflash_struct.h"


#ifdef  __cplusplus
extern "C" {
#endif

//! <b>Effects</b>: Setup dll environment.
//! such as locale or others.
//!
//! <b>Parameters</b>: none
//!
//! <b>Returns</b>:  0 if success, or other status indicate error.
LIB_EXPORT int LIB_API xflash_startup();

//! <b>Effects</b>: Cleanup dll environment.
//!
//! <b>Parameters</b>: none
//!
//! <b>Returns</b>:  none.
LIB_EXPORT void LIB_API xflash_cleanup();

//! <b>Effects</b>: set the dll logging level.
//! there are 6 levels, default is level 3: kinfo level.
//!
//! <b>Parameters</b>: level: log level enum.
//!
//! <b>Returns</b>:  none.
LIB_EXPORT void LIB_API xflash_set_log_level(enum logging_level level);

//! <b>Effects</b>: Parsing the scatter file.
//! get the information from the scatter file text.
//!
//! <b>Parameters</b>: scatter_file_name: the scatter file path name.
//!   info: the output variable, the information of the scatter file is put in here.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_get_scatter_info(char8* scatter_file_name, /*IN OUT*/struct scatter_file_info_t* info);

//! <b>Effects</b>: wait for device and get the COM port name.
//! If the correct device connected PC with USB via virtual COM,
//! the function will return, or it will block until timeout.
//!
//! <b>Parameters</b>: filter: the clue for device searching. like "USB\\VID_0E8D&PID_0003" array.
//!   count: the array count.
//!   port_name: the COM port name returned in here.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_waitfor_com(char8** filter, uint32 count, /*OUT*/char8* port_name);

//! <b>Effects</b>: the session of unique device.
//! one connected device has single session, all operations must related with a session.
//!
//! <b>Parameters</b>: NULL
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT HSESSION LIB_API xflash_create_session();

//! <b>Effects</b>: destroy the session.
//! If a device is disconnected, the session must be destroyed to release resources.
//!
//! <b>Parameters</b>: hs: the session handle.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_destroy_session(HSESSION hs);

//! <b>Effects</b>: connect with brom or loader.
//! handshake with loader, notify the loader to enter BROM download mode.
//!
//! <b>Parameters</b>: hs: the session handle.
//!   port_name: the port name such as COM name with which to connect the device.
//!   auth: the authentication file, used to determine whether the tool having the right to connect loader.
//!         if the chip is set to ignore this secure policy, this parameter can be NULL.
//!   cert: security switch. for special guy debug.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_connect_brom(HSESSION hs, char8* port_name,
                                                     char8* auth, char8* cert);

//! <b>Effects</b>: download the preloader file and enter preloader download mode.
//!
//! <b>Parameters</b>: hs: the session handle.
//!   da_name: loader file path name.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_download_loader(HSESSION hs, char8* loader_name);

//! <b>Effects</b>: connect with preloader.
//! handshake with loader, notify the loader to enter download mode.
//!
//! <b>Parameters</b>: hs: the session handle.
//!   port_name: the port name such as COM name with which to connect the device.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_connect_loader(HSESSION hs, char8* port_name);

//! <b>Effects</b>: If loader exists, xflash_connect_brom will connect loader.
//! then download and connect loader is not necessary.
//! so notify kernel to correct the stage.
//!
//! <b>Parameters</b>: hs: the session handle.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_skip_loader_stage(HSESSION hs);

//! <b>Effects</b>: some commands can be executed by loader or brom.
//!  this is extension for API.
//!
//! <b>Parameters</b>: hs: the session handle.
//!   ctrl_code: function control code indicate the action.
//!   inbuffer: input parameter.
//!   inbuffer_size: input buffer size.
//!   outbuffer: output parameter.
//!   outbuffer_size: output buffer size.
//!   bytes_returned: if some value is returned in outbuffer, the actual size is put here.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_device_control(HSESSION hs,
                                               enum device_control_code ctrl_code,
                                               pvoid inbuffer,
                                               uint32 inbuffer_size,
                                               pvoid outbuffer,
                                               uint32 outbuffer_size,
                                               uint32* bytes_returned);

//! <b>Effects</b>: Download images to device.
//!  download some file to specified partition.
//!  this function must called in preloader mode(not brom mode).
//!
//! <b>Parameters</b>: hs: the session handle.
//!   flist: the files infomation, include file path and the partition name to which the file is written.
//!   count: the flist count.
//!   callbacks: the callback functions used in device operations.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_download_partition(HSESSION hs,
                                      struct op_part_list_t* flist, uint32 count
                                      ,struct callbacks_struct_t* callbacks);


//! <b>Effects</b>: tell the preloader to jump the partition to run.
//! Before call this, must call xflash_download to send necessary image to preloader.
//!
//! <b>Parameters</b>: hs: the session handle.
//!   part_name: the partition name at which to start execute.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_partition_execution(HSESSION hs,
                                      const char8* part_name);

//! <b>Effects</b>: Test class function.
//!
//! <b>Parameters</b>: class_name: the class name to test.
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_test(char8* class_name);

//! <b>Effects</b>: Get this DLL's info.
//!
//! <b>Parameters</b>: info: the information returned here
//!
//! <b>Returns</b>:  STATUS_OK if success, or other status indicate specified error.
LIB_EXPORT status_t LIB_API xflash_get_lib_info(dll_info_t* info);


#ifdef  __cplusplus
}
#endif

#endif
