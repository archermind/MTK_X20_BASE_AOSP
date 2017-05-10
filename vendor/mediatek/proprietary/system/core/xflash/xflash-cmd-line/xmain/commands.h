#pragma once

#include <string>
#include <vector>
#include <boost/container/map.hpp>
#include "type_define.h"
using namespace std;

class commands
{
public:
   commands(void);
   ~commands(void);
   static status_t execute(string command,
                           vector<string>& arguments,
                           boost::container::map<string, string>& options);
};
