#ifndef	_DA_INFO_STRUCT_H_
#define	_DA_INFO_STRUCT_H_

#include "common/types.h"

#define ID_NUM1      (12)
#define ID_NUM2      (8)
#define MAGIC_VER    (0x014D4D4D)

typedef enum
{
   GFH_FILE_INFO = 0
   ,GFH_BL_INFO
} GFH_TYPE;


typedef struct
{
   uint32 magic;
   uint16 size;
   uint16 type;
} GFH_Header, *PGFH_Header;

typedef struct
{
   GFH_Header m_gfh_hdr;
   int8 id[ID_NUM1];
   uint32 file_version;
   uint16 file_type;
   int8 flash_dev;
   int8 sig_type;
   uint32 load_addr;
   uint32 length;
   uint32 max_size;
   uint32 content_offset;
   uint32 sig_length;
   uint32 jump_offset;
   uint32 addr;
} GFH_FILE_INFO_V1, *PGFH_FILE_INFO_V1;

typedef struct
{
   GFH_Header m_gfh_hdr;
   uint32 m_bl_attr;
} GFH_BL_INFO_V1, *PGFH_BL_INFO_V1;

#endif
