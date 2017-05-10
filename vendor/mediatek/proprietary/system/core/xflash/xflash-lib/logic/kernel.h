#pragma once
#include "common/common_include.h"
#include "xflash_struct.h"
#include <boost/thread/recursive_mutex.hpp>

typedef enum kernel_stage_e
{
   KSTAGE_NOT_START,
   KSTAGE_BROM,
   KSTAGE_LOADER,
   KSTAGE_FINISHED
}kernel_stage_e;

class boot_rom;
class ITransmission_engine;


class session_t
{
public:
   string port_name;
   boost::shared_ptr<ITransmission_engine> channel;
   boost::shared_ptr<boot_rom> brom;
   kernel_stage_e kstage;
   boost::recursive_mutex mtx;
};

class kernel
{
public:
   static int startup();
   static void set_log_level(enum logging_level level);
   static void cleanup();
   static kernel* instance();
   /*synchonized*/HSESSION create_new_session(HSESSION prefer);
   /*synchonized*/void delete_session(HSESSION h);
   /*synchonized*/boost::shared_ptr<session_t> get_session(HSESSION h);

private:
   //HSESSION to session map.
   boost::container::map<HSESSION, boost::shared_ptr<session_t> > session_map;
   HSESSION get_handle();
private:
   kernel(void);
   ~kernel(void);
   boost::mutex mtx;
};

