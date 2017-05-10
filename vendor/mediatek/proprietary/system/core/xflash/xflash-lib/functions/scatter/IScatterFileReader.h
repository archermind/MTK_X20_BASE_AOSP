#ifndef __ISCATTER_FILE_READER__
#define __ISCATTER_FILE_READER__

#include <string>
#include <vector>

using namespace std;

typedef struct project_info_t
{
	string config_version;
	string platform;
	string project;
	string storage;
	string bootChannel;
	string block_size;
}project_info_t;

typedef struct image_hdr_info_t
{
	string general;
	project_info_t project;

}image_hdr_info_t;

typedef struct image_info_t
{
	string part_idx;
	string part_name;
	string file_name;
	string is_download;
	string type;
	string linear_start_address;
	string physical_start_address;
	string part_size;
	string region;
	string storage;
	string boundary_check;
	string is_reserved;
	string operation_type;
   string d_type;
   string empty_boot_needed;
	string reserve;
}image_info_t;

typedef struct scatter_file_struct
{
	image_hdr_info_t hdr;
   std::vector<image_info_t> image_infos;
   BOOL is_linear_address_valid;
}scatter_file_struct;

DECLEAR_INTERFACE class IScatterFileReader
{
public:
   virtual BOOL get_scatter_file(string scatter_file_name, /*IN OUT*/scatter_file_struct* scatter_file)=0;
   virtual ~IScatterFileReader(){}
};

#endif
