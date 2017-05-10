#pragma once

#include "xflash_struct.h"

class gui_callbacks
{
public:
   gui_callbacks(callbacks_struct_t* cb);
   BOOL is_notify_stopped();
   void operation_progress(unsigned int progress);
   void stage_message(char* message);
public:
   //use this to store tphase
   void set_transfer_phase(enum transfer_phase phase);
private:
   gui_callbacks();
   callbacks_struct_t* cbs;
   enum transfer_phase tphase;
};
