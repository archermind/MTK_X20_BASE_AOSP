#include "common/common_include.h"
#include "brom/boot_rom.h"
#include "transfer/ITransmission.h"
#include "brom/boot_rom_cmd.h"
#include <boost/asio.hpp> //for htonl function.
#include "xflash_struct.h"
#include "boot_rom_sla_cb.h"
#include "common/zbuffer.h"

#define START_SEQUENCE_CNT 4
static uint8 start_sequence[START_SEQUENCE_CNT] =
{
   0xA0, //BOOT_ROM_START_CMD1,
   0x0A, //BOOT_ROM_START_CMD2,
   0x50, //BOOT_ROM_START_CMD3,
   0x05  //BOOT_ROM_START_CMD4
};

static uint32 checksum_plain(uint8* data, uint32 length)
{
   uint32 chksum = 0;
   uint8* ptr = data;
   uint32 i = 0;

   for(i=0; i<(length&(~3)); i += sizeof(uint32))
   {
      chksum += (*(uint32*)(data+i));
   }
   if(i < length)//can not aligned by 4
   {
      ptr += i;
      for(i = 0; i<(length&3); i++)/// remain
      {
         chksum += ptr[i];
      }
   }
   return chksum;
}

boot_rom::boot_rom(void)
: timeout(30000)
,is_bootloader(FALSE)
{
}

boot_rom::~boot_rom(void)
{
}

void boot_rom::set_transfer_channel(boost::shared_ptr<ITransmission> channel)
{
   CALL_LOGD;
   com = channel;
}

status_t boot_rom::connect()
{
   CALL_LOGD;
   try
   {
      uint8 ack = 0;
      uint32 try_time = 0;
      LOGI <<"start handshake with device.";
      for(uint32 idx=0; idx<START_SEQUENCE_CNT; ++idx)
      {
         if(try_time > 600) //30sec = 50ms * 600
         {
            return STATUS_BROM_CMD_STARTCMD_FAIL;
         }
         com->send(&start_sequence[idx], 1, timeout);
         try
         {
            com->receive(&ack, 1, 50);
         }
         catch (std::exception&)
         {
            //uart sync often timeout.
            //because external board's uart always exist, but brom does not start running.
            //so continue trying.
            --idx;
            ++try_time;
            continue;
         }

         //evil design of preloader. should remove things like these stuff.
         //preloader receive first sync byte,then send back "READY", and then receive sync sequence.
         #define PRELOADER_SYNC_STRING "READY"
         if((ack == 'R') && (idx == 0))//first sync char of preloader.
         {
            uint8 ready[8];
            com->receive(ready, 4, timeout);//receive the rest "EADY"
            --idx;
            is_bootloader = TRUE;
            LOGI <<"preloader exist. connect.";
            continue;
         }

         if((uint8)(~ack) != start_sequence[idx])
         {
            LOGE <<(boost::format("BRom protocol error: ACK 0x%X != 0x%X")%(uint32)((uint8)(~ack))%(uint32)start_sequence[idx]).str();
            BOOST_THROW_EXCEPTION(runtime_exception(string("BRom pototcol error.")));
         }

         LOGD <<(boost::format("send 0x%02X. receive 0x%02X")%((uint32)start_sequence[idx])%((uint32)ack)).str();
      }
   }
   catch (std::exception& e)
   {
      LOGE << "brom connect exception: ";
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_STARTCMD_FAIL;
   }

   return STATUS_OK;
}

status_t boot_rom::get_chip_id(chip_id_struct* id)
{
   CALL_LOGD;
   LOGI << "get chip id " ;
   try
   {
      uint8 cmd = BROM_CMD_GET_HW_CODE;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("get hw id error. return 0x%X")%(uint32)(ack)));
      }

      read16(&id->hw_code);
      uint16 status;
      read16(&status);

      cmd = BROM_CMD_GET_HW_SW_VER;

      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("get sw version error. return 0x%X")%(uint32)(ack)));
      }

      read16(&id->hw_sub_code);
      read16(&id->hw_version);
      read16(&id->sw_version);
      read16(&status);
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_BBCHIP_HW_VER_INCORRECT;
   }

   LOGI << (boost::format("chip id: hw_code[0x%X] hw_sub_code[0x%X] hw_version[0x%X] sw_version[0x%X]")\
      %(uint32)id->hw_code%(uint32)id->hw_sub_code%(uint32)id->hw_version%(uint32)id->sw_version).str();

   return STATUS_OK;
}

