#include "common/common_include.h"
#include "transfer/comm_engine.h"
#include "windows.h"

//this flag is control sync impl and async impl of COM R/W
#define USE_ASYNC_IMPL

static std::string to_string(uint8* data, uint32 length)
{
   string s = (boost::format("0x%08x Hex[")%length).str();
   length = length <= 16 ? length : 16;
   for(uint32 i=0; i<length; ++i)
   {
      s += (boost::format("%02x ")%(uint32)(data[i])).str();
   }
   s += "]";
   return s;
}

class sync_event
{
public:
   //idx 0 for R/W, idx 1 for interrupt thread.
   HANDLE r_evHandle[2];
   HANDLE w_evHandle[2];
};

comm_engine::comm_engine()
: hcom(INVALID_HANDLE_VALUE)
, ext(NULL)
, baud_rate(115200)
{
}

comm_engine::comm_engine(string str_baud_rate)
: hcom(INVALID_HANDLE_VALUE)
, ext(NULL)
,baud_rate(115200)
{
   try
   {
      if(!str_baud_rate.empty())
      {
         baud_rate = boost::lexical_cast<uint32>(str_baud_rate);
      }
   }
   catch(boost::bad_lexical_cast&)
   {
      LOGW << "COM baud rate setting value is invalid. check code.";
   }
}

comm_engine::~comm_engine(void)
{
   close();
}

