#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <fstream>
#include <vector>
#include <set>

class ObjectDetector : public rclcpp::Node
{
public:
    ObjectDetector() : Node("object_detector")
    {
        this->declare_parameter<std::string>("image_left_topic", "/my_camera/image_left");
        this->declare_parameter<std::string>("image_right_topic", "/my_camera/image_right");
        this->declare_parameter<bool>("use_cuda", false);

        std::string image_left_topic = this->get_parameter("image_left_topic").as_string();
        std::string image_right_topic = this->get_parameter("image_right_topic").as_string();
        use_cuda_ = this->get_parameter("use_cuda").as_bool();

        image_left_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_left_topic, 10, std::bind(&ObjectDetector::image_callback_left, this, std::placeholders::_1));

        image_right_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_right_topic, 10, std::bind(&ObjectDetector::image_callback_right, this, std::placeholders::_1));

        load_class_list();
        load_net();
    }

private:
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_left_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_right_sub_;
    cv::dnn::Net net_;
    std::vector<std::string> class_list_;
    bool use_cuda_;
    cv::Mat left_image_, right_image_;
    std::set<int> left_detections_, right_detections_; // Stores class IDs of detected objects

    void load_class_list()
    {
        std::ifstream ifs("Models/classes.txt");
        std::string line;
        while (getline(ifs, line))
        {
            class_list_.push_back(line);
        }
    }

    void load_net()
    {
        net_ = cv::dnn::readNet("Models/yolov5s.onnx");
        if (use_cuda_)
        {
            RCLCPP_INFO(this->get_logger(), "Using CUDA");
            net_.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            net_.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "Using CPU");
            net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
    }

    void detect_objects(cv::Mat &image, std::set<int> &detections)
    {
        detections.clear();
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);
        net_.setInput(blob);
        std::vector<cv::Mat> outputs;
        net_.forward(outputs, net_.getUnconnectedOutLayersNames());

        float *data = (float *)outputs[0].data;
        const int rows = 25200;
        const int dimensions = 85;

        for (int i = 0; i < rows; ++i)
        {
            float confidence = data[4];
            if (confidence > 0.4)
            {
                float *classes_scores = data + 5;
                cv::Mat scores(1, class_list_.size(), CV_32FC1, classes_scores);
                cv::Point class_id;
                double max_class_score;
                minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
                if (max_class_score > 0.2)
                {
                    detections.insert(class_id.x);
                }
            }
            data += dimensions;
        }
    }

    void check_and_print_common_objects()
    {
        // Find common objects
        std::set<int> common_detections;
        for (const auto &class_id : left_detections_)
        {
            if (right_detections_.find(class_id) != right_detections_.end())
            {
                common_detections.insert(class_id);
            }
        }

        if (!common_detections.empty())
        {
            for (const auto &class_id : common_detections)
            {
                RCLCPP_INFO(this->get_logger(), "Common Object Detected: %s", class_list_[class_id].c_str());
            }
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "No common objects detected");
        }
        RCLCPP_INFO(this->get_logger(), "-----------------------------");

    }

    void image_callback_left(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        left_image_ = cv_bridge::toCvCopy(msg, "bgr8")->image;
        detect_objects(left_image_, left_detections_);
        if (!right_image_.empty()) // Check if right image available
        {
            check_and_print_common_objects();
        }
    }

    void image_callback_right(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        right_image_ = cv_bridge::toCvCopy(msg, "bgr8")->image;
        detect_objects(right_image_, right_detections_);
        if (!left_image_.empty()) // Check if left image available
        {
            check_and_print_common_objects();
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ObjectDetector>());
    rclcpp::shutdown();
    return 0;
}