status_t boot_rom::send_loader(section_block_t* loader)
{
   CALL_LOGD;
   LOGI << "send DA data to boot rom. " ;
   try
   {
      uint8 cmd = BROM_CMD_SEND_DA;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("send DA command error. code 0x%X")%(uint32)(ack)));
      }

      uint32 echo;
      write32(loader->at_address);
      read32(&echo);

      write32(loader->length);
      read32(&echo);

      write32(loader->sig_length);
      read32(&echo);

      uint16 status;
      read16(&status);

      uint16 checksum = 0;

      uint32 adjust_length = loader->length;
      //send data

      for(uint32 cnt=0; cnt<(loader->length+1)/2; ++cnt)
      {
         uint16 byte16 = 0;
         if(cnt == (loader->length)/2)//not even length, the last byte. mind big endien.
         {
            int8 tail_byte[2];
            *(uint16*)tail_byte = 0;
            tail_byte[0] = *((uint8*)((loader->data)+sizeof(uint16)*cnt));
            byte16 = *(uint16*)tail_byte;
         }
         else
         {
            byte16 = *((uint16*)(loader->data)+cnt);
         }
         checksum ^= byte16;
      }

      write_buffer((uint8*)loader->data, loader->length);

      uint16 checksum_ret;
      read16(&checksum_ret);
      if(checksum_ret != checksum)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            (boost::format("DA data checksum mismatched. 0x%X != 0x%X")%(uint32)(checksum)%(uint32)(checksum_ret)).str()));
      }

      read16(&status);
      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("send DA data error. code 0x%X")%(uint32)(status)));
      }

   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_SEND_DA_FAIL;
   }

   return STATUS_OK;
}

status_t boot_rom::jump_to_loader(uint32 at_address)
{
   CALL_LOGD;
   return jump_to(at_address);
}

status_t boot_rom::send_data_to(section_block_t* section)
{
   CALL_LOGD;
    //reuse send da.
   return send_loader(section);
}

status_t boot_rom::jump_to(uint32 at_address)
{
   CALL_LOGD;
   LOGI << "boot rom jump to." ;
   try
   {
      //use this command to send data.
      uint8 cmd = BROM_CMD_JUMP_DA;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("send Jump DA command error. code 0x%X")%(uint32)(ack)));
      }

      uint32 echo;
      write32(at_address);
      read32(&echo);
      if(echo != at_address)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("Jump DA error: start address 0x%X maybe invalid. code 0x%X")%(uint32)(at_address)%(uint32)(echo)));
      }

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("Jump DA error. code 0x%X")%(uint32)(status)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_SEND_DA_FAIL;
   }

   return STATUS_OK;
}

status_t boot_rom::loader_send_partition_to(loader_partition_param_t* arg)
{
   CALL_LOGD;
   LOGI << "send partition data to loader. " ;
   try
   {
      uint8 cmd = BLDR_CMD_SEND_PARTITION_DATA;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BLDR_CMD_SEND_PARTITION_DATA error. code 0x%X")%(uint32)(ack)));
      }

      write_buffer((uint8*)arg->part_name, PARTITION_NAME_LEN);

      write32(arg->length);
      read8(&ack);

      if(FAIL(ack))
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BLDR_CMD_SEND_PARTITION_DATA param check error. code 0x%X")%(uint32)(ack)));
      }

      write_buffer(arg->data, arg->length);

      uint32 checksum = checksum_plain(arg->data, arg->length);

      write32(checksum);

      read8(&ack);
      if(FAIL(ack))
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BLDR_CMD_SEND_PARTITION_DATA param check error. code 0x%X")%(uint32)(ack)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }

   return STATUS_OK;
}

status_t boot_rom::loader_jump_to_partition(loader_partition_param_t* arg)
{
   CALL_LOGD;
   LOGI << "send partition data to loader. " ;
   try
   {
      uint8 cmd = BLDR_CMD_JUMP_TO_PARTITION;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BLDR_CMD_JUMP_TO_PARTITION error. code 0x%X")%(uint32)(ack)));
      }

      write_buffer((uint8*)arg->part_name, PARTITION_NAME_LEN);

      read8(&ack);
      if(FAIL(ack))
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BLDR_CMD_JUMP_TO_PARTITION param check error. code 0x%X")%(uint32)(ack)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }

   return STATUS_OK;
}


