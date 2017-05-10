#include "transfer/comm_engine.h"
#include "common/zlog.h"
#include <boost/format.hpp>
#include <unistd.h>
#include <QTime>

#define MAX_RETRY_COUNT 20

#include "common/common_include.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/timeb.h>
#include <sys/param.h>
#include <IOKit/serial/ioss.h>

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif

#define CBAUDEX 0x0010000

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
   {230400    ,B230400 }
};
//-----------------------------------------------------------------------------
static speed_t get_speed(uint32 baudrate)
{
   uint32 idx;
   for (idx = 0; idx < sizeof(speeds)/sizeof(speeds[0]); idx++)
   {
      if (baudrate == (uint32)speeds[idx].baud)
      {
         return speeds[idx].speed;
      }
   }
   return CBAUDEX;
}

static ulong32 get_tick_ms(void)
{
    struct timeval current;
    gettimeofday(&current, NULL);

    return current.tv_sec  + current.tv_usec/1000;
}

static struct termios gOriginalTTYAttrs;

comm_engine::comm_engine()
: hcom(INVALID_HANDLE_VALUE)
, ext(NULL)
, baud_rate(115200)
{
}

comm_engine::comm_engine(STD_::string str_baud_rate)
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
       hcom = ::open(name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

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
#if 0
   fcntl(hcom, F_SETFD, FD_CLOEXEC);
#endif

   if(ioctl(hcom, TIOCEXCL) == -1){
       LOGE << (boost::format("Error setting TIOCEXCL on %s - %s(%d).\n")%name%strerror(errno)%errno).str();
       BOOST_THROW_EXCEPTION(runtime_exception(
          string("Set COM TIOCEXCL error.")));
       return;
   }

   if(fcntl(hcom, F_SETFL, 0) == -1){
       LOGE << (boost::format("Error clearing O_NONBLOCK  %s - %s(%d).\n")%name%strerror(errno)%errno).str();
       BOOST_THROW_EXCEPTION(runtime_exception(
          string("Error clearing O_NONBLOCK.")));
       return;
   }

   struct termios  dcb;

   if (tcgetattr(hcom, &gOriginalTTYAttrs) == -1)
   {
       LOGE << (boost::format("Error getting tty attributes %s - %s(%d).\n")%name%strerror(errno)%errno).str();
       BOOST_THROW_EXCEPTION(runtime_exception(
                                 string("Get COM parameters error.")));
       return;
   }

   dcb = gOriginalTTYAttrs;

   cfmakeraw(&dcb);
   dcb.c_cc[VMIN] = 0;
   dcb.c_cc[VTIME] = 10;

   cfsetspeed(&dcb, B115200);
   dcb.c_cflag |= (CS7 | PARENB | CCTS_OFLOW | CRTS_IFLOW);

  speed_t speed = 115200;
  if(ioctl(hcom, IOSSIOSPEED, &speed) == -1){
      LOGE << (boost::format("Error calling ioctl(..., IOSSIOSPEED, ...)%s - %s(%d).\n")%name%strerror(errno)%errno).str();
      BOOST_THROW_EXCEPTION(runtime_exception(
                                string("Error calling ioctl(..., IOSSIOSPEED, ...).")));
      return;
  }

  if(tcsetattr(hcom, TCSANOW, &dcb) == -1){
      LOGE << (boost::format("Error setting tty attributes %s - %s(%d).\n")%name%strerror(errno)%errno).str();
      BOOST_THROW_EXCEPTION(runtime_exception(
                                string("Error setting tty attributes.")));
      return;
  }

  if(ioctl(hcom, TIOCSDTR) == -1){
      LOGE << (boost::format("Error asserting DTR %s - %s(%d).\n")%name%strerror(errno)%errno).str();
     // BOOST_THROW_EXCEPTION(runtime_exception(
       //                         string("Error asserting DTR.")));
      //return;
  }

  if(ioctl(hcom, TIOCCDTR) == -1){
      LOGE << (boost::format("Error clearing DTR %s - %s(%d).\n")%name%strerror(errno)%errno).str();
    //  BOOST_THROW_EXCEPTION(runtime_exception(
      //                          string("Error clearing DTR.")));
      //return;
  }

  int handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
  if(ioctl(hcom, TIOCMSET, &handshake) == -1){
      LOGE << (boost::format("Error setting handshake line %s - %s(%d).\n")%name%strerror(errno)%errno).str();
     // BOOST_THROW_EXCEPTION(runtime_exception(
       //                         string("Error clearing DTR.")));
      //return;
  }
/*
   dcb.c_cflag = (dcb.c_cflag & ~CSIZE) | CS8;
   dcb.c_iflag &= ~IGNBRK;
   dcb.c_lflag = 0;
   dcb.c_oflag = 0;

   dcb.c_iflag &= ~(IXON | IXOFF | IXANY);
   dcb.c_cflag |= (CLOCAL | CREAD);
   dcb.c_cflag &= ~(PARENB | PARODD);
   dcb.c_cflag |= 0; // No parity
   dcb.c_cflag &= ~CSTOPB;
   dcb.c_cflag &= ~CRTSCTS;

   tcsetattr(hcom, TCSANOW, &dcb);
*/
 //  set_baudrate(115200);
#if 0
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
#endif
   //use ext as user abort flag;
   ext = NULL;
   LOGI <<(boost::format("%s open complete.")%name).str();
   return;
}

/*void comm_engine::set_baudrate(uint32 baud){
    struct termios newtio;
    struct termios oldtio;
    speed_t baudrate_t = get_speed(baud);
   // int ret = 0;
    tcgetattr(hcom, &oldtio);
    newtio = oldtio;

    cfmakeraw(&newtio);

    newtio.c_cc[VTIME] = 5;
    newtio.c_cc[VMIN] = 0;
    cfsetospeed(&newtio, baudrate_t);
    cfsetispeed(&newtio, baudrate_t);
    tcflush(hcom, TCIFLUSH);

    tcsetattr(hcom, TCSANOW, &newtio);
}
*/
//return actual read bytes. timeout throw exception.
size_t comm_engine::receive(uint8* data, size_t length, uint32 timeout)
{
   //CALL_LOGV;
   uint32 begin = get_tick_ms();
   uint32 xferd_read = 0;
   int32 rc = -1;
   ulong32 num_bytes_read = 0;
   uint32 current = 0;

   while (xferd_read < length)
   {
      //use ext as user abort flag;
      if(ext)
      {
         LOGW << "Rx abort.";
         throw boost::thread_interrupted();
         break;
      }

      current = get_tick_ms();

      if((get_tick_ms() - begin > timeout))
      {
         LOGW << (boost::format("Rx timeout in %dms")%timeout).str();
         break;
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
   ulong32 begin = get_tick_ms();
   uint32 xferd_write = 0;
   int32 rc = -1;

   while (xferd_write < length)
   {
       ulong32 num_sent = 0;
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
         break;
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
       tcsetattr(hcom, TCSANOW, &gOriginalTTYAttrs);

      ::close(hcom);
      hcom = INVALID_HANDLE_VALUE;
   }
}

void comm_engine::test()
{
}

#define BAUD_CNT 1
static uint32 bauds[BAUD_CNT] =
{
   115200
};

