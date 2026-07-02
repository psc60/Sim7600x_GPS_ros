#include "usbCom.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
// #include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class MinimalPublisher : public rclcpp::Node
{
public:
    MinimalPublisher()
    : Node("minimal_publisher"), count_(0)
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
        auto timer_callback =
        [this]() -> void {
            std::string usb.GPSAnswer = usb.GPSRead();

			RCLCPP_INFO_STREAM(this->get_logger(), "Publishing: '" << navMsg.header << "'");
			publisher_->publish(navMsg);        
        };
        timer_ = this->create_wall_timer(500ms, timer_callback);
    }

private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    size_t count_;
};

int main(int argc, char * argv[])
{
    UsbCom usb("/dev/ttyUSB2", 115200);
    if (!usb.openPort()) return 1;
    if (!usb.sendAT()) return 1;
    
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MinimalPublisher>());
    rclcpp::shutdown();
    return 0;

    // std::string GPSInfo = usb.GPSRead();
    // auto fields = usb.split(GPSInfo, ',');

    return 0;
}