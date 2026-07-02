#include "usbCom.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>

// Constructor (Creating UsbCom Object, provides port ["ttyUSB2"] and baudrate [115200])
UsbCom::UsbCom(const std::string &device, int baudRate)
    : device_(device), baudRate_(baudRate), fd_(-1) {}

// Destructor
UsbCom::~UsbCom()
{
    closePort();
}

/**************************Port Configurations**************************/

/**
 * Trys to open the port provided
 *
 * @return false if failed connnection, or true if successful.
 */
bool UsbCom::openPort()
{
    fd_ = open(device_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0)
    {
        std::cerr << "Failed to open " << device_ << ": " << strerror(errno) << '\n';
        return false;
    }

    try
    {
        configureSerial();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        closePort();
        return false;
    }

    return true;
}

/**
 * Closes the object's device (port)
 */
void UsbCom::closePort()
{
    if (fd_ >= 0)
    {
        close(fd_);
        fd_ = -1;
    }
}

/**
 * Configure communication settings with the device/port
 * Used as part of the openPort function
 */
void UsbCom::configureSerial()
{
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }

    cfmakeraw(&tty);

    speed_t speed = B115200;
    // Spped based on Baudrate for port/device
    // Change speed depending on baudrate (115200 for Sim7600)
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }
}

/**************************Power on Sim7x00**************************/

/**
 * Opens communication with Sim7600 by sending AT to the port
 * Should receive OK if no errors
 *
 * @return false if failed connnection, or true if successful.
 */
bool UsbCom::sendAT()
{
    if (fd_ < 0) // Check if fd_ is still open
        return false;

    const char *cmd = "AT\r"; // Set command msg AT
    ssize_t n = write(fd_, cmd, strlen(cmd));
    if (n < 0)
    {
        std::cerr << "Write failed: " << strerror(errno) << '\n';
        return false;
    }

    if (UsbCom::waitForOK(3))
    {
        return true;
    }
    return false;
}

/**
 * After sending AT command, reads port for OK feedback
 * If not received, resends AT command, if received ends
 * Timeout default set to 3 seconds (Can be inputted)
 *
 * @return false if failed reading, true if OK received.
 */
bool UsbCom::waitForOK(int timeoutSeconds)
{
    if (fd_ < 0) // Check if fd_ is still open
        return false;

    std::string received;
    received.erase();
    char buffer[256];

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_, &readfds);

        timeval timeout;
        timeout.tv_sec = timeoutSeconds;
        timeout.tv_usec = 0;

        int ret = select(fd_ + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0)
        {
            std::cerr << "select failed: " << strerror(errno) << '\n';
            return false;
        }

        if (ret == 0)
        {
            std::cout << "No reply yet, resending AT...\n";
            received.clear();
            if (!sendAT())
                return false;
            continue;
        }

        ssize_t n = read(fd_, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "Read failed: " << strerror(errno) << '\n';
            return false;
        }

        if (n > 0)
        {
            buffer[n] = '\0';
            received += buffer;
            std::cout << buffer << std::flush;

            if (received.find("OK") != std::string::npos)
            {
                std::cout << "\nReceived OK\n";
                return true;
            }
            // Errpr search included for troubleshooting purposes
            if (received.find("ERROR") != std::string::npos)
            {
                std::cout << "\nReceived ERROR\n";
                return true;
            }
        }
    }
}

/**************************GPS Functions**************************/

/**
 * Activates Sim7600 GPS by sending AT+CGPS=1 to the port
 * Should receive OK if no errors
 *
 * @return false if failed activation, true if successful.
 */
bool UsbCom::GPSOn()
{
    if (fd_ < 0) // Check if fd_ is still open
        return false;

    const char *cmd = "AT+CGPS=1\r"; // Msg for activating Sim GPS
    ssize_t n = write(fd_, cmd, strlen(cmd));
    if (n < 0)
    {
        std::cerr << "Write failed: " << strerror(errno) << '\n';
        return false;
    }

    if (UsbCom::waitForOK(3))
    {
        return true;
    }
    return false;
}

/**
 * Sends AT+CGPSINFO to retrieve gps location
 * Then returns information as a string in form "lat,log,alt"
 *
 * @return string "" if failed, or CPGS information.
 */
std::string UsbCom::GPSRead()
{
    if (fd_ < 0) // Check if fd_ is still open
        return "\0";

    const char *cmd = "AT+CGPSINFO\r"; // Set command msg AT
    ssize_t n = write(fd_, cmd, strlen(cmd));
    if (n < 0)
    {
        std::cerr << "Write failed: " << strerror(errno) << '\n';
        return "";
    }

    std::string GPSReceived;
    std::string latDD, latMM, logDD, logMM, sLat, sLog;
    float Lat, Log;
    char buffer[256];

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_, &readfds);

        timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ret = select(fd_ + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0)
        {
            std::cerr << "select failed: " << strerror(errno) << '\n';
            return "";
        }

        if (ret == 0)
        {
            break;
        }

        ssize_t n = read(fd_, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "Read failed: " << strerror(errno) << '\n';
            return "";
        }

        if (n > 0)
        {
            buffer[n] = '\0';
            GPSReceived += buffer;
            std::cout << buffer << std::flush;

            if (GPSReceived.find(",,,,,,,,") != std::string::npos)
            {
                std::cout << "Still Locating...\n";
                // return "";
                continue;
            }

            auto colonPos = GPSReceived.find(':');
            if (colonPos == std::string::npos)
            {
                std::cerr << "Invalid input\n";
                return "";
            }

            std::string data = GPSReceived.substr(colonPos + 2); // Use +1 and code below for stability
            // if (!data.empty() && data.front() == ' ')
            // {
            //     data.erase(0, 1);
            // }

            auto fields = split(data, ',');
            if (fields.size() < 4)
            {
                std::cerr << "Not enough fields\n";
                continue;
            }

            std::string lat = fields[0]; // Derives lattitude number
            std::string lon = fields[2]; // Derives longitude number
            std::string alt = fields[6]; // Derives Altitude

            latDD = lat.substr(0, 2);
            latMM = lat.substr(2);

            Lat = std::stoi(latDD) + (std::stof(latMM) / 60);

            if (fields[1] == "N")
                sLat = std::to_string(Lat);
            else if (fields[1] == "S")
                sLat = std::to_string(0 - Lat);

            logDD = lon.substr(0, 3);
            logMM = lon.substr(3);

            Log = std::stoi(logDD) + (std::stof(logMM) / 60);

            if (fields[3] == "E")
                sLog = std::to_string(Log);
            else if (fields[3] == "W")
                sLog = std::to_string(0 - Log);

            GPSReceived = sLat + ", " + sLog + ", " + alt; // Updated for altitude
            return GPSReceived;
        }
    }

    return "";
}

/**
 * Takes in a string and separates it into parts through the delimiter
 *
 * @param s string spaced with commas
 * @param delim delimiter (comma) for separating fields
 * @return the parts as split by the delimiter.
 */
std::vector<std::string> UsbCom::split(const std::string &s, char delim)
{
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        parts.push_back(item);
    }
    return parts;
}