#if defined USE_ASYNC_IMPL
void comm_engine::open(const string& name)
{
   CALL_LOGI;
   if (hcom != INVALID_HANDLE_VALUE)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Can not open twice.")));
      return;
   }

   LOGI << (boost::format("try to open device: %s baud rate %d")%name%baud_rate).str();
   string port_name = name;
   if(boost::algorithm::all(name, boost::algorithm::is_digit()))
   {
      port_name = string("\\\\.\\COM")+name;
   }
   else if(starts_with(name, "COM"))
   {
      port_name = string("\\\\.\\")+name;
   }

   uint32 retry = 20;
   while(retry > 0)
   {

      hcom = ::CreateFile(port_name.c_str(), GENERIC_READ | GENERIC_WRITE,
         0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

      if (hcom != INVALID_HANDLE_VALUE)
      {
         break;
      }

      LOGI << "retry CreateFile.";
      --retry;
      this_thread::sleep_for(boost::chrono::milliseconds(50));
   }

   if (hcom == INVALID_HANDLE_VALUE)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Create COM File failed.")));
   }

   DWORD com_error = 0;
   ::ClearCommError(hcom, &com_error, NULL);
   ::SetupComm(hcom, 80*1024, 80*1024); //80K cache.
   DCB dcb = {0};

   GetCommState(hcom, &dcb);

   dcb.BaudRate = baud_rate;
   dcb.fAbortOnError = FALSE;
   dcb.fBinary = 1;

   // Set 8/N/1
   dcb.ByteSize = 8;
   dcb.fParity = FALSE;
   dcb.Parity = NOPARITY;
   dcb.StopBits = ONESTOPBIT;

   // Disable H/W flow control
   dcb.fDtrControl = DTR_CONTROL_ENABLE;
   dcb.fRtsControl = RTS_CONTROL_ENABLE;
   dcb.fOutxCtsFlow = FALSE;
   dcb.fOutxDsrFlow = FALSE;
   dcb.fDsrSensitivity = FALSE;

   // Disable S/W flow control
   dcb.fOutX = FALSE;
   dcb.fInX = FALSE;

   if (!::SetCommState(hcom, &dcb))
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("SetCommState failed.")));
      return;
   }

   ::ClearCommBreak(hcom);
   ::EscapeCommFunction(hcom, SETRTS);
   ::EscapeCommFunction(hcom, SETDTR);
   ::PurgeComm(hcom, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);

   COMMTIMEOUTS timeout = {0};

   timeout.ReadIntervalTimeout         = 0;
   //async read must set these zero. or GetOverlappedResult return immediately
   timeout.ReadTotalTimeoutMultiplier  = 0; //1;
   timeout.ReadTotalTimeoutConstant    = 0; //200;
   timeout.WriteTotalTimeoutMultiplier = 0;//1;
   timeout.WriteTotalTimeoutConstant   = 0;//200;

   ::SetCommTimeouts(hcom, &timeout);

   ext = (void*)new sync_event();
   ((sync_event*)ext)->w_evHandle[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
   ((sync_event*)ext)->w_evHandle[1] = ::CreateEvent(NULL, TRUE, FALSE, NULL);

   ((sync_event*)ext)->r_evHandle[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
   ((sync_event*)ext)->r_evHandle[1] = ::CreateEvent(NULL, TRUE, FALSE, NULL);

   LOGI << (boost::format("%s open complete.")%name).str();
   return;
}

//return actual read bytes. timeout throw exception.
size_t comm_engine::receive(uint8* data, size_t length, uint32 timeout)
{
   CALL_LOGV;
   uint32 xferd_read = 0;
   OVERLAPPED r_ov;

   while(xferd_read < length)
   {
      ::ZeroMemory(&r_ov,sizeof(OVERLAPPED));
      r_ov.hEvent = ((sync_event*)ext)->r_evHandle[0];

      if(!::ReadFile(hcom, data+xferd_read, length-xferd_read, NULL, &r_ov))
      {
         if(GetLastError() == ERROR_IO_PENDING)
         {
            DWORD result = ::WaitForMultipleObjects(2, ((sync_event*)ext)->r_evHandle, FALSE, timeout);
            if(result == WAIT_OBJECT_0)
            {
               xferd_read += r_ov.InternalHigh;
            }
            else if(result == WAIT_OBJECT_0 + 1)
            {
               LOGW << "Rx abort.";
               ::CancelIo(hcom);
               throw boost::thread_interrupted();
               break;
            }
            else if(result == WAIT_TIMEOUT)
            {
               ::CancelIo(hcom);
               LOGW << (boost::format("Rx timeout in %dms")%timeout).str();
               break;
            }
            else
            {
               uint32 last_err = GetLastError();
               LOGE << (boost::format("Rx WaitForMultipleObjects error: 0x%x")%last_err).str();
               break;
            }
         }
         else
         {
            uint32 last_err = GetLastError();
            LOGE << (boost::format("Rx ReadFile error. code: 0x%x")%last_err).str();
            break;
         }
      }
      else
      {
         if(r_ov.Internal == ERROR_NO_MORE_ITEMS)
         {
            DWORD com_error = 0;
            ::ClearCommError(hcom, &com_error, NULL);
            LOGW << "Rx Clear COM error" ;
         }
         xferd_read += r_ov.InternalHigh;
      }
   }
   if (xferd_read != length)
   {
      LOGW << (boost::format("Rx xferd length[0x%x] != expected length[0x%x]")%xferd_read%length).str();
      BOOST_THROW_EXCEPTION(runtime_exception(string("Rx data incomplete.")));
   }

   LOGD <<(boost::format("\t\t\t<-Rx: %s")%to_string(data, length)).str();
   return length;
}

//return actual send bytes. timeout throw exception.
size_t comm_engine::send(unsigned char* data, size_t length, unsigned int timeout)
{
   CALL_LOGV;
   uint32 xferd_write = 0;
   OVERLAPPED w_ov;

   while(xferd_write < length)
   {
      ::ZeroMemory(&w_ov,sizeof(OVERLAPPED));
      w_ov.hEvent = ((sync_event*)ext)->w_evHandle[0];

      if (!::WriteFile(hcom,data+xferd_write, length-xferd_write, NULL, &w_ov))
      {
         if (GetLastError() == ERROR_IO_PENDING)
         {
            DWORD result = ::WaitForMultipleObjects(2, ((sync_event*)ext)->w_evHandle, FALSE, timeout);
            if(result == WAIT_OBJECT_0)
            {
               xferd_write += w_ov.InternalHigh;
            }
            else if(result == WAIT_OBJECT_0 + 1)
            {
               LOGW << "Tx abort.";
               ::CancelIo(hcom);
               throw boost::thread_interrupted();
               break;
            }
            else if(result == WAIT_TIMEOUT)
            {
               ::CancelIo(hcom);
               LOGE << (boost::format("Tx timeout in %dms")%timeout).str();
               break;
            }
            else
            {
               uint32 last_err = GetLastError();
               LOGE << (boost::format("Tx WaitForMultipleObjects error: 0x%x")%last_err).str();
               break;
            }
         }
         else
         {
            uint32 last_err = GetLastError();
            LOGE << (boost::format("Tx WriteFile error. code: 0x%x")%last_err).str();
            break;
         }
      }
      else
      {
         xferd_write += w_ov.InternalHigh;
      }
      //::FlushFileBuffers(hcom);

   }
   if (xferd_write != length)
   {
      LOGW << (boost::format("Tx xferd_write length[0x%x] != expected length[0x%x]")%xferd_write%length).str();
      BOOST_THROW_EXCEPTION(runtime_exception(string("Tx data incomplete.")));
   }

   LOGD <<(boost::format("\t\t\tTx->: %s")%to_string(data, length)).str();
   return length;
}

void comm_engine::set_baudrate(uint32 baud)
{
   DCB dcb = {0};

   GetCommState(hcom, &dcb);

   dcb.BaudRate = baud;


   if (!::SetCommState(hcom, &dcb))
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("SetCommState failed.")));
      return;
   }

   ::EscapeCommFunction(hcom, SETRTS);
   ::EscapeCommFunction(hcom, SETDTR);
   ::PurgeComm(hcom, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
}

