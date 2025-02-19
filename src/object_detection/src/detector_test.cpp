#include <rclcpp/rclcpp.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <fstream>
#include <vector>

class ObjectDetector : public rclcpp::Node
{
public:
    ObjectDetector() : Node("detector_test_node")
    {
        this->declare_parameter<std::string>("image_path", "img.jpg");
        this->declare_parameter<bool>("use_cuda", false);

        std::string image_path = this->get_parameter("image_path").as_string();
        use_cuda_ = this->get_parameter("use_cuda").as_bool();

        load_class_list();
        load_net();

        // Load and process image
        process_image(image_path);
    }

private:
    cv::dnn::Net net_;
    std::vector<std::string> class_list_;
    bool use_cuda_;

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

    void process_image(const std::string &image_path)
    {
        cv::Mat image = cv::imread(image_path);
        if (image.empty())
        {
            RCLCPP_ERROR(this->get_logger(), "Could not load image: %s", image_path.c_str());
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Running YOLOv5 inference on image: %s", image_path.c_str());

        std::vector<int> class_ids;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;

        detect_objects(image, class_ids, confidences, boxes);

        // Draw detections
        for (size_t i = 0; i < class_ids.size(); ++i)
        {
            cv::rectangle(image, boxes[i], cv::Scalar(0, 255, 0), 2);
            std::string label = class_list_[class_ids[i]];
            cv::putText(image, label, cv::Point(boxes[i].x, boxes[i].y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
            RCLCPP_INFO(this->get_logger(), "Detected: %s", label.c_str());
        }

        // Show result
        cv::imshow("YOLOv5 Detection", image);
        cv::waitKey(0);
    }

    void detect_objects(cv::Mat &image, std::vector<int> &class_ids, std::vector<float> &confidences, std::vector<cv::Rect> &boxes)
    {
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
                    confidences.push_back(confidence);
                    class_ids.push_back(class_id.x);

                    int left = int((data[0] - 0.5 * data[2]) * image.cols / 640);
                    int top = int((data[1] - 0.5 * data[3]) * image.rows / 640);
                    int width = int(data[2] * image.cols / 640);
                    int height = int(data[3] * image.rows / 640);

                    boxes.push_back(cv::Rect(left, top, width, height));
                }
            }
            data += dimensions;
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
