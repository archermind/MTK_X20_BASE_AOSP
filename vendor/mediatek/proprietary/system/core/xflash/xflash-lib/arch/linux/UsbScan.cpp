#include "arch/linux/UsbScan.h"
#include "type_define.h"
#include "common/common_include.h"

#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/timeb.h>


static uint32 get_tick_ms(void)
{
   struct timeb tb;
   ::ftime(&tb);
   //since 1970 to 2010 to prevent 32bit overflow.
   return (tb.time-0x4B300C00)*1000+tb.millitm;
}

#define UEVENT_BUFFER_SIZE 2048

UsbScan::UsbScan()
:stop_flag(FALSE)
,hotplug_socket(-1)
{
}

UsbScan::~UsbScan()
{
   if(hotplug_socket > 0)
   {
      ::close(hotplug_socket);
   }
}

int UsbScan::init_hotplug_sock()
{
   const int buffer_size = UEVENT_BUFFER_SIZE;
   struct sockaddr_nl snl;
   bzero(&snl, sizeof(struct sockaddr_nl));
   snl.nl_family = AF_NETLINK;
   snl.nl_pid = getpid();
   snl.nl_groups = 1;

   hotplug_socket = ::socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

   if(hotplug_socket == -1)
   {
      LOGI <<"create socket error.";

      return -1;
   }

   int tmp = 1;

   ::setsockopt(hotplug_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
   ::setsockopt(hotplug_socket, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));

   int ret = ::bind(hotplug_socket, (struct sockaddr*)&snl, sizeof(struct sockaddr_nl));
   if(ret < 0)
   {
      LOGI << "bind socket error.";
      return -1;
   }

   return hotplug_socket;
}

BOOL UsbScan::wait_for_device(/*IN*/usb_filter* filter, /*IN OUT*/usb_device_info* dev_info, const int time_out)
{
   hotplug_socket = init_hotplug_sock();
   if(hotplug_socket == -1)
   {
      return FALSE;
   }

   struct timeval tv;
   int ret, recv_len;

   uint32 begin = get_tick_ms();

   while(!stop_flag)
   {
      char buf[UEVENT_BUFFER_SIZE*2] = {0};

      if (get_tick_ms() - begin > 60000)
      {
         LOGE << "searching USB port Timeout 60 seconds";
         return FALSE;
      }

      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(hotplug_socket, &fds);

      tv.tv_sec = 0;
      tv.tv_usec = 100 * 1000;

      ret = ::select(hotplug_socket + 1, &fds, NULL, NULL, &tv);

      if(ret < 0)
         continue;

      if(!FD_ISSET(hotplug_socket, &fds))
         continue;

      recv_len = recv(hotplug_socket, &buf, UEVENT_BUFFER_SIZE*2, 0);
      if(recv_len <= 0)
      {
         LOGE<<"COM LINK receive error.";
         return FALSE;
      }

      LOGI <<"Usb Scan Info: " << buf;
      string scan_info(buf);

      string act_add = "add@";
      string act_rm = "remove@";
      string tty = "/tty/ttyACM";
      string tty_acm = "/ttyACM";
      string sys = "/sys";
      string symbolic_pid = "/idProduct";
      string symbolic_vid = "/idVendor";

      //buffer is lile this: "add@/devices/pci0000:00/0000:00:1d.0/usb6/6-1/6-1:1.0/tty/ttyACM0"
      //abstract pid/vid name /sys/devices/pci0000:00/0000:00:1d.0/usb6/6-1/idProduct
      if((scan_info.find(act_add) != string::npos) && (scan_info.find(tty_acm) != string::npos))
      {
         scan_info.erase(scan_info.find(act_add), act_add.size());
         //"/devices/pci0000:00/0000:00:1d.0/usb6/6-1/6-1:1.0/tty/ttyACM0"
         string dev_name = scan_info.substr(scan_info.find(tty));
         //"/tty/ttyACM0"
         scan_info.erase(scan_info.find(tty));

         //"/devices/pci0000:00/0000:00:1d.0/usb6/6-1/6-1:1.0"
         scan_info.erase(scan_info.rfind("/"));
         //"/devices/pci0000:00/0000:00:1d.0/usb6/6-1/"
         string dev_path = sys + scan_info;
         //"/sys/devices/pci0000:00/0000:00:1d.0/usb6/6-1/"
         string vid = dev_path + symbolic_vid;
         string pid = dev_path + symbolic_pid;

         #define BUFFER_LEN 8
         char pid_buf[BUFFER_LEN] = {0};
         char vid_buf[BUFFER_LEN] = {0};

         int fd = open(vid.c_str(), O_RDONLY);
         read(fd, vid_buf, 4);
         ::close(fd);

         fd = open(pid.c_str(), O_RDONLY);
         read(fd, pid_buf, 4);
         ::close(fd);

         string sz_dev_id = (boost::format("USB\\VID_%s&PID_%s")%vid_buf%pid_buf).str();
         if(filter->match(sz_dev_id))
         {
            string device_symbolic = string("/dev") + dev_name.substr(dev_name.find(tty_acm));
            // "/dev/ttyACM0"
std::cout<<"8"<<device_symbolic<<std::endl;
            LOGI << "com port name is: " << device_symbolic;
            dev_info->device_instance_id = device_symbolic;
            dev_info->symbolic_link_name = device_symbolic;
            dev_info->friendly_name = device_symbolic;
            dev_info->dbcc_name = device_symbolic;
            dev_info->port_name = device_symbolic;
            dev_info->device_class = "USB";
            return TRUE;
         }

      }
      else if(scan_info.find(act_rm) != string::npos)
      {
         std::cout << scan_info;
         continue;
      }
      else
      {
         continue;
      }
   }
   return FALSE;
}

void UsbScan::stop()
{
   stop_flag = TRUE;
}

