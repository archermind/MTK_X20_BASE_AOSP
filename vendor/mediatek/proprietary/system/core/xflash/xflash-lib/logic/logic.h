#pragma once

#include "xflash_struct.h"

struct scatter_file_struct;
struct scatter_file_info_t;

class logic
{
public:
   logic(void);
   ~logic(void);
public:
   static status_t get_scatter_info(string file_name, scatter_file_info_t* info);

   static status_t download_partition(HSESSION hs, op_part_list_t* flist,
                       uint32 count, struct callbacks_struct_t* callbacks);
   static status_t jump_to_partition(HSESSION hs, string part_name);

private:
   static status_t load_scatter_file(string file_name, scatter_file_struct* scatter_file);
 };
