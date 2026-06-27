#include "usbCom.h"
#include <iostream>

int main()
{
    UsbCom usb("/dev/ttyUSB2", 115200);

    if (!usb.openPort())
    {
        return 1;
    }

    if (!usb.sendAT())
    {
        return 1;
    }

    if (!usb.GPSOn())
    {
        return 1;
    }

    // std::string GPSInfo = usb.GPSRead();
    // auto fields = usb.split(GPSInfo, ',');

    return 0;
}