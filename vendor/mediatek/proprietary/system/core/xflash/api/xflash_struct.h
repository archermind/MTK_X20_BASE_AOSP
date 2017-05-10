#ifndef __XFLASH_LIB_STRUCT__
#define __XFLASH_LIB_STRUCT__

#include "type_define.h"

#if defined(_LINUX)
   #define __stdcall
#endif

typedef enum logging_level
{
    ktrace = 0,
    kdebug,
    kinfo,
    kwarning,
    kerror,
    kfatal
}logging_level_e;

#define PARTITION_NAME_LEN 64
#define PARAM_BUF_SIZE 64
#define MAX_FILE_NAME_LEN 512
#define MAX_PARTITION_COUNT		128

typedef struct partition_info_t
{
   char8 part_name[PARTITION_NAME_LEN];
   char8 file_name[MAX_FILE_NAME_LEN];
   uint64 start_address;
   uint64 size;
   uint32 storage;
   uint32 region;
   uint32 operation_type;
   BOOL empty_boot_needed;
   BOOL enable;
}partition_info_t;

typedef struct partition_info_list_t
{
   uint32 count;
   partition_info_t part[MAX_PARTITION_COUNT];
}partition_info_list_t;

typedef struct scatter_file_info_t
{
   struct header_t
   {
      char8 version[PARAM_BUF_SIZE];
      char8 platform[PARAM_BUF_SIZE];
      uint32 partition_count;
   }hdr;
   partition_info_t part[MAX_PARTITION_COUNT]; //only visable partitions
}scatter_file_info_t;

typedef struct op_part_list_t
{
   char8 part_name[PARTITION_NAME_LEN];
   char8 file_path[MAX_FILE_NAME_LEN];
}op_part_list_t;


typedef enum transfer_phase
{
    TPHASE_INIT = 0
   ,TPHASE_DA
   ,TPHASE_LOADER
   ,TPHASE_GENERIC
}transfer_stage_e;

typedef void (__stdcall *CB_OPERATION_PROGRESS)(pvoid _this, enum transfer_phase phase,
                                                uint32 progress);
typedef void (__stdcall *CB_STAGE_MESSAGE)(pvoid _this, char8* message);
typedef BOOL (__stdcall *CB_NOTIFY_STOP)(pvoid _this);

typedef struct callbacks_struct_t
{
   pvoid _this;
   CB_OPERATION_PROGRESS cb_op_progress;
   CB_STAGE_MESSAGE cb_stage_message;
   CB_NOTIFY_STOP cb_notify_stop;
}callbacks_struct_t;


typedef struct dll_info_t
{
   char8* version;
   char8* release_type;
   char8* build_data;
   char8* change_list;
   char8* builder;
}dll_info_t;


typedef enum boot_agent
{
    BOOT_AGENT_UNKNOWN = 0,
    BOOT_AGENT_BOOT_ROM = 1,
    BOOT_AGENT_LOADER = 2,
}boot_agent_e;

//brom device control
typedef enum device_control_code
{
    DEV_GET_CHIP_ID = 1,
    DEV_GET_BOOT_AGENT,  //BOOT_AGENT_BOOT_ROM or BOOT_AGENT_LOADER
    DEV_BROM_SEND_DATA_TO,
    DEV_BROM_JUMP_TO,
    DEV_LOADER_JUMP_PARTITION,
    DEV_LOADER_SUPPORT_EMPTY_BOOT,  //TRUE or FALSE
}device_control_code_e;

typedef struct section_block_t
{
	uint8*	data;
   uint32	length;
   uint32	at_address;
   uint32	jmp_address;
   uint32   sig_offset;
   uint32   sig_length;
} section_block_t;

typedef enum storage_type
{
   STORAGE_UNKNOW = 0x0,
   STORAGE_NONE = STORAGE_UNKNOW,
   STORAGE_EMMC = 0x1,
   STORAGE_SDMMC,
   STORAGE_UFS,
   STORAGE_NAND = 0x10,
   STORAGE_NAND_SLC,
   STORAGE_NAND_MLC,
   STORAGE_NAND_TLC,
   STORAGE_NAND_SPI,
   STORAGE_NOR = 0x20,
   STORAGE_NOR_SERIAL,
   STORAGE_NOR_PARALLEL,
}storage_type_e;

typedef enum emmc_section{
   EMMC_UNKNOWN = 0
   ,EMMC_BOOT1
   ,EMMC_BOOT2
   ,EMMC_RPMB
   ,EMMC_GP1
   ,EMMC_GP2
   ,EMMC_GP3
   ,EMMC_GP4
   ,EMMC_USER
   ,EMMC_END
   ,EMMC_BOOT1_BOOT2
} emmc_section_e;

typedef enum ufs_section{
   UFS_SECTION_UNKONWN = 0
   ,UFS_SECTION_LUA1
   ,UFS_SECTION_LUA2
   ,UFS_SECTION_LUA3
   ,UFS_SECTION_LUA4
   ,UFS_SECTION_LUA5
   ,UFS_SECTION_LUA6
   ,UFS_SECTION_LUA7
   ,UFS_SECTION_LUA8
} ufs_section_e;

typedef enum nand_cell_usage
{
   CELL_UNI = 0,
   CELL_BINARY,
   CELL_TRI,
   CELL_QUAD,
   CELL_PENTA,
   CELL_HEX,
   CELL_HEPT,
   CELL_OCT,
}nand_cell_usage_e;

#endif
