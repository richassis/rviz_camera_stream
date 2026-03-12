# rviz_camera_stream

A RViz2 plugin that renders the main visualization to a texture and publishes it as ROS Image messages.

This is the ROS 2 port of the original [rviz_camera_stream](https://github.com/lucasw/rviz_camera_stream) package.

## Description

This package contains a display plugin for RViz2 called `rviz_camera_stream` that captures what is being visualized in the main RViz2 window and publishes it as `sensor_msgs/Image` messages on a ROS topic.

## Features

- Renders RViz2 3D scene to an off-screen texture
- Publishes the rendered image as a ROS message (bgr8 encoding)
- Customizable resolution configuration
- Automatic synchronization with RViz2 main camera

## Build

```bash
cd /path/to/your/workspace
colcon build --packages-select rviz_camera_stream
source install/setup.bash
```

## Usage

1. Open RViz2
2. Click "Add" to add a new display
3. Select "rviz_camera_stream/CameraPub" from the list of available displays
4. Configure the properties:
   - **Topic**: Topic name where the image will be published (default: `/rviz_render`)
   - **Width**: Image width in pixels (default: 1280)
   - **Height**: Image height in pixels (default: 720)

## Image Visualization

To visualize the published images, you can use:

```bash
ros2 run rqt_image_view rqt_image_view /rviz_render
```

Or add an "Image" display in RViz2 itself configured for the `/rviz_render` topic.

## Dependencies

- ROS 2
- RViz2
- sensor_msgs
- rclcpp
- pluginlib
- rviz_common
- rviz_rendering
- rviz_ogre_vendor

## License

BSD
