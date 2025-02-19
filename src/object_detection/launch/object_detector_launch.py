import launch
import launch_ros.actions

def generate_launch_description():
    return launch.LaunchDescription([
        launch_ros.actions.Node(
            package='object_detection',
            executable='object_detector_node',
            name='object_detector',
            output='screen',
            parameters=[{
                'image_left_topic': '/my_camera/image_left',
                'image_right_topic': '/my_camera/image_right',
            }]
        ),
    ])
