#pragma once
#include <string>
//#include <boost/log/trivial.hpp>
#include "type_define.h"
#include "xflash_struct.h"

//#include <boost/atomic.hpp>
#pragma warning(disable: 4996) //Function call with parameters that may be unsafe

#define Tc(x) #x
#define DEFAULT_CALL_LOG_INFO(line) (std::string(__FUNCTION__)+" #"+__FILE__+" (line:"+Tc(line)+")")
#define DBG_CODE_LINE DEFAULT_CALL_LOG_INFO(__LINE__)

#define CALL_LOG_FILE(line) (std::string(" #")+__FILE__+" (line:"+Tc(line)+")")

#define FILE_POS (std::string(" !")+std::string(__FUNCTION__)+CALL_LOG_FILE(__LINE__))

#define CALL_LOGV call_log _instance(std::string(__FUNCTION__), CALL_LOG_FILE(__LINE__), ktrace);
#define CALL_LOGD call_log _instance(std::string(__FUNCTION__), CALL_LOG_FILE(__LINE__), kdebug);
#define CALL_LOGI call_log _instance(std::string(__FUNCTION__), CALL_LOG_FILE(__LINE__), kinfo);

class call_log
{
public:
   call_log(std::string function, std::string file, enum logging_level level);
   ~call_log(void);
private:
   call_log(void);
   uint32 id;
   enum logging_level level;
   std::string mfunction;
   //static boost::atomic<long> counter;
};

