#if defined(_LINUX)
#include "common/common_include.h"
#include "transfer/comm_engine.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <sys/timeb.h>

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif

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

struct speed_map {
   unsigned int baud;
   speed_t      speed;
};
//-----------------------------------------------------------------------------
static struct speed_map speeds[] = {
   {50        ,B50     },
   {75        ,B75     },
   {110       ,B110    },
   {134       ,B134    },
   {150       ,B150    },
   {200       ,B200    },
   {300       ,B300    },
   {600       ,B600    },
   {1200      ,B1200   },
   {1800      ,B1800   },
   {2400      ,B2400   },
   {4800      ,B4800   },
   {9600      ,B9600   },
   {19200     ,B19200  },
   {38400     ,B38400  },
   {57600     ,B57600  },
   {115200    ,B115200 },
   {230400    ,B230400 },
   {460800    ,B460800 },
   {500000    ,B500000 },
   {576000    ,B576000 },
   {921600    ,B921600 },
   {1000000   ,B1000000},
   {1152000   ,B1152000},
   {1500000   ,B1500000},
   {2000000   ,B2000000},
   {2500000   ,B2500000},
   {3000000   ,B3000000},
   {3500000   ,B3500000},
   {4000000   ,B4000000},
};
//-----------------------------------------------------------------------------
static speed_t get_speed(uint32 baudrate)
{
   uint32 idx;
   for (idx = 0; idx < sizeof(speeds)/sizeof(speeds[0]); idx++)
   {
      if (baudrate == (int)speeds[idx].baud)
      {
         return speeds[idx].speed;
      }
   }
   return CBAUDEX;
}

static uint32 get_tick_ms(void)
{
    struct timeb tb;
    ::ftime(&tb);
    //since 1970 to 2010 to prevent 32bit overflow.
    return (tb.time-0x4B300C00)*1000+tb.millitm;
}


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

void comm_engine::open(const string& name)
{
   CALL_LOGI;
   if (hcom >= 0)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Can not open twice.")));
      return;
   }

   LOGI << (boost::format("try to open serial port: %s baud rate %d")%name%baud_rate).str();

   uint32 retry = 20;
   while(retry > 0)
   {
      hcom = ::open(name.c_str(), O_RDWR|O_NOCTTY|O_NONBLOCK,0);

      if (hcom >= 0)
      {
         break;
      }
      --retry;
      usleep(50*1000);//50ms
   }

   if(retry <= 0)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
            string("Create COM File failed.")));
   }

   fcntl(hcom, F_SETFD, FD_CLOEXEC);

   struct termios  dcb;

   if (tcgetattr(hcom, &dcb) != 0)
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Get COM parameters error.")));
      return;
   }

   cfmakeraw(&dcb);
   //8N1
   dcb.c_cflag &= ~(PARENB|CSTOPB|CSIZE);
   dcb.c_cflag |= CS8;
   //disable hw flow control
   dcb.c_cflag &= ~CRTSCTS;
   //disable sw flow control
   dcb.c_cflag &= ~(IXON|IXOFF|IXANY);

   //set timeout
   dcb.c_cc[VMIN] = 0;
   dcb.c_cc[VTIME] = 1;//100ms return

   //set baud rate
   speed_t speed = get_speed(baud_rate);
   cfsetospeed(&dcb, speed);
   cfsetispeed(&dcb, speed);

   int32 sig = TIOCM_RTS | TIOCM_DTR;

   if ((tcsetattr(hcom, TCSANOW, &dcb) != 0)
    || (ioctl(hcom, TIOCCBRK, 0) != 0)
    || (tcflush(hcom, TCIOFLUSH) != 0)
    || (ioctl(hcom, TIOCMBIS, &sig) != 0))
   {
      BOOST_THROW_EXCEPTION(runtime_exception(
         string("Set COM parameters error.")));
      return;
   }

   //use ext as user abort flag;
   ext = NULL;
   LOGI <<(boost::format("%s open complete.")%name).str();
   return;
}

//return actual read bytes. timeout throw exception.
size_t comm_engine::receive(uint8* data, size_t length, uint32 timeout)
{
   //CALL_LOGV;
   uint32 begin = get_tick_ms();
   uint32 xferd_read = 0;
   int32 rc = -1;

   while (xferd_read < length)
   {
      //use ext as user abort flag;
      if(ext)
      {
         LOGW << "Rx abort.";
         throw boost::thread_interrupted();
         break;
      }

      if((get_tick_ms() - begin > timeout))
      {
         LOGW << (boost::format("Rx timeout in %dms")%timeout).str();
      }
      rc = ::read(hcom, data+xferd_read, length-xferd_read);
      if (rc < 0)
      {
	if(errno == 11)
		continue;

        LOGE << (boost::format("::read com return: %d errno %d. %s")%rc%errno%strerror(errno)).str();
        break;
      }
      xferd_read += rc;
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
   uint32 begin = get_tick_ms();
   uint32 xferd_write = 0;
   int32 rc = -1;

   while (xferd_write < length)
   {
      //use ext as user abort flag;
      if(ext)
      {
         LOGW << "Rx abort.";
         throw boost::thread_interrupted();
         break;
      }

      if((get_tick_ms() - begin > timeout))
      {
         LOGW << (boost::format("Rx timeout in %dms")%timeout).str();
      }
      rc = ::write(hcom, data+xferd_write, length-xferd_write);
      if (rc < 0)
      {
	if(errno == 11)
		continue;

         LOGE << (boost::format("::write com return: %d errno %d. %s")%rc%errno%strerror(errno)).str();
         break;
      }
      xferd_write += rc;
   }

   if (xferd_write != length)
   {
      LOGW << (boost::format("Tx xferd length[0x%x] != expected length[0x%x]")%xferd_write%length).str();
      BOOST_THROW_EXCEPTION(runtime_exception(string("Tx data incomplete.")));
   }

   LOGD <<(boost::format("\t\t\tTx->: %s")%to_string(data, length)).str();
   return length;
}

void comm_engine::cancel()
{
   CALL_LOGD;
   //use this as user abort flag;
   ext = (pvoid)0x1;
   return;
}

void comm_engine::close()
{
   CALL_LOGI;
   cancel();
   if (hcom != INVALID_HANDLE_VALUE)
   {
      ::close(hcom);
      hcom = INVALID_HANDLE_VALUE;
   }
}

/*
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
*/
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
         //com.set_baudrate(bauds[i]);
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
      //boost::thread t2(boost::bind(&recv_loop, &com));
      //t1.join();
      //t2.join();

   }
   catch (std::exception& e)
   {
      std::cout << "Exception: " << e.what() << "\n";
   }

   return;
}


#endif
