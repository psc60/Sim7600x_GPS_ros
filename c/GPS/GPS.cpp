/**
 *  @filename   :   GPS.cpp
 *  @brief      :   Implements for sim7600c 4g hat raspberry pi demo
 *  @author     :   Kaloha from Waveshare
 *
 *  Copyright (C) Waveshare     April 27 2018
 *  http://www.waveshare.com / http://www.waveshare.net
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "../arduPi.h"
#include "../sim7x00.h"

#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <bits/stdc++.h>

using namespace std::chrono_literals;

// Pin definition
int POWERKEY = 6;

int8_t answer;

/**************************GPS Publisher Class**************************/

class GPSPublisher : public rclcpp::Node
{
	public:
	GPSPublisher()
		: Node("gps_publisher"), count_(0)
	{
		// printf("Presetup\n");
		sim7600.PowerOnWithoutSIM(POWERKEY);
		// printf("Power hecho\n");
		sim7600.GPSInitSession();
		// sim7600.GPSPositioning();
		// printf("Postsetup\n");
		
		publisher_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("/NavSatFix", rclcpp::SensorDataQoS()); // sensor_msgs/NavSatFix

	}

	void GPSTeleop() {
		std::string sresult = sim7600.GPSReadWithOpenedSession();
		if (sresult == "")
			RCLCPP_INFO_STREAM(this->get_logger(), "No GPS reading");
		else
		{
			int first_dot_index = sresult.find(",");
			int second_dot_index = sresult.find(",", first_dot_index + 1);

			float Lat = std::stof(sresult.substr(0, first_dot_index));
			float Log = std::stof(sresult.substr(first_dot_index+1, second_dot_index-first_dot_index-1));
			float Altitude = std::stof(sresult.substr(second_dot_index+1)); // meters
			

			auto navMsg = NavSatFix(
				header(),
				status(),
				latitude(Lat),
				longitude(Log),
				altitude(Altitude),
				position_covariance(),
				position_covariance_type(0) // UNKNOWN
			);								   // CHANGE

			RCLCPP_INFO_STREAM(this->get_logger(), "Publishing: '" << navMsg.header << "'");
			publisher_->publish(navMsg);
		}

	}
};

/**************************Main Fucntion**************************/

int main(int argc, char * argv[]){

	// pthread_setname_np(pthread_self(), "main_thread");
	rclcpp::init(argc, argv);
	auto gpsNode = std::make_shared<ArduCamNode>();
	rclcpp::spin(gpsNode);
	rclcpp::shutdown();
	// sim7600.GPSCloseSession();
	return (0);

}
