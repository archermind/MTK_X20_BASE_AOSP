#include "common/common_include.h"
#include "loader/file/loader_image.h"
#include "loader/file/loader_struct.h"
#include "xflash_struct.h"

loader_image::loader_image()
: sig_length(0)
, loader_length(0)
{
}

status_t loader_image::load(std::string file_name)
{
   CALL_LOGD;
   try
   {
      std::ifstream loader_file(file_name.c_str(), std::ios::in|std::ios::binary);
      if(!loader_file)
      {
         LOGE<<(boost::format("open loader file failed. %s")%file_name.c_str()).str();
         return STATUS_PRELOADER_INVALID;
      }
      loader_file.seekg (0, std::ios::end);
      loader_length = loader_file.tellg();
      loader_file.seekg (0, std::ios::beg);

      //process sig file.
      sig_length = 0;
      string sig_file_name = file_name;
      sig_file_name.erase(sig_file_name.rfind(".bin"));
      sig_file_name += ".sig";
      boost::filesystem::path loader_sig_path(sig_file_name, (void*)boost::filesystem::native);
      if(boost::filesystem::exists(loader_sig_path))
      {
         std::ifstream sig_file(sig_file_name.c_str(), std::ios::in|std::ios::binary);
         if(!sig_file)
         {
            LOGE<<(boost::format("open loader SIG file failed. %s")%sig_file_name.c_str()).str();
            return STATUS_PRELOADER_INVALID;
         }
         sig_file.seekg (0, std::ios::end);
         sig_length = sig_file.tellg();
         sig_file.seekg (0, std::ios::beg);
      }
      else
      {
         LOGI << "loader has no sig file.";
      }

      image = boost::make_shared<zbuffer>(loader_length + sig_length);
      loader_file.read((char8*)image->get(), loader_length);
      if(sig_length != 0)
      {
         loader_file.read((char8*)image->get()+loader_length, sig_length);
      }
   }
   catch (std::exception& e)
   {
      LOGE << boost::diagnostic_information(e);
      return STATUS_PRELOADER_INVALID;
   }

   return STATUS_OK;
}


status_t loader_image::get_section_data(/*IN OUT*/ section_block_t* section)
{
   CALL_LOGD;
   if(!image)
   {
      return STATUS_PRELOADER_INVALID;
   }

   uint32 load_addr = 0;
   uint32 jump_addr = 0;

   PGFH_Header gfh_hdr = (PGFH_Header)(image->get());
   PGFH_FILE_INFO_V1 gfh_file_info_v1;

   BOOL info_found = FALSE;
   while (MAGIC_VER == gfh_hdr->magic)
   {
      switch (gfh_hdr->type)
      {
      case GFH_FILE_INFO:
         gfh_file_info_v1 = (PGFH_FILE_INFO_V1)gfh_hdr;
         load_addr = gfh_file_info_v1->load_addr;
         jump_addr = gfh_file_info_v1->load_addr + gfh_file_info_v1->jump_offset;

         //move next Header.
         gfh_hdr = (PGFH_Header)((int8*)gfh_hdr + sizeof(GFH_FILE_INFO_V1));
         info_found = TRUE;
         break;

      case GFH_BL_INFO:
         //move next Header.
         gfh_hdr = (PGFH_Header)((int8*)gfh_hdr + sizeof(GFH_BL_INFO_V1));
         break;

      default:
         gfh_hdr = (PGFH_Header)((int8*)gfh_hdr + sizeof(GFH_Header));
         break;
      }

      if(info_found)
      {
         break;
      }
   }

   if(info_found)
   {
      section->data = image->get();
      section->length = image->size();
      section->at_address = load_addr;
      section->jmp_address = jump_addr;
      section->sig_offset = loader_length;
      section->sig_length = sig_length;
      return STATUS_OK;
   }
   else
   {
      return STATUS_PRELOADER_INVALID;
   }
}

void loader_image::test()
{

}