void comm_engine::cancel()
{
   CALL_LOGD;
   if(ext != NULL)
   {
      ::SetEvent(((sync_event*)ext)->r_evHandle[1]);
      ::SetEvent(((sync_event*)ext)->w_evHandle[1]);
      //::CancelIoEx(hcom, NULL); //Xp only have CancelIo.
   }
   return;
}

void comm_engine::close()
{
   CALL_LOGI;
   cancel();
   if (hcom != INVALID_HANDLE_VALUE)
   {
      DWORD com_error = 0;
      ::ClearCommError(hcom, &com_error, NULL);
      ::CloseHandle(hcom);
      hcom = INVALID_HANDLE_VALUE;
   }
   if(ext != NULL)
   {
      ::CloseHandle(((sync_event*)ext)->r_evHandle[0]);
      ::CloseHandle(((sync_event*)ext)->r_evHandle[1]);
      ::CloseHandle(((sync_event*)ext)->w_evHandle[0]);
      ::CloseHandle(((sync_event*)ext)->w_evHandle[1]);
      ((sync_event*)ext)->r_evHandle[0] = ((sync_event*)ext)->r_evHandle[1] = INVALID_HANDLE_VALUE;
      ((sync_event*)ext)->w_evHandle[0] = ((sync_event*)ext)->w_evHandle[1] = INVALID_HANDLE_VALUE;
      delete ext;
      ext = NULL;
   }
}

#else   //SYNC MODE IMPL

