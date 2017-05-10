#pragma once
#include <boost/shared_ptr.hpp>
#include "common/zbuffer.h"

#pragma warning (disable:4290)

struct section_block_t;

class loader_image
{
public:
   loader_image();
   status_t load(std::string file_name);
   status_t get_section_data(/*IN OUT*/ section_block_t* section);
public:
   static void test();

private:
   boost::shared_ptr<zbuffer> image;
   uint32 loader_length;
   uint32 sig_length;
};
