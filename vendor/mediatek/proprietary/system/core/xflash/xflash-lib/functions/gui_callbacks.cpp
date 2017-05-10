#include "common/common_include.h"
#include "gui_callbacks.h"

gui_callbacks::gui_callbacks(callbacks_struct_t* cb)
:cbs(cb), tphase(TPHASE_INIT)
{}

BOOL gui_callbacks::is_notify_stopped()
{
   if(cbs != NULL && cbs->cb_notify_stop != NULL)
   {
      return cbs->cb_notify_stop(cbs->_this);
   }
   else
   {
      return FALSE;
   }
}

void gui_callbacks::operation_progress(unsigned int progress)
{
   if(cbs != NULL && cbs->cb_op_progress != NULL)
   {
      cbs->cb_op_progress(cbs->_this, tphase, progress);
   }
}

void gui_callbacks::stage_message(char* message)
{
   if(cbs != NULL && cbs->cb_stage_message != NULL)
   {
      cbs->cb_stage_message(cbs->_this, message);
   }
}

void gui_callbacks::set_transfer_phase(enum transfer_phase phase)
{
   tphase = phase;
}
