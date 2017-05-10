#include "common/common_include.h"
#include "brom/boot_rom_logic.h"
#include "transfer/comm_engine.h"
#include "arch/IUsbScan.h"
#include "brom/boot_rom.h"
#include "brom/boot_rom_cmd.h"
#include "logic/connection.h"
#include "boot_rom_sla_cb.h"
#include "common/zbuffer.h"

using namespace STD_;

boot_rom_logic::boot_rom_logic(void)
{
}

boot_rom_logic::~boot_rom_logic(void)
{
}

static std::string to_string(uint8* data, uint32 length)
{
   string s = (boost::format("[")).str();
   length = length <= 16 ? length : 16;
   for(uint32 i=0; i<length; ++i)
   {
      s += (boost::format("%02x ")%(uint32)(data[i])).str();
   }
   s += "]";
   return s;
}

status_t boot_rom_logic::security_verify_connection(boost::shared_ptr<boot_rom> brom, STD_::string auth, string cert)
{
   CALL_LOGD;
   status_t status = STATUS_OK;

#if defined(SECURITY_VERIFY_CONNECTION_SUPPORT)
   uint8 preloader_ver;
   status = brom->get_preloader_version(&preloader_ver);
   if(SUCCESSED(status))
   {
      LOGI <<"Preloader exist. skip connection verification.";
      return STATUS_OK;
   }

   uint8* me_id = 0;
   status = brom->get_get_me_id(&me_id);
   if(FAIL(status))
   {
      return status;
   }

   LOGI <<(boost::format("ME ID: %s")%to_string(me_id, 16)).str();

   if(!cert.empty())
   {
      filesystem::path cert_path(cert, (void*)filesystem::native);
      if(boost::filesystem::exists(cert_path))
      {
         std::fstream img(cert_path.c_str(), std::ios::binary|std::ios::in);
         uint32 filesz = (uint32)(filesystem::file_size(cert_path));

         //shared_ptr<uint8[]> data = make_shared<uint8[]>(filesz);
         boost::shared_ptr<zbuffer> data = boost::make_shared<zbuffer>(filesz);
         img.read((char8*)data->get(), filesz);

         status = brom->send_cert_file(data->get(), filesz);
         if(FAIL(status))
         {
            return status;
         }
      }
      else
      {
         LOGW << (boost::format("File Not Found: %s")%cert_path).str();
         return STATUS_FILE_NOT_FOUND;
      }
   }

   uint32 security_cfg = 0;
   status = brom->get_security_config(&security_cfg);
   if(FAIL(status))
   {
      return status;
   }

   BOOL enable_sbc = (security_cfg & SEC_CFG_SBC_EN)?TRUE:FALSE;
   BOOL enable_sla = (security_cfg & SEC_CFG_SLA_EN)?TRUE:FALSE;
   BOOL enable_daa = (security_cfg & SEC_CFG_DAA_EN)?TRUE:FALSE;

   if(enable_sla || enable_daa)
   {
      if(!auth.empty())
      {
         filesystem::path auth_path(auth, (void*)filesystem::native);
         if(boost::filesystem::exists(auth_path))
         {
            std::fstream img(auth_path.c_str(), std::ios::binary|std::ios::in);
            uint32 filesz = (uint32)(filesystem::file_size(auth_path));

            //shared_ptr<uint8[]> data = make_shared<uint8[]>(filesz);
            boost::shared_ptr<zbuffer> data = boost::make_shared<zbuffer>(filesz);
            img.read((char8*)data->get(), filesz);

            status = brom->send_auth_file(data->get(), filesz);
            if(FAIL(status))
            {
               return status;
            }
         }
         else
         {
            LOGE << (boost::format("Auth File Not Found: %s")%auth).str();
            return STATUS_FILE_NOT_FOUND;
         }
      }
      else
      {
         LOGE << "BROM said that Need AUTH file.";
         return STATUS_FILE_NOT_FOUND;
      }

      if(enable_sla)
      {
         status = brom->qualify_host();
         if(FAIL(status))
         {
            return status;
         }
      }
   }
#endif
   return status;
}

void boot_rom_logic::test(void)
{
   UsbScan usbScan;
   usb_device_info dev_info;
   usb_filter filter;
   filter.add_filter("USB\\VID_0E8D&PID_2000");
   filter.add_filter("USB\\VID_0E8D&PID_0003");

   cout << "Wait for device. " << endl;
   usbScan.wait_for_device(&filter, &dev_info, 60000);

   cout <<"Port: " <<dev_info.port_name << endl;
   cout <<"Device Class: " <<dev_info.device_class << endl;
   cout <<"Friendly Name: " <<dev_info.friendly_name << endl;
   cout <<"Instance Id: " <<dev_info.device_instance_id << endl;
   cout <<"Interface Symbolic Link name: " <<dev_info.symbolic_link_name << endl;
   cout << "Device Name: " <<dev_info.dbcc_name << endl;

   boost::shared_ptr<ITransmission_engine> channel = boost::make_shared<comm_engine>("115200");
   channel->open(dev_info.symbolic_link_name);

   boot_rom boot;
   boot.set_transfer_channel(channel);
   boot.connect();

   chip_id_struct chip;
   boot.get_chip_id(&chip);

   section_block_t section;

   BOOST_ASSERT(STATUS_OK == boot.send_loader(&section));
   BOOST_ASSERT(STATUS_OK == boot.jump_to_loader(section.jmp_address));


   return;
}
