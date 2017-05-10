#include "commands.h"
#include "error_code.h"
#include <boost/foreach.hpp>
#include <iostream>
#pragma warning(disable:4996) //Function call with parameters that may be unsafe
#include <boost/exception/all.hpp>
#include "core/cmds_impl.h"
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
using namespace boost;

//use this for ctrl+C
static string g_device_id;
void register_break_handler();

commands::commands(void)
{
}

commands::~commands(void)
{
}


status_t commands::execute(string command, vector<string>& arguments,
                           boost::container::map<string, string>& options)
{
   status_t status = STATUS_OK;
   register_break_handler();
   try
   {
      if(command == "enter-fastboot")
      {
         boost::container::map<string, string> args;
         if(arguments.size() == 0)
         {
            std::cout << "error: check command arguments count." <<std::endl;
            return STATUS_INVALID_PARAMETER;
         }
         args["scatter-file-name"] = arguments[0];
         if(arguments.size() == 2) //has auth file
         {
            args["auth-file-name"] = arguments[1];
         }
         else
         {
            args["auth-file-name"] = "";
         }

         boost::shared_ptr<ICommand> cmd = boost::make_shared<cmd_enter_fastboot>();
         status = cmd->execute(args);
      }
      else if(command == "flash")
      {
         boost::container::map<string, string> args;
         if(arguments.size() < 2)//scatter file & run script.
         {
            std::cout << "error: check command arguments count." <<std::endl;
            return STATUS_INVALID_PARAMETER;
         }
         args["scatter-file-name"] = arguments[0];
         args["script-file-name"] = arguments[1];

         if(arguments.size() == 3) //has auth file
         {
            args["auth-file-name"] = arguments[2];
         }
         else
         {
            args["auth-file-name"] = "";
         }

         BOOL loop_infinit = FALSE;
         if(options.count("multi-devices") != 0)
         {
            loop_infinit = TRUE;
         }

         do
         {
            //this_thread::sleep_for(boost::chrono::milliseconds(3000));
             boost::shared_ptr<ICommand> cmd = boost::make_shared<cmd_enter_fastboot>();
            status = cmd->execute(args);
            if (FAIL(status))
            {
               std::cerr << "ERROR: enter fastboot code: " << status << std::endl;
            }

            this_thread::sleep_for(boost::chrono::milliseconds(3000));
             cmd = boost::make_shared<cmd_flash>();
            //create fastboot dispatch thread.
            boost::shared_ptr<boost::thread> worker_thr = boost::make_shared<boost::thread>(boost::bind(&ICommand::execute, cmd, args));

            if(!loop_infinit)
            {
               worker_thr->join();
            }
            else
            {
               worker_thr->detach();
            }
         }while(loop_infinit);
      }
   }
   catch(std::exception &e)
   {
      std::cout << boost::diagnostic_information(e) << std::endl;
      return STATUS_ERR;
   }
   return status;
}

#if defined(WIN32)
#include "windows.h"

BOOL ctrl_handler(uint32 ctrl)
{
   std::vector<string> args;
   boost::container::map<string, string> opts;
   switch(ctrl)
   {
      // Handle the CTRL-C signal.
   case CTRL_C_EVENT:
   case CTRL_CLOSE_EVENT:
      {
         //commands::execute("stop", args, opts);
         //return TRUE;
      }
      return FALSE;
   default:
      return FALSE;
   }
}

void register_break_handler()
{
   ::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
   SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrl_handler, TRUE);
}

#else

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void ctrl_handler(int ctrl)
{
   std::vector<string> args;
   boost::container::map<string, string> opts;
   switch(ctrl)
   {
      // Handle the CTRL-C signal.
   case SIGINT:
   case SIGTSTP:
      {
         //commands::execute("stop", args, opts);
      }
   default:
	 break;
   }
   return;
}

void register_break_handler()
{
   signal(SIGINT, ctrl_handler);
   signal(SIGTSTP, ctrl_handler);
}
#endif

