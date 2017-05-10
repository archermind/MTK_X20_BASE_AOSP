#ifndef __LIB_UTILS__
#define __LIB_UTILS__
#include <boost/filesystem.hpp>
#include <string>

uint64 to_hex(std::string s);
uint64 to_int(std::string s);
std::string to_str(uint64 n);

boost::filesystem::path get_temp_directory();
boost::filesystem::path get_output_directory();
boost::filesystem::path get_config_directory();
boost::filesystem::path get_sub_directory(std::string relative);

#endif