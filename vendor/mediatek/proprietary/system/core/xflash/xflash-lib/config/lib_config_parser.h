#pragma once

#define LIB_CFG_FILE "lib.cfg.xml"
#define CHIP_MAPPING_CFG_FILE "chip.mapping.cfg.xml"
#include <string>
#include "common/common_include.h"

class lib_config_parser
{
public:
   lib_config_parser(STD_::string f_name);
   STD_::string get_value(STD_::string key);
private:
   lib_config_parser();
   STD_::string file_name;
};