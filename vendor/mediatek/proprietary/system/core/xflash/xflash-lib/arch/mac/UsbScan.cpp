#include "UsbScan.h"
#include "common/zlog.h"
#include <qdebug.h>
#include <QTime>
#include <boost/format.hpp>

UsbScan::UsbScan()
{

}

UsbScan::~UsbScan()
{

}

BOOL UsbScan::wait_for_device(usb_filter *filter, usb_device_info *dev_info, const int time_out){

    QTime time;
    time.start();

    while(stop_flag != TRUE){
        if(time.elapsed() > time_out){
            LOGW << (boost::format("Search com port timeout in %dms")%time_out).str();
            return FALSE;
        }

        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            if(info.hasProductIdentifier() && info.hasVendorIdentifier()){
                qDebug() << "Name           :" << info.portName() << "\n";
                qDebug() << "Product Identifier  :" << info.productIdentifier() << "\n";
                qDebug() << "Vendor Identifier  :" << info.vendorIdentifier() << "\n";
                string dev_id = (boost::format("USB\\VID_%04x&PID_%04x")%info.vendorIdentifier()%info.productIdentifier()).str();
                char usbIdentifier[100] = {0};
                sprintf(usbIdentifier, "USB\\VID_%04x&PID_%04x", info.vendorIdentifier(), info.productIdentifier());
                qDebug() << "USB ID:   " << usbIdentifier << "\n";
                if(filter->match(dev_id)){
                    char devicepath[32];
                    memset(devicepath, 0x00, 32);
                    sprintf(devicepath, "/dev/%s", info.portName().toLocal8Bit().constData());
                    dev_info->symbolic_link_name = string(devicepath);
                    dev_info->port_name = string(devicepath);
                    qDebug() << "device path: " << devicepath << "\n";
                    return TRUE;
                }

            }
        }
    }

    return TRUE;
}

void UsbScan::stop(){
    stop_flag = TRUE;
}
