#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <string>

#include "arch/win/UsbScan.h"
#include <dbt.h>
#include <setupapi.h>
#include <algorithm>

//#include <initguid.h>
//DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

#pragma comment(lib, "Setupapi.lib")
using namespace std;

static GUID GUID_DEVINTERFACE_USB_DEVICE ={ 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
static const GUID GUID_DEVINTERFACE_LIST[] =
{
   // GUID_DEVINTERFACE_USB_DEVICE
   GUID_DEVINTERFACE_USB_DEVICE,
   //// GUID_DEVINTERFACE_COMPORT
   //{ 0x86e0d1e0, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } },
};

boost::container::map<HWND, UsbScan*> UsbScan::objectMap = boost::container::map<HWND, UsbScan*>();

UsbScan::UsbScan(void)
: hWnd(NULL)
{
}

UsbScan::~UsbScan(void)
{
}

static string& replace_all(string& str, const string& old_value, const string& new_value)
{
   while(TRUE)
   {
      string::size_type pos(0);
      if((pos = str.find(old_value)) != string::npos)
      {
         str.replace(pos,old_value.length(),new_value);
      }
      else
      {
         break;
      }
   }
   return str;
}

static string GetClassDesc(const GUID* pGuid)
{
   char buf[MAX_PATH];
   DWORD size;
   if ( SetupDiGetClassDescription(pGuid, buf, sizeof(buf), &size) ) {
      return string(buf);
   } else {
      cout << "Can't get class description" << endl;
      return string("");
   }
}


BOOL UsbScan::FindDevice(LPARAM pHdr)
{
   // pDevInf->dbcc_name:
   // \\?\USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
   // szDevId: USB\Vid_04e8&Pid_503b\0002F9A9828E0F06
   // szClass: USB
   PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
   devInfo->dbcc_name = pDevInf->dbcc_name;

   string szDevId = pDevInf->dbcc_name+4;
   std::size_t idx = szDevId.rfind("#");
   szDevId = szDevId.substr(0, idx);

   szDevId = replace_all(szDevId, "#", "\\");
   transform(szDevId.begin(), szDevId.end(), szDevId.begin(), toupper);

   idx = szDevId.find("\\");
   string szClass = szDevId.substr(0, idx);

   // Only process USB
   if ( "USB" != szClass ) {
      return FALSE;
   }

   HDEVINFO hDevInfo = SetupDiGetClassDevs(
       &GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_DEVICEINTERFACE| DIGCF_PRESENT
       );

   if(INVALID_HANDLE_VALUE == hDevInfo)
   {
      return FALSE;
   }

   SP_DEVINFO_DATA spDevInfoData;
   spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

   BOOL deviceFound = FALSE;
   for(int k=0; SetupDiEnumDeviceInfo(hDevInfo, k, &spDevInfoData); k++)
   {
      DWORD nSize=0 ;
      char buf[MAX_PATH];

      if(!SetupDiGetDeviceInstanceId(hDevInfo, &spDevInfoData, buf, sizeof(buf), &nSize))
      {
         break;
      }
      if(szDevId == buf)
      {
         if(filter->match(szDevId))
         {
            deviceFound = TRUE;
            break;
         }
      }
   }

   if (deviceFound)
   {
      DWORD DataT ;
      char buf[MAX_PATH];
      DWORD nSize = 0;

      devInfo->device_instance_id = szDevId;
      // get Friendly Name or Device Description
      if ( SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData,
         SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buf, sizeof(buf), &nSize) ) {
      } else if ( SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData,
         SPDRP_DEVICEDESC, &DataT, (PBYTE)buf, sizeof(buf), &nSize) ) {
      } else {
         lstrcpy(buf, "Unknown");
      }

      devInfo->device_class = GetClassDesc(&(spDevInfoData.ClassGuid));
      devInfo->friendly_name = buf;

      std::size_t idx1 = devInfo->friendly_name.rfind("COM");
      std::size_t idx2 = devInfo->friendly_name.rfind(")");
      if(idx1 != string::npos && idx2 != string::npos)
      {
         devInfo->port_name = devInfo->friendly_name.substr(idx1, idx2 - idx1);
      }

      //get symbolic link name
      DWORD i = 0;
      SP_DEVICE_INTERFACE_DATA devInterfaceData;
      devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
      SP_DEVICE_INTERFACE_DETAIL_DATA* pDetailData = NULL;
      DWORD ReqLen = 0;

      while(1)
      {
         if(SetupDiEnumDeviceInterfaces(hDevInfo,  &spDevInfoData, &GUID_DEVINTERFACE_USB_DEVICE, i++, &devInterfaceData))
         {
            // Get size of symbolic link name
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, NULL, 0, &ReqLen, NULL);
            boost::scoped_ptr<char> pData(new char[ReqLen]);
            pDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)(pData.get());
            pDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

            // Get symbolic link name
            if(SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, pDetailData, ReqLen, NULL, &spDevInfoData))
            {
               devInfo->symbolic_link_name = pDetailData->DevicePath;
               break;
            }
         }
         else
         {
            cout << "Symbolic link get error: " << GetLastError() << endl;
            break;
         }
      }
   }

   SetupDiDestroyDeviceInfoList(hDevInfo);

   return deviceFound;
}

