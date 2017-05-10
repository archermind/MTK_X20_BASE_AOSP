#ifndef	_IUSB_SCAN_H_
#define	_IUSB_SCAN_H_

#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <algorithm>
#include "common/types.h"
#include <boost/algorithm/string.hpp>

using namespace std;

class usb_filter
{
public:
   //some string like "USB\VID_22B8&PID_41DB&MI_01"
   void add_filter(string s)
   {
      boost::algorithm::to_upper(s);
      filter_list.push_back(s);
   }
   BOOL match(string source)
   {
      boost::algorithm::to_upper(source);
      BOOST_FOREACH(string& id, filter_list)
      {
         if(source.find(id) != string::npos)
         {
            return TRUE;
         }
      }
      return FALSE;
   }
private:
   boost::container::list<std::string> filter_list;
};

struct usb_device_info
{
public:
   string device_instance_id;
   string symbolic_link_name;
   string device_class;
   string friendly_name;
   string port_name;
   string dbcc_name;
};

DECLEAR_INTERFACE class IUsbScan
{
public:
   virtual BOOL wait_for_device(usb_filter* filter, usb_device_info* dev_info, const int time_out) = 0;
   virtual void stop() = 0;
public:
   virtual ~IUsbScan(){}
};

#if defined(_WIN32)
#include "win/UsbScan.h"
#elif defined(_LINUX)
#include "linux/UsbScan.h"
#else
#include "mac/UsbScan.h"
#endif


#endif
