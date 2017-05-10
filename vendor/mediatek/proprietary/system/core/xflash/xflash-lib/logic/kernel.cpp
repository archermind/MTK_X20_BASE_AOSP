#include "logic/kernel.h"
#include <boost/locale/encoding.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/info.hpp>
#include <boost/locale/util.hpp>
#include <common/utils.h>
#include "config/lib_config_parser.h"


kernel::kernel(void)
{
}

kernel::~kernel(void)
{
}

kernel* kernel::instance()
{
   //explict declare singleton.
   //do not move this to global. because the globals destruction sequence is uncertain.
   static kernel the_core;
   return &the_core;
}

int kernel::startup()
{
   uint32 endian = 0xFFFF0000;
   if((uint8)endian == 0xFF)
   {
      //cout <<"Do not support Big endian currently."<< std::endl ;
      LOGF<<"Do not support Big endian currently. Quit";
      return -1;
   }
   //boost xml_read do not support CHN path. so set locale.
   std::locale::global(std::locale(std::locale(), "", std::locale::all ^ std::locale::numeric));

   lib_config_parser lib_cfg(LIB_CFG_FILE);
   string str_log_level = lib_cfg.get_value("log_level");
   enum logging_level log_level = kerror;
   try
   {
      if(!str_log_level.empty())
      {
         log_level = (enum logging_level)boost::lexical_cast<uint32>(str_log_level);
      }
   }
   catch(boost::bad_lexical_cast&)
   {
   }

   zlog_namespace::set_log_level(log_level);

   //explict call this to avoid Multi-Thread race condition lock check.
   kernel::instance();
   return 0;
}

void kernel::cleanup()
{
   LOGI<<"clean up.";
}

void kernel::set_log_level(enum logging_level level)
{
   zlog_namespace::set_log_level(level);
   LOGI<<(boost::format("set global log level: %d. ##trace[0] ,debug[1], info[2], warning[3], err[4], fatal[5]")%(uint32)level).str();
}

HSESSION kernel::get_handle()
{
   static HSESSION seed = 1;
   //find a empty slot.
   while(true)
   {
      if(session_map.count(seed) == 0)
      {
         if(seed != INVALID_HSESSION_VALUE)
         {
            break;
         }
      }
      ++seed;
   }

   return seed;
}

HSESSION kernel::create_new_session(HSESSION prefer)
{
   boost::mutex::scoped_lock lock(mtx);
   boost::shared_ptr<session_t> session = boost::make_shared<session_t>();

   if(prefer != INVALID_HSESSION_VALUE
      && (session_map.count(prefer) == 0))
   {
      session_map[prefer] = session;
   }
   else
   {
      prefer = get_handle();
      session_map[prefer] = session;
   }

   LOGI<<(boost::format("create new hsession 0x%x")%prefer).str();
   return prefer;
}

void kernel::delete_session(HSESSION h)
{
   boost::mutex::scoped_lock lock(mtx);
   if(session_map.count(h) != 0)
   {
      session_map.erase(h);
      LOGI<<(boost::format("delete hsession 0x%x")%h).str();
   }
}

boost::shared_ptr<session_t> kernel::get_session(HSESSION h)
{
   boost::mutex::scoped_lock lock(mtx);
   if(session_map.count(h) != 0)
   {
      return session_map[h];
   }
   else
   {
      //empty object, caller must valid it with if(!session);
      return boost::shared_ptr<session_t>();
   }
}