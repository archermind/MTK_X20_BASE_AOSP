#ifndef LINUX_USBSCANTHREAD_H
#define LINUX_USBSCANTHREAD_H

#include "arch/IUsbScan.h"

using namespace std;

class UsbScan : IMPLEMENTS public IUsbScan
{
public:
   UsbScan(void);
   ~UsbScan(void);
public:
   BOOL wait_for_device(/*IN*/usb_filter* filter, /*IN OUT*/usb_device_info* dev_info, const int time_out = 60000);
   void stop();
   static void test();
private:
   int init_hotplug_sock();
   int hotplug_socket;
   BOOL stop_flag;
};


#endif // DEVICESCANTHREAD_H
