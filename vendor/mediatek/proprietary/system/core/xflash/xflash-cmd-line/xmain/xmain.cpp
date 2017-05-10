// client.cpp : Defines the entry point for the console application.
//
#include <string>
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/error_info.hpp>
//#pragma warning(disable:4996)
#include <boost/algorithm/string.hpp>
#include <boost/container/map.hpp>
# pragma warning (disable:4819) //unicode warning
#include <boost/format.hpp>
#include "commands.h"
#include "xflash_api.h"

using namespace boost;
using namespace boost::program_options;

#define VERSION "1.1601.0.0"

int main(int argc, char* argv[])
{
   try
   {
      //boost xml_read do not support CHN path. so set locale.
      std::locale::global(std::locale(std::locale(), "", std::locale::all ^ std::locale::numeric));

        std::string commands_desc;
      commands_desc += "Commands:\n";
      commands_desc += (format("%|-40s| %s")%"  enter-fastboot <scatter> [auth]"%"\tDevice enter fastboot mode.\n").str();
      commands_desc += (format("%|-40s| %s")%"  flash <scatter> <script> [-M] [auth]"%"\tRun fastboot command script.\n").str();


      options_description desc("Allowed options");
      desc.add_options()
         ("help,h", "Command List")
         ("version,v", "Program version.")
         ("multi-devices,M", "Multi-download via fastboot.")
         //("id,s", value<string>(), "Specify device identifier.")
         ;

      options_description actual_desc("All options");
      actual_desc.add_options()
         ("help,h", "Command List")
         ("version,v", "Program version.")
         ("multi-devices,M", "Multi-download via fastboot.")
         ("command,c", value<string>(), "Major commands")
         ("arguments,a", value<vector<string> >(), "Arguments list")
         //("id,s", value<string>(), "Specify device identifier.")
         ;

      positional_options_description p;
      p.add("command", 1).add("arguments", -1);

      variables_map vm;
      try
      {
         store(command_line_parser(argc, argv).options(actual_desc).positional(p).run(), vm);
      }
      catch(boost::exception& e)
      {
         std::cout << boost::diagnostic_information(e) <<endl;
         return -1;
      }
      notify(vm);

      xflash_startup();

      unsigned int result = 0;
      if (vm.count("help"))
      {
         std::cout << desc;
         std::cout << commands_desc;
      }
      else if (vm.count("version"))
      {
         std::cout << "Version: " << VERSION << std::endl;

         dll_info_t info;
         xflash_get_lib_info(&info);

         std::cout << "Lib version: " << info.version << std::endl
                  << "Lib build: " << info.build_data << std::endl;
      }
      else if (vm.count("command"))
      {
            std::vector<std::string> args;
         if (vm.count("arguments"))
         {
            args = vm["arguments"].as<vector<std::string> >();
         }

            boost::container::map<std::string, std::string> opts;
         /*if (vm.count("id"))
         {
            opts["id"] = vm["id"].as<string>();
         }
         */
         if (vm.count("multi-devices"))
         {
            opts["multi-devices"] = "yes";
         }
            result = commands::execute(vm["command"].as<std::string>(), args, opts);
      }
      else
      {
         std::cout << desc;
         std::cout << commands_desc;
      }

      xflash_cleanup();
      return result;
   }
   catch(boost::exception& e)
   {
      std::cout << boost::diagnostic_information(e) <<endl;
      return -1;
   }

   return 0;
}

