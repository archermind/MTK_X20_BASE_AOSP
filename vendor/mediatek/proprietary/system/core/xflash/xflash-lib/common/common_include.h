#ifndef	_COMMON_INCLUDE_H_
#define	_COMMON_INCLUDE_H_

#if defined(_WIN32) || defined(_LINUX)
namespace STD_ = std;
#else
namespace STD_ = std::__1;
#endif

//warning C4345: behavior change: an object of POD type
//constructed with an initializer of the form () will be default-initialized
#pragma warning(disable: 4345)
# pragma warning (disable:4996) //unsafe
#include "arch/host.h"
#include "common/zlog.h"
#include "common/types.h"
#include "common/call_log.h"
#include "error_code.h"
#include "common/utils.h"
# pragma warning (disable:4819) //unicode warning
#include <boost/format.hpp>
#include "common/runtime_exception.h"
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/assert.hpp>
#include <boost/container/map.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>



using std::string;
using namespace boost;

#endif