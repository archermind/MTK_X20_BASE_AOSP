#include "zlog.h"
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>

#if defined (_WIN32) || defined(WIN32) || defined(Q_OS_WIN) || defined(Q_OS_MAC)

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;


boost::mutex glb_log::mtx;
boost::shared_ptr<zlog> glb_log::inst;
//Flashtool log source
boost::shared_ptr<zlog>& glb_log::instance(void)
{
   if (!inst)
   {
      boost::lock_guard<boost::mutex> lock(mtx);
      if (!inst)
      {
         inst = boost::shared_ptr<zlog>(new zlog(keywords::channel=FL_TAG));
         init();
      }
   }
   return inst;
}
//init log sink.
void glb_log::init()
{
   //Flashtool log sink
   /*  Open this for flashtool lib.
   logging::add_file_log
      (
      keywords::open_mode = std::ios::app,
      //keywords::target = "logs",
      keywords::file_name = "./Log/FL_%Y%m%d-%H%M%S_%N.log",
      keywords::rotation_size = 10 * 1024 * 1024,
      keywords::format =
      (
         expr::stream
         << "["<< std::right<<std::setw(7)<<std::setfill('0')<<expr::attr< unsigned int >("LineID")
         <<"] ["<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S:%f")
         <<"] [Tid"<< expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
         <<"] ["<<logging::trivial::severity <<"] "
         << expr::smessage
      ),
      keywords::auto_flush = true,
      //keywords::filter = (expr::has_attr(tag_attr) && tag_attr == FL_TAG)
      keywords::filter = (expr::attr<std::string>("Channel")==FL_TAG)
      );

    */

   //Flashtool + DA
   logging::add_file_log
      (
      keywords::open_mode = std::ios::app,
      //keywords::target = "logs",
      keywords::file_name = "./Log/%Y%m%d-%H%M%S_%N.log",
      keywords::rotation_size = 10 * 1024 * 1024,
      keywords::format =
      (
         expr::stream
         << expr::if_(expr::attr<std::string>("Channel")==DA_TAG)
            [
               expr::stream << "%%\t\t\t"
            ]
         << "["<< std::right<<std::setw(7)<<std::setfill('0')<<expr::attr< unsigned int >("LineID")
         <<"] ["<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S:%f")
         <<"] [Tid"<< expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
         <<"] ["<<logging::trivial::severity <<"] "
         << expr::smessage
      ),
      keywords::auto_flush = true,
      keywords::filter = (expr::attr<std::string>("Channel")==FL_TAG || expr::attr<std::string>("Channel")==DA_TAG)
      );

   logging::add_common_attributes(); //global attribute.

   //inst->add_attribute("Tag", boost::log::attributes::constant<std::string>(FL_TAG));
}

boost::mutex da_log::mtx;
boost::shared_ptr<zlog> da_log::inst;
//DA log source
boost::shared_ptr<zlog>& da_log::instance(void)
{
   if (!inst)
   {
      boost::lock_guard<boost::mutex> lock(mtx);
      if (!inst)
      {
         inst = boost::shared_ptr<zlog>(new zlog(keywords::channel=DA_TAG));
         //do not generate da log sink.
         //init();
      }
   }
   return inst;
}
//init da log sink.
void da_log::init()
{
   //DA log sink
   logging::add_file_log
      (
      keywords::open_mode = std::ios::app,
      //keywords::target = "logs",  //where to put the rotated files.
      keywords::file_name = "./Log/DA_%Y%m%d-%H%M%S_%N.log",
      keywords::rotation_size = 10 * 1024 * 1024,
      keywords::format =
      (
         expr::stream
         << "["<< std::right<<std::setw(7)<<std::setfill('0')<<expr::attr< unsigned int >("LineID")
         <<"] ["<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S")
         <<"] [Tid"<< expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
         <<"] ["<<logging::trivial::severity <<"] "
         << expr::if_(expr::has_attr(stub_attr))
            [
               expr::stream << "[" << stub_attr << "] "
            ]
         << expr::smessage
      ),
      keywords::auto_flush = true,
      //keywords::filter = (expr::has_attr(tag_attr) && tag_attr == DA_TAG)
      keywords::filter = (expr::attr<std::string>("Channel")==DA_TAG)
      );
   logging::add_common_attributes(); //global attribute.

   //inst->add_attribute("Tag", boost::log::attributes::constant<std::string>(DA_TAG));
}

namespace zlog_namespace
{
   void set_log_level(enum logging_level level)
   {
      logging::core::get()->set_filter
         (
         logging::trivial::severity >= static_cast<boost::log::trivial::severity_level>(level)
         );
   }
}

void log_test()
{
   //set_log_level();
   //using namespace logging::trivial;
   //boost::log::sources::severity_logger<boost::log::trivial::severity_level> slg;
   //slg.add_attribute("Tag", boost::log::attributes::constant<std::string>(DA_TAG));


   // boost::log::sources::severity_logger<boost::log::trivial::severity_level> aslg;
   // aslg.add_attribute("Tag", boost::log::attributes::constant<std::string>(FL_TAG));

   CALL_LOGD;
   {
      BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::info)) << "DA log q";
      BOOST_LOG_SEV((*da_log::instance()),(boost::log::trivial::debug)) << "DA log q2";


      LOGW << "FL XXXX A warning message";

      LOGW << "FL X???? A warning message";

      // BOOST_LOG_SEV(aslg, trace) << "FL A trace message";
      // BOOST_LOG_SEV(aslg, warning) << "FL A warning message2";
   }
   return;
}

/**************************************************************/
#else

#include "zlog.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include "common/utils.h"
# pragma warning (disable:4819) //unicode warning
#include <boost/format.hpp>


zlog& zlog_inst()
{
   static zlog zlog_inst1("GLB");
   return zlog_inst1;
}

zlog::zlog(std::string perfix)
{
   boost::filesystem::path tPath = get_sub_directory("Log");
   if(!boost::filesystem::exists(tPath))
   {
      boost::filesystem::create_directory(tPath);
   }

   //boost::gregorian::date today = boost::gregorian::day_clock::local_day();
   std::string strTime = boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
   tPath /= perfix+"."+strTime+".log";

   fout = boost::shared_ptr<STD_::ofstream>(new ofstream(tPath.string().c_str(), std::ios::binary)) ;
}


zlog::~zlog(void)
{
}

zlog& zlog::operator<<(const char *s)
{
   boost::recursive_mutex::scoped_lock lock(mtx);
   if(cur_log_level >= log_level)
      *fout<<s;
   return *this;
}
zlog& zlog::operator<<(unsigned char *s)
{
   boost::recursive_mutex::scoped_lock lock(mtx);
   if(cur_log_level >= log_level)
      *fout<<s;
   return *this;
}
zlog& zlog::operator<<(char *s)
{
   boost::recursive_mutex::scoped_lock lock(mtx);
   if(cur_log_level >= log_level)
      *fout<<s;
   return *this;
}

zlog& zlog::operator<<(std::string s)
{
   boost::recursive_mutex::scoped_lock lock(mtx);
   if(cur_log_level >= log_level)
      *fout<<s;
   return *this;
}

void zlog::set_log_level(enum logging_level level)
{
   log_level = level;
}

zlog& zlog::get_inst(enum logging_level level)
{
   static unsigned int line = 0;
   boost::recursive_mutex::scoped_lock lock(mtx);
   cur_log_level = level;
   *this << (boost::format("\n[%|-07d|]")%line).str();
   ++line;
   return *this;
}

namespace zlog_namespace
{
   void set_log_level(enum logging_level level)
   {
      zlog_inst().set_log_level(level);
   }
}

#endif
