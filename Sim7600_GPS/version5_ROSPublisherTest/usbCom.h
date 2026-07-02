#ifndef USBCOM_H
#define USBCOM_H

#include <string>
#include <vector>

class UsbCom
{
public:
    UsbCom(const std::string &device, int baudRate = 115200);
    ~UsbCom();

    bool openPort();
    void closePort();
    bool sendAT();
    bool GPSOn();
    std::string GPSRead();

private:
    std::string device_;
    int baudRate_;
    int fd_;

    void configureSerial();
    bool waitForOK(int timeoutSeconds = 3);
};

static std::vector<std::string> split(const std::string &s, char delim);

#endif
