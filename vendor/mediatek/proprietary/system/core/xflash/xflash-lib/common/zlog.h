#pragma once
#pragma warning(disable: 4996) //Function call with parameters that may be unsafe

#if defined (_WIN32) || defined(WIN32) || defined (Q_OS_WIN) || defined(Q_OS_MAC)

#include <fstream>
#include <iomanip>
//#include <boost/smart_ptr/shared_ptr.hpp>
//#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include "common/call_log.h"
#include "xflash_struct.h"


BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(stub_attr, "Stub", std::string)

#define DA_TAG "Download Agent"
#define FL_TAG "FlashtoolLib"


#define LOGV\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::trace))
#define LOGD\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::debug))
#define LOGI\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::info))
#define LOGW\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::warning))
#define LOGE\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::error))
#define LOGF\
   BOOST_LOG_SEV((*glb_log::instance()),(boost::log::trivial::fatal))

#define LOG(level)\
   BOOST_LOG_SEV((*glb_log::instance()),(static_cast<boost::log::trivial::severity_level>(level)))


#define DALOGV\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::trace))
#define DALOGD\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::debug))
#define DALOGI\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::info))
#define DALOGW\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::warning))
#define DALOGE\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::error))
#define DALOGF\
   BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::fatal))


namespace zlog_namespace
{
   void set_log_level(enum logging_level level = kinfo);
}

void log_test();


typedef boost::log::sources::severity_channel_logger<boost::log::trivial::severity_level> zlog;
class glb_log
{
public:
   static boost::shared_ptr<zlog>& instance(void);

private:
   static boost::mutex mtx;
   static boost::shared_ptr<zlog> inst;
private:
   glb_log(){}
   static void init();

};

class da_log
{
public:
   static boost::shared_ptr<zlog>& instance(void);

private:
   static boost::mutex mtx;
   static boost::shared_ptr<zlog> inst;
private:
   da_log(){}
   static void init();
};



/**************************************************************/
#else
//gcc v4.4.7 and later version do not use this. just use as win32.
//this is used only because gcc4.4.3 do not support boost::log. and customer's linux is out of date.


#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "xflash_struct.h"
#include "common/common_include.h"

using namespace std;
using namespace boost;

class zlog
{
public:
   zlog(std::string perfix);
   ~zlog(void);

   zlog& operator<<(const char *s);
   zlog& operator<<(unsigned char *s);
   zlog& operator<<(char *s);
   zlog& operator<<(std::string s);
public:
   zlog& get_inst(enum logging_level level);
   enum logging_level get_log_level();
   void set_log_level(enum logging_level level = kinfo);
private:
   zlog();
   enum logging_level log_level;
   enum logging_level cur_log_level;
   boost::shared_ptr<STD_::ofstream> fout;
   boost::recursive_mutex mtx;
};


#define LOGV\
   zlog_inst().get_inst(ktrace)
#define LOGD\
   zlog_inst().get_inst(kdebug)
#define LOGI\
   zlog_inst().get_inst(kinfo)
#define LOGW\
   zlog_inst().get_inst(kwarning)
#define LOGE\
   zlog_inst().get_inst(kerror)
#define LOGF\
   zlog_inst().get_inst(kfatal)

#define LOG(level)\
   zlog_inst().get_inst(level)

#define DALOGV\
   zlog_inst().get_inst(ktrace)
#define DALOGD\
   zlog_inst().get_inst(kdebug)
#define DALOGI\
   zlog_inst().get_inst(kinfo)
#define DALOGW\
   zlog_inst().get_inst(kwarning)
#define DALOGE\
   zlog_inst().get_inst(kerror)
#define DALOGF\
   zlog_inst().get_inst(kfatal)

namespace zlog_namespace
{
   void set_log_level(enum logging_level level = kinfo);
}


extern zlog& zlog_inst();
#endif
