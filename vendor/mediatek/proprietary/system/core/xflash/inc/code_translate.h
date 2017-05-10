#ifndef __CODE_TRANSLATE__
#define __CODE_TRANSLATE__

#include <string>
#include "type_define.h"

class code_translate
{
public:
   static std::string err_to_string(uint32 error_code);
   static std::string dev_ctrl_to_string(uint32 dc);
};


#endif