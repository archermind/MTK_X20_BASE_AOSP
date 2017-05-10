#include "cmds_impl.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
# pragma warning (disable:4996) //unsafe
# pragma warning (disable:4819) //unicode warning
#include <boost/format.hpp>
#include "xflash_api.h"
#include "error_code.h"
#include "zbuffer.h"
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <qprocess.h>
#include <qstring.h>

using namespace STD_;

void ACallbackable::set_stop_var(int32* stop_f)
{
   stop_flag = stop_f;
}

BOOL ACallbackable::is_notify_stopped()
{
   return (*stop_flag) != 0;
}

void ACallbackable::operation_progress(unsigned int progress)
{
   std::cout <<"progress " <<progress;
}

void ACallbackable::stage_message(char* message)
{
   STD_::string s = message;
   s += "\n";
   std::cout << s;
}


status_t cmd_enter_fastboot::execute(boost::container::map<STD_::string, STD_::string> args)
{
   scatter_file_info_t info;
   status_t status = xflash_get_scatter_info((char8*)args["scatter-file-name"].c_str(), &info);
   if(FAIL(status))
   {
      std::cout << (boost::format("get scatter info error. code 0x%X")%status).str();
      return status;
   }

   boost::shared_ptr<zbuffer> lst = boost::make_shared<zbuffer>(info.hdr.partition_count * sizeof(op_part_list_t));
   op_part_list_t* flist = (op_part_list_t*)lst->get();

   STD_::string fastboot_partition = "lk";
   STD_::string loader_file;
   uint32 count_needed = 0;
   for(uint32 idx = 0; idx<info.hdr.partition_count; ++idx)
   {
      if(info.part[idx].empty_boot_needed)
      {
         strncpy(flist[count_needed].part_name, info.part[idx].part_name, PARTITION_NAME_LEN);
         strncpy(flist[count_needed].file_path, info.part[idx].file_name, MAX_FILE_NAME_LEN);
         ++count_needed;

         //if has atf partition, should jump atf first.
         if(STD_::string(info.part[idx].part_name) == "tee1")
         {
            fastboot_partition = "tee1";
         }
      }

      if(STD_::string(info.part[idx].part_name) == "preloader")
      {
         loader_file = info.part[idx].file_name;
      }
   }

   if(count_needed < 1)
   {
      std::cout << "lk.bin must exist in download list. check scatter file.";
      return STATUS_ERR;
   }


   char* filter[2] =
   {
      "USB\\VID_0E8D&PID_2000",
      "USB\\VID_0E8D&PID_0003"
   };

   char port_name[256] = {0};
    memset(port_name, 0x00, 256);
   std::cout << "\nSTART." << std::endl;
   std::cout << "wait for device." << std::endl;
   status = xflash_waitfor_com(filter, sizeof(filter)/sizeof(filter[0]), port_name);
   if(FAIL(status))
   {
      std::cout << (boost::format("wait for com error. code 0x%x\n")%status).str();
      return status;
   }

   int32 stop_f = 0;
   callbacks_struct_t cbs =
   {
      (void*)this,
      &gui_callbacks<cmd_enter_fastboot>::operation_progress,
      &gui_callbacks<cmd_enter_fastboot>::stage_message,
      &gui_callbacks<cmd_enter_fastboot>::is_notify_stopped,
   };

   HSESSION hs = xflash_create_session();

   std::cout << "connect boot rom." << std::endl;
   status = xflash_connect_brom(hs, port_name, (char8*)args["auth-file-name"].c_str(), NULL);

   uint32 boot_at = BOOT_AGENT_UNKNOWN;

   if(FAIL(status))
   {
      std::cout << (boost::format("connect boot rom error. code 0x%x\n")%status).str();
      goto exit;
   }

   status = xflash_device_control(hs, DEV_GET_BOOT_AGENT, 0, 0, &boot_at, sizeof(uint32), 0);
   if(FAIL(status))
   {
      std::cout << (boost::format("DEV_GET_BOOT_AGENT error. code 0x%x\n")%status).str();
      goto exit;
   }

   if (boot_at == BOOT_AGENT_BOOT_ROM)
   {
      std::cout << "download preloader." << std::endl;
      status = xflash_download_loader(hs, (char8*)loader_file.c_str());
      if(FAIL(status))
      {
         std::cout << (boost::format("download loader error. code 0x%x\n")%status).str();
         goto exit;
      }

      std::cout << "wait for preloader running." << std::endl;
      status = xflash_waitfor_com(filter, 1, port_name);
      if(FAIL(status))
      {
         std::cout << (boost::format("wait for loader com error. code 0x%x\n")%status).str();
         goto exit;
      }

      std::cout << "connect preloader." << std::endl;
      status = xflash_connect_loader(hs, port_name);
      if(FAIL(status))
      {
         std::cout << (boost::format("connect loader error. code 0x%x\n")%status).str();
         goto exit;
      }
   }
   else
   {
      std::cout << "boot from preloader." << std::endl;
      uint32 is_support_empty_boot = FALSE;
      status = xflash_device_control(hs, DEV_LOADER_SUPPORT_EMPTY_BOOT, 0, 0, &is_support_empty_boot, sizeof(uint32), 0);
      if(FAIL(status))
      {
         std::cout << (boost::format("DEV_LOADER_SUPPORT_EMPTY_BOOT error. code 0x%x\n")%status).str();
         goto exit;
      }
      if(!is_support_empty_boot)
      {
         std::cout << (boost::format("This version of preloader in device do not support empty boot. Press DOWNLOAD_KEY and retry.\n")).str();
         goto exit;
      }

      xflash_skip_loader_stage(hs);
   }

   std::cout << "download required partition images." << std::endl;

   set_stop_var(&stop_f);

   status = xflash_download_partition(hs, flist, count_needed, &cbs);
   if(FAIL(status))
   {
      std::cout << (boost::format("download partition error. code 0x%x\n")%status).str();
      goto exit;
   }

   std::cout << "jump to: "<< fastboot_partition << std::endl;
   status = xflash_partition_execution(hs, (char8*)fastboot_partition.c_str());
   if(FAIL(status))
   {
      std::cout << (boost::format("jump to error. code 0x%x\n")%status).str();
      goto exit;
   }
exit:
   xflash_destroy_session(hs);
   std::cout << "END." << std::endl;
   return status;
}