status_t boot_rom::device_control(uint32 ctrl_code,
                                  void* inbuffer,
                                  uint32 inbuffer_size,
                                  void* outbuffer,
                                  uint32 outbuffer_size,
                                  uint32* bytes_returned)
{
   CALL_LOGD;
   status_t status = STATUS_OK;
   if(bytes_returned != NULL)
   {
      *bytes_returned = 0;
   }

   if(ctrl_code == DEV_GET_CHIP_ID)
   {
      if(outbuffer_size < sizeof(chip_id_struct))
      {
         status = STATUS_BUFFER_OVERFLOW;
      }
      else
      {
         get_chip_id((chip_id_struct*)outbuffer);
         if(bytes_returned != NULL)
         {
            *bytes_returned = sizeof(chip_id_struct);
         }
         status = STATUS_OK;
      }
   }
   else if(ctrl_code == DEV_GET_BOOT_AGENT)
   {
      if(outbuffer == NULL || outbuffer_size != sizeof(uint32))
      {
         status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         *(uint32*)outbuffer = BOOT_AGENT_BOOT_ROM;
         uint8 preloader_ver;
         status = get_preloader_version(&preloader_ver);
         if(SUCCESSED(status))
         {
            *(uint32*)outbuffer = BOOT_AGENT_LOADER;
         }
         status = STATUS_OK;
      }
   }
   else if(ctrl_code == DEV_LOADER_SUPPORT_EMPTY_BOOT)
   {
      if(outbuffer == NULL || outbuffer_size != sizeof(uint32))
      {
         status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         *(uint32*)outbuffer = (uint32)TRUE;
         uint8 preloader_ver;
         status = get_preloader_version(&preloader_ver);
         if(SUCCESSED(status))
         {
            if(preloader_ver < 2)//version 1 not support xflash(empty boot)
            {
                *(uint32*)outbuffer = (uint32)FALSE;
            }
         }
         status = STATUS_OK;
      }
   }
   else if(ctrl_code == DEV_BROM_SEND_DATA_TO)
   {
      if(inbuffer == NULL)
      {
         status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         struct section_block_t* sec = (struct section_block_t*)inbuffer;
         status = send_data_to(sec);
      }
   }
   else if(ctrl_code == DEV_BROM_JUMP_TO)
   {
      if(inbuffer == NULL)
      {
         status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         uint32 at_address = *(uint32*)inbuffer;
         status = jump_to(at_address);
      }
   }
   else if(ctrl_code == DEV_LOADER_JUMP_PARTITION)
   {
      if(inbuffer == NULL)
      {
         status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         loader_partition_param_t arg = {0};
         strncpy(arg.part_name, (char8*)inbuffer, inbuffer_size);

         status = loader_jump_to_partition(&arg);
      }
   }
   else
   {
      status = STATUS_NOT_IMPLEMENTED;
   }

   return status;
}

status_t boot_rom::get_preloader_version(uint8* version)
{
   CALL_LOGD;
   try
   {
      BOOST_ASSERT(version != 0);
      uint8 cmd = BLDR_CMD_GET_BLDR_VER;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      //boot rom do not support this, so return value same as cmd
      if(cmd == ack)
      {
         LOGI<< "brom connection. not preloader.";
         return STATUS_BROM_CMD_FAIL;
      }

      *version = ack;
      LOGI<< (boost::format("preloader version: 0x%x")%(uint32)ack).str();

   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }

   return STATUS_OK;

}

status_t boot_rom::get_security_config(uint32* flag)
{
   CALL_LOGD;
   try
   {
      BOOST_ASSERT(flag != 0);
      uint8 cmd = BROM_CMD_GET_TARGET_CONFIG;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_CMD_GET_TARGET_CONFIG error. code 0x%X")%(uint32)(ack)));
      }

      uint32 cfg = 0;
      read32(&cfg);

      *flag = cfg;

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_CMD_GET_TARGET_CONFIG error. code 0x%X")%(uint32)(status)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }

   return STATUS_OK;
}
status_t boot_rom::send_cert_file(uint8* data, uint32 length)
{
   CALL_LOGD;
   try
   {
      uint8 cmd = BROM_SCMD_SEND_CERT;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_CERT error. code 0x%X")%(uint32)(ack)));
      }

      write32(length);
      uint32 echo = 0;
      read32(&echo);
      if(echo != length)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_CERT error. code 0x%X")%(uint32)(ack)));
      }

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_CERT error. code 0x%X")%(uint32)(status)));
      }

      write_buffer(data, length);

      uint16 chk_sum;
      //not use this.
      read16(&chk_sum);


      read16(&status);
      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_CERT error. code 0x%X")%(uint32)(status)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }
   return STATUS_OK;
}

