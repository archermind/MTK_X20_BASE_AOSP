#include "common/common_include.h"
#include "logic/connection.h"
#include "logic/kernel.h"
#include "loader/file/loader_image.h"
#include "brom/boot_rom.h"
#include "common/zbuffer.h"
#include "transfer/ITransmission.h"
#include "brom/boot_rom.h"
#include "brom/boot_rom_logic.h"
#include "transfer/comm_engine.h"
#include "config/lib_config_parser.h"
#include "arch/IUsbScan.h"
#include "code_translate.h"

connection::connection(void)
{
}

connection::~connection(void)
{
}

//explict declare
boost::recursive_mutex connection::com_mtx;

HSESSION connection::create_session(HSESSION prefer)
{
   CALL_LOGV;
   HSESSION hs = kernel::instance()->create_new_session(prefer);
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);

   session->brom = boost::make_shared<boot_rom>();

   lib_config_parser lib_cfg(LIB_CFG_FILE);
   string baud_rate = lib_cfg.get_value("uart_connection_baudrate");
   session->channel = boost::make_shared<comm_engine>(baud_rate);

   session->kstage = KSTAGE_NOT_START;

   return hs;
}

status_t connection::destroy_session(HSESSION hs)
{
   CALL_LOGV;
   kernel::instance()->delete_session(hs);
   return STATUS_OK;
}

status_t connection::waitfor_com(char8** filter, uint32 count, STD_::string& port_name)
{
   CALL_LOGV;
   if(filter == NULL || count == 0)
   {
      return STATUS_INVALID_PARAMETER;
   }
   UsbScan usbScan;
   usb_device_info dev_info;
   usb_filter vidpid;
   for(unsigned int idx=0; idx<count; ++idx)
   {
      //add like "USB\\VID_0E8D&PID_0003"
      vidpid.add_filter(filter[idx]);
   }

   boost::recursive_mutex::scoped_lock lock(com_mtx);
   LOGI<<"wait for device..";

   if(!usbScan.wait_for_device(&vidpid, &dev_info, 600000))
   {
      return STATUS_USB_SCAN_ERR;
   }

   port_name = dev_info.symbolic_link_name;
   return STATUS_OK;
}


status_t connection::connect_brom(HSESSION hs, string port_name, string auth, string cert)
{
   CALL_LOGD;
   try
   {
      LOGI<<"(1/2)connecting brom.";
      LOGI <<(boost::format("symbolic link: [%s]")%port_name).str();
      boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
      if(!session)
      {
         LOGE<<"invalid session.";
         return STATUS_INVALID_SESSION;
      }

      boost::recursive_mutex::scoped_lock lock(session->mtx);
      if(session->kstage != KSTAGE_NOT_START)
      {
         LOGE<<"Can not connect BROM again.";
         return STATUS_INVALID_STAGE;
      }

      session->port_name = port_name;

      session->channel->open(session->port_name);
      session->brom->set_transfer_channel(session->channel);
      status_t status = session->brom->connect();
      if(SUCCESSED(status))
      {
         session->kstage = KSTAGE_BROM;
      }
      else
      {
         return status;
      }
      LOGI<<"(2/2)security verify tool and DA.";
      return boot_rom_logic::security_verify_connection(session->brom, auth, cert);
   }
   catch(std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_ERR;
   }
}

status_t connection::download_loader(HSESSION hs,
                                     string preloader_name)
{
   CALL_LOGD;
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
   if(!session)
   {
      LOGE<<"invalid session.";
      return STATUS_INVALID_SESSION;
   }

   boost::recursive_mutex::scoped_lock lock(session->mtx);
   if(session->kstage != KSTAGE_BROM)
   {
      LOGE<<"connect BROM first.";
      return STATUS_INVALID_STAGE;
   }

   chip_id_struct chip;
   session->brom->get_chip_id(&chip);

   status_t status;
   LOGI<<"(1/7)connecting loader.";
   loader_image loader_img;


   LOGI<<"(2/7)read preloader image file.";
   LOGI <<(boost::format("loader: [%s]")%preloader_name).str();
   status = loader_img.load(preloader_name);
   if(FAIL(status))
   {
      return status;
   }

   section_block_t section;

   LOGI<<"get loader data.";
   status = loader_img.get_section_data(&section);
   if(status != STATUS_OK)
   {
      return status;
   }

   LOGI<<"send loader data to brom.";
   status = session->brom->send_loader(&section);
   if(status != STATUS_OK)
   {
      return status;
   }

   LOGI<<"jump to loader.";
   status = session->brom->jump_to_loader(section.jmp_address);
   if(status != STATUS_OK)
   {
      return status;
   }

   return STATUS_OK;
}

status_t connection::connect_loader(HSESSION hs, string port_name)
{
   CALL_LOGD;
   try
   {
      LOGI<<"(1/2)connecting preloader.";
      LOGI <<(boost::format("symbolic link: [%s]")%port_name).str();
      boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
      if(!session)
      {
         LOGE<<"invalid session.";
         return STATUS_INVALID_SESSION;
      }

      boost::recursive_mutex::scoped_lock lock(session->mtx);
      if(session->kstage != KSTAGE_BROM)
      {
         LOGE<<"connect BROM first.";
         return STATUS_INVALID_STAGE;
      }

      session->port_name = port_name;

      session->channel = boost::make_shared<comm_engine>();
      session->channel->open(session->port_name);
      session->brom->set_transfer_channel(session->channel);
      status_t status = session->brom->connect();
      if(SUCCESSED(status))
      {
         session->kstage = KSTAGE_LOADER;
      }

      return status;
   }
   catch(std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_ERR;
   }
}

status_t connection::skip_loader_stage(HSESSION hs)
{
   CALL_LOGD;
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);

   boost::recursive_mutex::scoped_lock lock(session->mtx);
   session->kstage = KSTAGE_LOADER;

   return STATUS_OK;
}

status_t connection::device_control(HSESSION hs,
                                    uint32 ctrl_code,
                                    void* inbuffer,
                                    uint32 inbuffer_size,
                                    void* outbuffer,
                                    uint32 outbuffer_size,
                                    uint32* bytes_returned)
{
   CALL_LOGD;
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
   if(!session)
   {
      LOGE<<"invalid session.";
      return STATUS_INVALID_SESSION;
   }
   LOGI << (boost::format("device control: %s code[0x%x]")%code_translate::dev_ctrl_to_string(ctrl_code)%ctrl_code).str();

   boost::recursive_mutex::scoped_lock lock(session->mtx);
   status_t status;

   if(session->kstage == KSTAGE_BROM || session->kstage == KSTAGE_LOADER)
   {
      status = session->brom->device_control(ctrl_code,
         inbuffer, inbuffer_size,
         outbuffer, outbuffer_size,
         bytes_returned);
   }
   else
   {
      status = STATUS_INVALID_STAGE;
   }
   return status;
}