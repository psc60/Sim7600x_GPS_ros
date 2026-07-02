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

class UsbCom
{
    public:
        static std::vector<std::string> split(const std::string &s, char delim)
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

        std::string GPSRead()
        {
            std::string GPSReceived;
            std::string latDD, latMM, logDD, logMM, sLat, sLog;
            float Lat, Log;
            char buffer[256];

            GPSReceived = "+CGPSINFO: 5122.772674,N,00219.495173,W,160426,122604.0,180.1,0.0,";
            std::cout << GPSReceived << "\n" << std::flush;

            if (GPSReceived.find(",,,,,,,,") != std::string::npos)
            {
                std::cout << "\nStill Locating...\n";
                return "";
            }

            auto colonPos = GPSReceived.find(':');
            if (colonPos == std::string::npos)
            {
                std::cerr << "Invalid input\n";
                return "";
            }

            std::string data = GPSReceived.substr(colonPos + 1);
            if (!data.empty() && data.front() == ' ')
            {
                data.erase(0, 1);
            }

            auto fields = split(data, ',');
            if (fields.size() < 4)
            {
                std::cerr << "Not enough fields\n";
                return "";
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

            GPSReceived = sLat + "," + sLog + "," + alt; // Updated for altitude
            std::cout << GPSReceived << "\n";
            return GPSReceived;
        }
};

int main()
{
    auto testV = UsbCom();
    testV.GPSRead();
    return 0;
}