status_t boot_rom::send_auth_file(uint8* data, uint32 length)
{
   CALL_LOGD;
   try
   {
      uint8 cmd = BROM_SCMD_SEND_AUTH;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_AUTH error. code 0x%X")%(uint32)(ack)));
      }

      write32(length);
      uint32 echo = 0;
      read32(&echo);
      if(echo != length)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_AUTH error. code 0x%X")%(uint32)(ack)));
      }

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_AUTH error. code 0x%X")%(uint32)(status)));
      }

      write_buffer(data, length);

      uint16 chk_sum;
      //not use this.
      read16(&chk_sum);


      read16(&status);
      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SEND_AUTH error. code 0x%X")%(uint32)(status)));
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }
   return STATUS_OK;
}

status_t boot_rom::get_get_me_id(uint8** data)
{
   CALL_LOGD;
   #define ME_ID_BUFFER_LEN 16
   static uint8 me_id[ME_ID_BUFFER_LEN];
   memset(me_id, 0, ME_ID_BUFFER_LEN);
   try
   {
      if(data == 0)
      {
         return STATUS_INVALID_PARAMETER;
      }

      uint8 cmd = BROM_SCMD_GET_ME_ID;
      uint8 ack = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_GET_ME_ID error. code 0x%X")%(uint32)(ack)));
      }

      uint32 data_len = 0;
      read32(&data_len);

      BOOST_ASSERT(data_len == ME_ID_BUFFER_LEN);
      read_buffer(me_id, data_len);

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_GET_ME_ID error. code 0x%X")%(uint32)(status)));
      }

      *data = (uint8*)me_id;
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }
   return STATUS_OK;
}

status_t boot_rom::qualify_host()
{
   CALL_LOGD;
   try
   {
      uint8 cmd = BROM_SCMD_SLA;
      uint8 ack = 0;
  	  unsigned char	uc_tmp;
	  unsigned int i = 0;
      write8(cmd);
      read8(&ack);
      if(cmd != ack)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SLA error. code 0x%X")%(uint32)(ack)));
      }

      uint16 status;
      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SLA error. code 0x%X")%(uint32)(status)));
      }

      uint32 length;
      read32(&length);

      //boost::shared_ptr<uint8[]> buffer = boost::make_shared<uint8[]>(length);
      boost::shared_ptr<zbuffer> buffer = boost::make_shared<zbuffer>(length);
      uint8* data = buffer->get();

      read_buffer(data, length);

	  /* convert from word array to byte array */
	  for (i = 0; i < length; i += 2) {
		  uc_tmp = data[i];
		  data[i] = data[i+1];
		  data[i+1] = uc_tmp;
	  }

      uint8* data_out = 0;
      uint32 data_out_len = 0;

      if(0 != sla_callbacks::sla_start(data, length, &data_out, &data_out_len))
      {
         data_out = (uint8*)"SLA";//"sla failed, let protocol continue. running dead.";
         data_out_len = 4;
      }

      write32(data_out_len);

      uint32 echo = 0;
      read32(&echo);
      if(echo != data_out_len)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SLA error. code 0x%X")%(uint32)(ack)));
      }

      read16(&status);

      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SLA error. code 0x%X")%(uint32)(status)));
      }

	  /* convert from word array to byte array */
	  for (i = 0; i < data_out_len; i += 2) {
		  uc_tmp = data_out[i];
		  data_out[i] = data_out[i+1];
		  data_out[i+1] = uc_tmp;
	  }

      write_buffer(data_out, data_out_len);

      read16(&status);
      if(status >= E_ERROR)
      {
         BOOST_THROW_EXCEPTION(runtime_exception(
            boost::format("BROM_SCMD_SLA error. code 0x%X")%(uint32)(status)));
      }

      sla_callbacks::sla_end(0);
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_BROM_CMD_FAIL;
   }
   return STATUS_OK;
}

///********************************
void boot_rom::read8(uint8* data)
{
   com->receive(data, 1, timeout);
}
void boot_rom::read16(uint16* data)
{
   com->receive((uint8*)data, sizeof(uint16), timeout);
   *data = ntohs(*data);
}
void boot_rom::read32(uint32* data)
{
   com->receive((uint8*)data, sizeof(uint32), timeout);
   *data = ntohl(*data);
}
void boot_rom::read_buffer(uint8* data, uint32 length)
{
   com->receive(data, length, timeout);
}

void boot_rom::write8(uint8 data)
{
   com->send(&data, 1, timeout);
}
void boot_rom::write16(uint16 data)
{
   data = htons(data);
   com->send((uint8*)&data, sizeof(uint16), timeout);
}
void boot_rom::write32(uint32 data)
{
   data = htonl(data);
   com->send((uint8*)&data, sizeof(uint32), timeout);
}
void boot_rom::write_buffer(uint8* data, uint32 length)
{
   com->send(data, length, timeout);
}
