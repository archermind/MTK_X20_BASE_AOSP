#include <boost/container/map.hpp>
#include "functions/scatter/scatter_transfer.h"
#include "xflash_struct.h"

uint32 scatter_transfer::transfer_string(std::string key)
{
   boost::container::map<std::string, uint32> m;
   //storage.
   m["HW_STORAGE_EMMC"] = (uint32)STORAGE_EMMC;
   m["HW_STORAGE_NAND"] = (uint32)STORAGE_NAND;
   m["HW_STORAGE_NOR"] = (uint32)STORAGE_NOR;
   m["HW_STORAGE_UFS"] = (uint32)STORAGE_UFS;

   //section
   m["EMMC_BOOT_1"] = (uint32)EMMC_BOOT1;
   m["EMMC_BOOT_2"] = (uint32)EMMC_BOOT2;
   m["EMMC_BOOT1_BOOT2"] = (uint32)EMMC_BOOT1_BOOT2;
   m["EMMC_USER"] = (uint32)EMMC_USER;
   m["EMMC_RPMP"] = (uint32)EMMC_RPMB;
   m["EMMC_GP_1"] = (uint32)EMMC_GP1;
   m["EMMC_GP_2"] = (uint32)EMMC_GP2;
   m["EMMC_GP_3"] = (uint32)EMMC_GP3;
   m["EMMC_GP_4"] = (uint32)EMMC_GP4;

   m["UFS_LUA1"] = (uint32)UFS_SECTION_LUA1;
   m["UFS_LUA2"] = (uint32)UFS_SECTION_LUA2;
   m["UFS_LUA3"] = (uint32)UFS_SECTION_LUA3;
   m["UFS_LUA4"] = (uint32)UFS_SECTION_LUA4;
   m["UFS_LUA5"] = (uint32)UFS_SECTION_LUA5;
   m["UFS_LUA6"] = (uint32)UFS_SECTION_LUA6;
   m["UFS_LUA7"] = (uint32)UFS_SECTION_LUA7;
   m["UFS_LUA8"] = (uint32)UFS_SECTION_LUA8;

   //nand section used for MLC
   m["d_type1"] = (uint32)CELL_UNI;
   m["d_type2"] = (uint32)CELL_BINARY;
   m["d_type3"] = (uint32)CELL_TRI;
   m["d_type4"] = (uint32)CELL_QUAD;
   m["d_type5"] = (uint32)CELL_PENTA;
   m["d_type6"] = (uint32)CELL_HEX;
   m["d_type7"] = (uint32)CELL_HEPT;
   m["d_type8"] = (uint32)CELL_OCT;

   m["true"] = (uint32)TRUE;
   m["false"] = (uint32)FALSE;

   m["yes"] = (uint32)TRUE;
   m["no"] = (uint32)FALSE;

   if(m.count(key) != 0)
   {
      return m[key];
   }
   else
   {
      return STORAGE_UNKNOW;
   }
}