BOOL UsbScan::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
   BOOL result = FALSE;
   if ( DBT_DEVICEARRIVAL == wParam )
   {
      PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
      PDEV_BROADCAST_DEVICEINTERFACE pDevInf;
      PDEV_BROADCAST_HANDLE pDevHnd;
      PDEV_BROADCAST_OEM pDevOem;
      PDEV_BROADCAST_PORT pDevPort;
      PDEV_BROADCAST_VOLUME pDevVolume;
      switch( pHdr->dbch_devicetype )
      {
      case DBT_DEVTYP_DEVICEINTERFACE:
         pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
         result = FindDevice((LPARAM)pDevInf);
         break;

      case DBT_DEVTYP_HANDLE:
         pDevHnd = (PDEV_BROADCAST_HANDLE)pHdr;
         break;

      case DBT_DEVTYP_OEM:
         pDevOem = (PDEV_BROADCAST_OEM)pHdr;
         break;

      case DBT_DEVTYP_PORT:
         pDevPort = (PDEV_BROADCAST_PORT)pHdr;
         break;

      case DBT_DEVTYP_VOLUME:
         pDevVolume = (PDEV_BROADCAST_VOLUME)pHdr;
         break;
      }
   }
   else if(DBT_DEVICEREMOVECOMPLETE == wParam)
   {
      cout << "info: device removed." <<endl;
   }
   return result;
}

HRESULT CALLBACK UsbScan::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_DEVICECHANGE:
      if(objectMap[hWnd]->OnDeviceChange(wParam, lParam))
      {
         objectMap[hWnd]->found = TRUE;
         PostQuitMessage(0);
      }
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

ATOM UsbScan::RegisterWindowClass(void)
{
   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style			= CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc	= &UsbScan::WndProc;
   wcex.cbClsExtra		= 0;
   wcex.cbWndExtra		= 0;
   wcex.hInstance		= GetModuleHandle(NULL);
   wcex.hIcon			= NULL;
   wcex.hCursor		= NULL;
   wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName	= NULL;
   wcex.lpszClassName	= "emptyWnd";
   wcex.hIconSm		= NULL;

   return RegisterClassEx(&wcex);
}

BOOL UsbScan::wait_for_device(/*IN*/usb_filter* usbFilter, /*IN OUT*/usb_device_info* dev, const int time_out)
{
   if(dev == NULL || usbFilter == NULL)
   {
      cout << "paramters cannot be null." << endl;
      return FALSE;
   }

   RegisterWindowClass();
   hWnd = CreateWindow("emptyWnd", "T", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   objectMap[hWnd] = this;
   devInfo = dev;
   filter = usbFilter;
   found = FALSE;

   HDEVNOTIFY hDevNotify;
   DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
   ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
   NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
   NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

   for(int i=0; i<sizeof(GUID_DEVINTERFACE_LIST)/sizeof(GUID); i++)
   {
      NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_LIST[i];
      hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
      if( !hDevNotify )
      {
         std::cout << "Can't register device notification: " << std::endl;
         return FALSE;
      }
   }

   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0))
   {
      DispatchMessage(&msg);
   }

   UnregisterDeviceNotification(hDevNotify);

   BOOL device_found = objectMap[hWnd]->found;
   objectMap.erase(hWnd);
   return device_found;
}

void UsbScan::stop()
{
   if(hWnd != NULL)
   {
      SendMessage(hWnd, WM_DESTROY, 0, 0);
   }
}


void UsbScan::test()
{
   UsbScan usbScan;
   usb_device_info dev_info;
   usb_filter filter;
   filter.add_filter("USB\\VID_0E8D&PID_2000");
   filter.add_filter("USB\\VID_0E8D&PID_0003");

   cout << "Wait for device. " << endl;
   usbScan.wait_for_device(&filter, &dev_info);

   cout <<"Port: " <<dev_info.port_name << endl;
   cout <<"Device Class: " <<dev_info.device_class << endl;
   cout <<"Friendly Name: " <<dev_info.friendly_name << endl;
   cout <<"Instance Id: " <<dev_info.device_instance_id << endl;
   cout <<"Interface Symbolic Link name: " <<dev_info.symbolic_link_name << endl;
   cout << "Device Name: " <<dev_info.dbcc_name << endl;
   return;
}


