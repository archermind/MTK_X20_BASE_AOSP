#pragma once
#include <windows.h>
#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <algorithm>
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
   usb_device_info* devInfo;
   usb_filter* filter;
   HWND hWnd;
   BOOL found;
   static boost::container::map<HWND, UsbScan*> objectMap;
   static HRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   BOOL OnDeviceChange(WPARAM wParam, LPARAM lParam);
   BOOL FindDevice(LPARAM pHdr);
   ATOM RegisterWindowClass(void);


};