void comm_engine::open(const string& name)
{
   CALL_LOGI;
   if (hcom != INVALID_HANDLE_VALUE)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Can not open twice.")));
      return;
   }

   LOGI << (boost::format("try to open device: %s baud rate %d")%name%baud_rate).str();
   string port_name = name;
   if(boost::algorithm::all(name, boost::algorithm::is_digit()))
   {
      port_name = string("\\\\.\\COM")+name;
   }
   else if(starts_with(name, "COM"))
   {
      port_name = string("\\\\.\\")+name;
   }

   uint32 retry = 20;
   while(retry > 0)
   {

      hcom = ::CreateFile(port_name.c_str(), GENERIC_READ | GENERIC_WRITE,
         0, NULL, OPEN_EXISTING, 0, NULL);

      if (hcom != INVALID_HANDLE_VALUE)
      {
         break;
      }

      LOGI << "retry CreateFile.";
      --retry;
      this_thread::sleep_for(boost::chrono::milliseconds(50));
   }

   if (hcom == INVALID_HANDLE_VALUE)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Create COM File failed.")));
   }

   DWORD com_error = 0;
   ::ClearCommError(hcom, &com_error, NULL);
   ::SetupComm(hcom, 16*1024, 16*1024); //16K cache.
   DCB dcb = {0};

   GetCommState(hcom, &dcb);

   dcb.BaudRate = baud_rate;
   dcb.fAbortOnError = FALSE;
   dcb.fBinary = 1;

   // Set 8/N/1
   dcb.ByteSize = 8;
   dcb.fParity = FALSE;
   dcb.Parity = NOPARITY;
   dcb.StopBits = ONESTOPBIT;

   // Disable H/W flow control
   dcb.fDtrControl = DTR_CONTROL_ENABLE;
   dcb.fRtsControl = RTS_CONTROL_ENABLE;
   dcb.fOutxCtsFlow = FALSE;
   dcb.fOutxDsrFlow = FALSE;
   dcb.fDsrSensitivity = FALSE;

   // Disable S/W flow control
   dcb.fOutX = FALSE;
   dcb.fInX = FALSE;

   if (!::SetCommState(hcom, &dcb))
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("SetCommState failed.")));
      return;
   }

   ::ClearCommBreak(hcom);
   ::EscapeCommFunction(hcom, SETRTS);
   ::EscapeCommFunction(hcom, SETDTR);
   ::PurgeComm(hcom, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);

   COMMTIMEOUTS timeout = {0};

   timeout.ReadIntervalTimeout         = 0;
   timeout.ReadTotalTimeoutMultiplier  = 1; //1;
   timeout.ReadTotalTimeoutConstant    = 20; //20;
   timeout.WriteTotalTimeoutMultiplier = 1;//1;
   timeout.WriteTotalTimeoutConstant   = 200;//200;

   ::SetCommTimeouts(hcom, &timeout);

   LOGI << (boost::format("%s open complete.")%name).str();
   return;
}


//return actual read bytes. timeout throw exception.
size_t comm_engine::receive(uint8* data, size_t length, uint32 timeout)
{
   DWORD num_bytes_read = 0;
   uint32 xferd_read = 0;
   uint32 start_time = ::GetTickCount();

   while(xferd_read < length)
   {
      // check if exceed timeout value
      if( (::GetTickCount()-start_time) >= timeout)
      {
         LOGE << (boost::format("Rx timeout in %dms")%timeout).str();
         break;
      }
      if (!::ReadFile(hcom, data+xferd_read, length-xferd_read, &num_bytes_read, NULL))
      {
         uint32 err = GetLastError();
         //if(GetLastError() != )
         LOGE << (boost::format("WRITE[0x%x], read[0x%x]")%err%num_bytes_read).str();
         //break;
      }
      xferd_read += num_bytes_read;

      if(ext)
      {
         LOGI << "Rx aborted by user.";
         break;
      }
   }

   if (xferd_read != length)
   {
      LOGE << (boost::format("Rx xferd length[0x%x] != expected length[0x%x]")%xferd_read%length).str();
      BOOST_THROW_EXCEPTION(runtime_exception(string("Rx data incomplete.")));
   }

   LOGD <<(boost::format("\t\t\t<-Rx: %s")%to_string(data, length)).str();
   return length;
}

