#include "common/common_include.h"
#include "config/lib_config_parser.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp>
//warning C4345: behavior change: an object of POD type
//constructed with an initializer of the form () will be default-initialized
#pragma warning(disable:4345)

lib_config_parser::lib_config_parser(string f_name)
: file_name(f_name)
{
}

STD_::string lib_config_parser::get_value(string key)
{
   typedef boost::property_tree::ptree xml_tree;
   typedef boost::property_tree::ptree::value_type xnode;
   xml_tree xml_parser;

   STD_::string value;
   try
   {
      boost::filesystem::path lib_cfg = get_config_directory()/file_name;
      if(!boost::filesystem::exists(lib_cfg))
      {
         //LOGI << (boost::format("CFG: File Not Found: %s")%lib_cfg.string()).str();
         return value;
      }

      //LOGI << (boost::format("Read lib config file: ")%lib_cfg.string()).str();
      read_xml(lib_cfg.string(), xml_parser);

      BOOST_AUTO(root, xml_parser.get_child_optional(STD_::string("config.")+key));
      if(root)
      {
         value = xml_parser.get<STD_::string>(STD_::string("config.")+key);
      }
   }
   catch(boost::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
   }

   return value;
}
