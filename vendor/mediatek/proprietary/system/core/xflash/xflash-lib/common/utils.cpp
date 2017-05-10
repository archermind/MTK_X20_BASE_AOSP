#include "common/common_include.h"
#include "common/utils.h"
#include <boost/regex.hpp>

#define min(a, b)   ((a) < (b) ? (a) : (b))

uint64 to_hex(std::string s)
{
   std::stringstream stream;

   uint64 n = 0;
   stream << s;
   stream >>std::hex>> n;
   return n;
}
uint64 to_int(std::string s)
{
   std::stringstream stream;

   uint64 n = 0;
   stream << s;
   stream >>std::dec>> n;
   return n;
}

std::string to_str(uint64 n)
{
   std::string str;
   std::stringstream ss;

   ss<<std::hex<<n;
   ss>>str;

   return str;
}

boost::filesystem::path get_temp_directory()
{
   boost::filesystem::path currentPath = boost::filesystem::initial_path();
   boost::filesystem::path tPath = currentPath / "temp";
   if(!boost::filesystem::exists(tPath))
   {
      boost::filesystem::create_directory(tPath);
   }

   return tPath;
}

boost::filesystem::path get_output_directory()
{
   boost::filesystem::path currentPath = boost::filesystem::initial_path();
   boost::filesystem::path tPath = currentPath / "output";
   if(!boost::filesystem::exists(tPath))
   {
      boost::filesystem::create_directory(tPath);
   }

   return tPath;
}

boost::filesystem::path get_sub_directory(std::string relative)
{
   boost::filesystem::path currentPath = boost::filesystem::initial_path();
   boost::filesystem::path tPath = currentPath / relative;
   if(!boost::filesystem::exists(tPath))
   {
      boost::filesystem::create_directory(tPath);
   }

   return tPath;
}

boost::filesystem::path get_config_directory()
{
   boost::filesystem::path currentPath = boost::filesystem::initial_path();
   boost::filesystem::path tPath = currentPath / "config";
   if(!boost::filesystem::exists(tPath))
   {
      boost::filesystem::create_directory(tPath);
   }

   return tPath;
}

