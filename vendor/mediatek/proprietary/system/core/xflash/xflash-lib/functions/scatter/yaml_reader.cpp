#include "common/common_include.h"
#include "functions/scatter/yaml_reader.h"

#include "yaml-cpp/yaml.h"

#define FILE_V111 "V1.1.1"
#define FILE_V112 "V1.1.2"

void operator >> (const YAML::Node& node, std::string& s)
{
   s = node.as<std::string>();
}
void operator >> (const YAML::Node& node, image_info_t& image_info)
{
   node["partition_index"] >> image_info.part_idx;     //Not used
   node["partition_name"] >> image_info.part_name;
   node["file_name"] >> image_info.file_name;
   node["is_download"] >> image_info.is_download;
   node["type"] >> image_info.type;                     //Not used
   node["linear_start_addr"] >> image_info.linear_start_address;    //Need revise
   node["physical_start_addr"] >> image_info.physical_start_address; //Need revise
   node["partition_size"] >> image_info.part_size;         //Need revise
   node["region"] >> image_info.region;
   node["storage"] >> image_info.storage;
   node["boundary_check"] >> image_info.boundary_check;  //Not used
   node["is_reserved"] >> image_info.is_reserved;         //Not used
   node["operation_type"] >> image_info.operation_type;   //

   try
   {
      node["empty_boot_needed"] >> image_info.empty_boot_needed;
   }
   catch(const YAML::Exception&)
   {
      //LOGV << "scatter file has no 'empty_boot_needed'.";
   }

   try
   {
      node["d_type"] >> image_info.d_type;
   }
   catch(const YAML::Exception&)
   {
      //LOGV << "scatter file has no 'd_type'.";
   }

   node["reserve"] >> image_info.reserve;                 //Not used
}

void operator >> (const YAML::Node& node, project_info_t& proj)
{
   node["config_version"] >> proj.config_version;
   node["platform"] >> proj.platform;
   node["project"] >> proj.project;              //Not used
   node["storage"] >> proj.storage;             //Not used
   node["boot_channel"] >> proj.bootChannel;  //Not used
   node["block_size"] >> proj.block_size;
}

void operator >> (const YAML::Node& node, image_hdr_info_t& hdr)
{
   node["general"] >> hdr.general;
   node["info"][0] >> hdr.project;
}



void operator >> (YAML::Node& doc, scatter_file_struct& scatter_file)
{
   doc[0] >> scatter_file.hdr;

   if(scatter_file.hdr.project.config_version == FILE_V111)
   {
      scatter_file.is_linear_address_valid = TRUE;
   }
   else
   {
      scatter_file.is_linear_address_valid = FALSE;
   }

   for(unsigned i=1;i<doc.size();i++)
   {
      image_info_t info;
      doc[i] >> info;
      scatter_file.image_infos.push_back(info);
   }
}

BOOL yaml_reader::verify(string scatter_file_name)
{
   fstream input(scatter_file_name.c_str(), ios::in);
   YAML::Node doc;
   try
   {
      doc = YAML::Load(input);
   }
   catch(YAML::Exception& e)
   {
      e = e;
      return FALSE;
   }
   return TRUE;
}

BOOL yaml_reader::get_scatter_file(string scatter_file_name, /*IN OUT*/scatter_file_struct* scatter_file)
{
   CALL_LOGD;
   fstream input(scatter_file_name.c_str(), ios::in);
   YAML::Node doc;
   try
   {
      doc = YAML::Load(input);
   }
   catch(const YAML::Exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return FALSE;
   }

   try
   {
      scatter_file->image_infos.clear();
      doc >> (*scatter_file);
   }
   catch(const YAML::Exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return FALSE;
   }
   return TRUE;
}