void cmd_flash::run_script(STD_::string script_path_name, STD_::string device_sn)
{
   boost::filesystem::path script_path(script_path_name, (void*)boost::filesystem::native);
   if(boost::filesystem::exists(script_path))
   {
        STD_::fstream script(script_path_name.c_str(), std::ios::in);

      STD_::string line;
      while(std::getline(script, line))
      {
if(boost::all(line, boost::is_space()))
         {
            //jump blank line
            continue;
         }
         line += " -s ";
         line += device_sn;

         std::cout << "[system]: "<< line << std::endl;
            if(::system(line.c_str()) != 0)
         {
            std::cerr << "Error in run system command:" <<line << std::endl;
         }
      }
   }
   else
   {
      std::cerr << "Script file not found: " <<script_path_name << std::endl;
   }
   return;
}

boost::container::map<STD_::string, STD_::string> cmd_flash::session_map;
boost::mutex cmd_flash::session_mtx;

status_t cmd_flash::execute(boost::container::map<STD_::string,STD_::string> args)
{
   status_t status = STATUS_OK;
   STD_::string str_script = args["script-file-name"];

   QProcess proc;
   QStringList arg;
   arg << "devices";

   proc.setProcessChannelMode(QProcess::MergedChannels);

   std::cout << "[system]: fastboot devices" << std::endl;
   proc.start("fastboot.exe", arg);

   if(!proc.waitForStarted())
   {
      std::cout << "start fastboot process error." <<std::endl;
      return STATUS_THREAD;
   }

   proc.closeWriteChannel();
   QByteArray out;

   while(proc.waitForReadyRead())
   {
      out += proc.readAllStandardOutput();
   }
   //std::cout << out.data();

   proc.waitForFinished();

   stringstream s;
   s << out.data();
   STD_::string line;

   STD_::string device_sn;

   {
      boost::mutex::scoped_lock lock(session_mtx);
      while(std::getline(s, line))
      {
         if(boost::algorithm::starts_with(line, "\r")
            || boost::algorithm::starts_with(line, "\n"))
         {
            continue;
         }
         std::cout << line << std::endl;
         std::vector<string> strs;
         //split
         //line is like this:  "0123456789ABCDEF   fastboot"
         boost::split(strs, line, boost::is_any_of("\t "));
         //only process one device.
         if(session_map.count(strs[0]) == 0)
         {
            device_sn = strs[0];
            session_map[device_sn] = "new_device";
            break;
         }
      }

      if(device_sn.empty())
      {
         return STATUS_OK;
      }
   }

   run_script(str_script, device_sn);

   {
      boost::mutex::scoped_lock lock(session_mtx);
      session_map.erase(device_sn);
   }

   return status;
}