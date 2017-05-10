#pragma once

#include <boost/container/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include "xflash_api.h"


#if defined(_WIN32) || defined(_LINUX)
namespace STD_ = std;
#else
namespace STD_ = std::__1;
#endif

class ICommand
{
public:
   virtual status_t execute(boost::container::map<STD_::string, STD_::string> args) = 0;
   virtual ~ICommand(){}
};

class ACallbackable
{
public:
   ACallbackable(): stop_flag(NULL){}
   void set_stop_var(int32* stop_f);
public:
   virtual BOOL is_notify_stopped();
   virtual void operation_progress(unsigned int progress);
   virtual void stage_message(char* message);
   virtual ~ACallbackable(){}
protected:
   int32* stop_flag;
};

template <class T>
class gui_callbacks
{
public:
   static BOOL __stdcall is_notify_stopped(void* _this)
   {
      return ((T*)_this)->is_notify_stopped();
   };
   static void __stdcall operation_progress(void* _this, enum transfer_phase phase,
      unsigned int progress)
   {
      ((T*)_this)->operation_progress(progress);
   }
   static void __stdcall stage_message(void* _this, char* message)
   {
      ((T*)_this)->stage_message(message);
   }
};


//commands implement.
class cmd_enter_fastboot : public ICommand, public ACallbackable
{
public:
   cmd_enter_fastboot(void){}
   virtual ~cmd_enter_fastboot(void){}
   virtual status_t execute(boost::container::map<STD_::string, STD_::string> args);
};

class cmd_flash : public ICommand, public ACallbackable
{
public:
   cmd_flash(void){}
   virtual ~cmd_flash(void){}
   virtual status_t execute(boost::container::map<STD_::string, STD_::string> args);
private:
   void run_script(STD_::string script_path_name, STD_::string device_sn);
   static boost::container::map<STD_::string, STD_::string> session_map;
   static boost::mutex session_mtx;
};