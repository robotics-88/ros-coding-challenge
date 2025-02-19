#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class ImagePublisher : public rclcpp::Node
{
public:
    ImagePublisher() : Node("image_publisher")
    {
        // declare and get parameters
        this->declare_parameter<std::string>("left_image_path", "img.jpg");
        this->declare_parameter<std::string>("right_image_path", "img.jpg");

        std::string left_image_path = this->get_parameter("left_image_path").as_string();
        std::string right_image_path = this->get_parameter("right_image_path").as_string();

        left_image_ = cv::imread(left_image_path);
        right_image_ = cv::imread(right_image_path);

        if (left_image_.empty() || right_image_.empty())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to load image/images");
            rclcpp::shutdown();
        }

        left_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("/my_camera/image_left", 10);
        right_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("/my_camera/image_right", 10);

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(5000),
            std::bind(&ImagePublisher::publish_images, this));
    }

private:
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr left_publisher_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr right_publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    cv::Mat left_image_;
    cv::Mat right_image_;

    void publish_images()
    {
        auto left_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", left_image_).toImageMsg();
        auto right_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", right_image_).toImageMsg();

        left_msg->header.stamp = this->now();
        right_msg->header.stamp = this->now();

        left_publisher_->publish(*left_msg);
        right_publisher_->publish(*right_msg);

        RCLCPP_INFO(this->get_logger(), "Published left and right images");
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImagePublisher>());
    rclcpp::shutdown();
    return 0;
}
