#include "common/common_include.h"
#include "common/zbuffer.h"
#include "logic/logic.h"
#include "logic/kernel.h"
#include "brom/boot_rom.h"
#include "functions/gui_callbacks.h"
#include "functions/scatter/yaml_reader.h"
#include "functions/scatter/scatter_transfer.h"
#include "xflash_struct.h"

#include "logic/connection.h"
#include "code_translate.h"

logic::logic(void)
{
}

logic::~logic(void)
{
}

status_t logic::load_scatter_file(string file_name, scatter_file_struct* scatter_file)
{
   CALL_LOGD;
   boost::filesystem::path image_path(file_name, (void*)boost::filesystem::native);
   if(!boost::filesystem::exists(image_path))
   {
      LOGE << (boost::format("scatter file Not Found: %s")%file_name).str();
      return STATUS_FILE_NOT_FOUND;
   }

   LOGI << (boost::format("load scatter file: %s")%file_name).str();
   boost::shared_ptr<IScatterFileReader> scatter_reader;
   if(yaml_reader::verify(file_name))
   {
      scatter_reader = boost::make_shared<yaml_reader>();
      if(!scatter_reader->get_scatter_file(file_name, scatter_file))
      {
         return STATUS_SCATTER_FILE_INVALID;
      }
   }
   else
   {
      return STATUS_SCATTER_FILE_INVALID;
   }

   return STATUS_OK;
}

status_t logic::get_scatter_info(string file_name, scatter_file_info_t* info)
{
   CALL_LOGD;
   if(info == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }
   scatter_file_struct scatter_file;
   status_t status = load_scatter_file(file_name, &scatter_file);
   if(FAIL(status))
   {
      return status;
   }

   LOGI << "get scatter file info.";
   memset(info, 0, sizeof(scatter_file_info_t));
   strncpy(info->hdr.platform, scatter_file.hdr.project.platform.c_str(), PARAM_BUF_SIZE-1);
   strncpy(info->hdr.version, scatter_file.hdr.project.config_version.c_str(), PARAM_BUF_SIZE-1);

   //Get scatter file path to rectify partition file name;
   boost::filesystem::path scatter_path(file_name, (void*)boost::filesystem::native);
   scatter_path.remove_leaf();
   boost::filesystem::path image_path;

   uint32 idx = 0;
   BOOST_FOREACH(image_info_t& inf, scatter_file.image_infos)
   {
      if(inf.is_download == "true")
      {
         strncpy(info->part[idx].part_name, inf.part_name.c_str(), PARTITION_NAME_LEN-1);
         //NEED rectify the partition file name here.
         image_path = scatter_path/inf.file_name;

         strncpy(info->part[idx].file_name, image_path.string().c_str(), MAX_FILE_NAME_LEN-1);
         info->part[idx].start_address = to_hex(inf.physical_start_address);
         info->part[idx].size = to_hex(inf.part_size);
         info->part[idx].storage = scatter_transfer::transfer_string(inf.storage);
         info->part[idx].region = scatter_transfer::transfer_string(inf.region);
         info->part[idx].empty_boot_needed = scatter_transfer::transfer_string(inf.empty_boot_needed);
         ++idx;
      }
   }

   info->hdr.partition_count = idx;

   return STATUS_OK;
}

status_t logic::download_partition(HSESSION hs, op_part_list_t* flist, uint32 count
                                   , struct callbacks_struct_t* callbacks)
{
   CALL_LOGI;
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
   if(!session)
   {
      LOGE<<"invalid session.";
      return STATUS_INVALID_SESSION;
   }
   boost::recursive_mutex::scoped_lock lock(session->mtx);
   if(session->kstage != KSTAGE_LOADER)
   {
      LOGE<<"loader is not start. connect loader first.";
      return STATUS_INVALID_STAGE;
   }

   boost::shared_ptr<gui_callbacks> gui_cb = boost::make_shared<gui_callbacks>(callbacks);
   status_t status = STATUS_OK;

   LOGI<<"DOWNLOAD START.";

   for(uint32 idx=0; idx<count; ++idx)
   {
      LOGI <<(boost::format("download [%s]")%flist[idx].file_path).str();
      boost::filesystem::path file_path(flist[idx].file_path, (void*)boost::filesystem::native);
      if(!boost::filesystem::exists(file_path))
      {
         LOGE << (boost::format("File Not Found. %s")%file_path.string()).str();
         return STATUS_FILE_NOT_FOUND;
      }

      std::ifstream ifile(flist[idx].file_path, std::ios::in|std::ios::binary);
      if(!ifile)
      {
         LOGE<<(boost::format("open file failed. %s")%flist[idx].file_path).str();
         return STATUS_FILE_NOT_FOUND;
      }

      ifile.seekg (0, std::ios::end);
      uint32 file_length = ifile.tellg();
      ifile.seekg (0, std::ios::beg);

      boost::shared_ptr<zbuffer> image = boost::make_shared<zbuffer>(file_length);
      ifile.read((char8*)image->get(), file_length);

      ifile.close();

      string msg = (boost::format("#%d write to %s")%(idx+1)%flist[idx].part_name).str();
      gui_cb->stage_message((char8*)msg.c_str());
      LOGI << msg;

      loader_partition_param_t arg;
      memcpy(arg.part_name, flist[idx].part_name, PARTITION_NAME_LEN);
      arg.data = image->get();
      arg.length = image->size();

      status = session->brom->loader_send_partition_to(&arg);

      if(FAIL(status))
      {
         LOGE<<(boost::format("download failed[index:%d, name: %s]: %s")%idx%flist[idx].part_name%code_translate::err_to_string(status)).str();
         return status;
      }
   }

   LOGI<<"DOWNLOAD END.";
   return status;
}

status_t logic::jump_to_partition(HSESSION hs, string part_name)
{
   CALL_LOGI;
   boost::shared_ptr<session_t> session = kernel::instance()->get_session(hs);
   if(!session)
   {
      LOGE<<"invalid session.";
      return STATUS_INVALID_SESSION;
   }
   boost::recursive_mutex::scoped_lock lock(session->mtx);
   if(session->kstage != KSTAGE_LOADER)
   {
      LOGE<<"loader is not start. connect loader first.";
      return STATUS_INVALID_STAGE;
   }

   loader_partition_param_t arg = {0};
   strncpy(arg.part_name, part_name.c_str(), part_name.length());

   LOGI <<(boost::format("Jump to partition [%s]")%part_name).str();
   status_t status = session->brom->loader_jump_to_partition(&arg);

   if(FAIL(status))
   {
      LOGE<<(boost::format("Jump to fail: %s [%s]")%part_name%code_translate::err_to_string(status)).str();
   }
   return status;
}