//return actual send bytes. timeout throw exception.
size_t comm_engine::send(unsigned char* data, size_t length, unsigned int timeout)
{
   uint32 xferd_write = 0;
   DWORD num_sent = 0;
   uint32 start_time = ::GetTickCount();

   while (xferd_write < length)
   {
      if((::GetTickCount()-start_time) >= timeout)
      {
         LOGE << (boost::format("Tx timeout in %dms")%timeout).str();
         break;
      }
      if (!::WriteFile(hcom, data+xferd_write, length-xferd_write, &num_sent, NULL))
      {
         uint32 err = GetLastError();
         //if(GetLastError() != )
         LOGE << (boost::format("WRITE[0x%x], num sent[0x%x]")%err%num_sent).str();
         //break;
      }
      xferd_write += num_sent;

      if(ext)
      {
         LOGI << "Tx aborted by user.";
         break;
      }
   }

   if (xferd_write != length)
   {
      LOGE << (boost::format("Tx xferd length[0x%x] != expected length[0x%x]")%xferd_write%length).str();
      BOOST_THROW_EXCEPTION(runtime_exception(string("Tx data incomplete.")));
   }

   //::FlushFileBuffers(hcom);
   LOGD <<(boost::format("\t\t\tTx->: %s")%to_string(data, length)).str();
   return length;
}

void comm_engine::set_baudrate(uint32 baud)
{
   DCB dcb = {0};

   GetCommState(hcom, &dcb);

   dcb.BaudRate = baud;


   if (!::SetCommState(hcom, &dcb))
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("SetCommState failed.")));
      return;
   }

   ::EscapeCommFunction(hcom, SETRTS);
   ::EscapeCommFunction(hcom, SETDTR);
   ::PurgeComm(hcom, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
}

void comm_engine::cancel()
{
   CALL_LOGD;
   ext = (pvoid)0x1;
   return;
}

void comm_engine::close()
{
   CALL_LOGI;
   cancel();
   if (hcom != INVALID_HANDLE_VALUE)
   {
      DWORD com_error = 0;
      ::ClearCommError(hcom, &com_error, NULL);
      ::CloseHandle(hcom);
      hcom = INVALID_HANDLE_VALUE;
   }
   ext = NULL;
}

#endif





static void send_loop(comm_engine* com)
{
#define M2M 2*1024*1024
   unsigned char* buf1 = new unsigned char[M2M];
   memset(buf1, 'S', M2M);
   int n = 0;

   while(1)
   {
      std::cout << "Tx " << n++ <<std::endl;
      com->send(buf1, M2M, 7000);
      std::cout << "Tx OK" <<std::endl;
   }
}
static void recv_loop(comm_engine* com)
{
   unsigned char buf[5];
   buf[4] = 0;
   int n = 0;

   while(1)
   {
      std::cout << "Rx " << n++ <<std::endl;
      com->receive(buf, 4, 12000);
      std::cout << "Rx OK" <<std::endl;
   }
}

#define BAUD_CNT 1
static uint32 bauds[BAUD_CNT]
=
{
   115200
};
void comm_engine::test()
{
   CALL_LOGI;

   using namespace std;
   comm_engine com;

   com.open("COM24");

   try
   {
      uint8 s = 0xA0;
      uint8 ack = 0;
      int i = 0;
      while(1)
      {
         com.set_baudrate(bauds[i]);
         //Sleep(2);
         try
         {
            com.send(&s, 1, 20);
            com.receive(&ack, 1, 20);
            std::cout << std::dec<< "baud:" <<bauds[i] <<" "<< std::hex << ack << std::endl;
         }
         catch (std::exception&)
         {
            ++i;
            if(i == BAUD_CNT) i=0;
            continue;
         }
      }

      //boost::thread t1(boost::bind(&send_loop, &com));
      boost::thread t2(boost::bind(&recv_loop, &com));
      //t1.join();
      t2.join();

   }
   catch (std::exception& e)
   {
      std::cout << "Exception: " << e.what() << "\n";
   }

   return;
}