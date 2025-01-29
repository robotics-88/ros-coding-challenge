# ros-coding-challenge
Steps for the Robotics 88 coding challenge 🔥

1. Clone this repo locally and make a branch named `test/your-name`
2. Make a ROS2 package in C++ on this branch such that
    1. It subscribes to 2 synced image topics, e.g. `my_camera/image_left` and  `my_camera/image_right`
    2. In the image callback, it identifies at least one corresponding object in each pair of frames using machine learning
    3. It prints the classified type of the detected object
3. Add a launch file to make the image topic configurable
4. Create a PR here for your branch
