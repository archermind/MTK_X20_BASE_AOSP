#include "common/call_log.h"
#include "common/zlog.h"
# pragma warning (disable:4819) //unicode warning
#include <boost/format.hpp>

//boost::atomic<long> call_log::counter(0);

call_log::call_log(std::string function, std::string file, enum logging_level level)
: level(level), mfunction(function)
{
   static uint32 counter = 0;
   id = ++counter;
   LOG(level) << (boost::format("-->[C%d] %s")%id%(mfunction+file)).str();
}

call_log::~call_log(void)
{
   LOG(level) << (boost::format("<--[C%d] %s")%id%mfunction).str();
}
