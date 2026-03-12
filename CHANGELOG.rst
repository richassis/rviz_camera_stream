^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rviz_camera_stream
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.0.0 (2026-03-12)
-------------------
* Ported to ROS 2 (ament_cmake, rclcpp, rviz_common, pluginlib)
* Replaced catkin build system with ament_cmake
* Replaced rviz (ROS1) with rviz_common/rviz_rendering (ROS2)
* Added configurable image encoding property (rgb8, rgba8, bgr8, bgra8, mono8)
* Added configurable resolution and publication rate properties
* Automatic synchronization with the main RViz2 camera

Forthcoming
-----------
* Added github action
* Cleanup according to roslint and catkin_lint
* Modifications by Lucas Walter
* initial: adds a copy of rviz Camera plugin
* Contributors: Florent Lamiraux, John Wason, Julian, Lucas Walter, Mikhail Medvedev, Nikolas Engelhard, Thibaud Lasguignes
