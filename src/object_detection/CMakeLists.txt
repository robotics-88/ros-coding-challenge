cmake_minimum_required(VERSION 3.5)
project(object_detection)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV REQUIRED)

# object detection node
add_executable(object_detector_node src/object_detection_node.cpp)
ament_target_dependencies(object_detector_node rclcpp sensor_msgs cv_bridge image_transport OpenCV)

# test node (reads image from local)
add_executable(detector_test src/detector_test.cpp)
ament_target_dependencies(detector_test rclcpp OpenCV)

# dummy image publisher node for testing
add_executable(image_publisher src/dummy_publisher.cpp)
ament_target_dependencies(image_publisher rclcpp sensor_msgs cv_bridge image_transport OpenCV)

install(TARGETS
  object_detector_node
  detector_test
  image_publisher
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
