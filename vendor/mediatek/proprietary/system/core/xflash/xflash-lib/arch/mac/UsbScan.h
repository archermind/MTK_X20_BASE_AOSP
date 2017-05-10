#ifndef USBSCAN_H
#define USBSCAN_H

#include <qserialportinfo.h>
#include "arch/IUsbScan.h"

class UsbScan: IMPLEMENTS public IUsbScan
{
public:
    UsbScan();
    ~UsbScan();

public:
    BOOL wait_for_device(usb_filter *filter, usb_device_info *dev_info, const int time_out);
    void stop();

private:
    BOOL stop_flag;
};

#endif // USBSCAN_H
