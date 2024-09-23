# ros-coding-challenge
Steps for the Robotics 88 coding challenge 🔥

1. Clone this repo locally and make a branch named `test/your-name`
2. Make a ROS package in C++ on this branch such that
    1. It subscribes to an image topic `my_camera/image`
    2. In the image callback, it converts the image from RGB to grayscale
    3. It performs contour analysis on the image
    4. It republishes the grayscale image with contours overlaid
3. Add a launch file to make the image topic configurable
4. Create a PR here for your branch
