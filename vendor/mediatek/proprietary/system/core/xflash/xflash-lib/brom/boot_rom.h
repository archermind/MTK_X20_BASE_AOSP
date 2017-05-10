#ifndef	_BOOT_ROM_H_
#define	_BOOT_ROM_H_

#include "common/types.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include "xflash_struct.h"

using namespace boost;

class ITransmission;
struct chip_id_struct;

typedef struct loader_partition_param_t
{
	char8 part_name[PARTITION_NAME_LEN];
   uint8* data;
   uint32 length;
} loader_partition_param_t;

struct chip_id_struct
{
   uint16 hw_code;
   uint16 hw_sub_code;
   uint16 hw_version;
   uint16 sw_version;
};

class boot_rom
{
public:
   boot_rom(void);
   ~boot_rom(void);
public:
   void set_transfer_channel(boost::shared_ptr<ITransmission> channel);
   status_t connect();
   status_t get_chip_id(/*IN OUT*/chip_id_struct* id);
   status_t send_loader(section_block_t* loader);
   status_t jump_to_loader(uint32 at_address);
   status_t device_control(uint32 ctrl_code,
                        void* inbuffer,
                        uint32 inbuffer_size,
                        void* outbuffer,
                        uint32 outbuffer_size,
                        uint32* bytes_returned);
   status_t loader_send_partition_to(loader_partition_param_t* arg);
   status_t loader_jump_to_partition(loader_partition_param_t* arg);
public:
   status_t get_preloader_version(uint8* version);
   status_t get_security_config(uint32* flag);
   status_t send_cert_file(uint8* data, uint32 length);
   status_t send_auth_file(uint8* data, uint32 length);
   status_t get_get_me_id(uint8** data);
   status_t qualify_host();
private:
   status_t send_data_to(section_block_t* section);
   status_t jump_to(uint32 at_address);
private:
   boost::shared_ptr<ITransmission> com;
   uint32 timeout;
   BOOL is_bootloader;
private:
   void read8(uint8* data);
   void read16(uint16* data);
   void read32(uint32* data);
   void read_buffer(uint8* data, uint32 length);
   void write8(uint8 data);
   void write16(uint16 data);
   void write32(uint32 data);
   void write_buffer(uint8* data, uint32 length);
};

#endif
