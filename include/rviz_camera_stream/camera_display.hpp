#ifndef RVIZ_CAMERA_STREAM__CAMERA_DISPLAY_HPP_
#define RVIZ_CAMERA_STREAM__CAMERA_DISPLAY_HPP_

// RViz2 headers
#include "rviz_common/display.hpp"
#include "rviz_common/properties/int_property.hpp"
#include "rviz_common/properties/string_property.hpp"

// ROS 2 headers
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

// OGRE headers
#include <OgreTexture.h>
#include <OgreRenderTexture.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCamera.h>

namespace rviz_camera_stream
{

class RenderTextureDisplay : public rviz_common::Display
{
  Q_OBJECT

public:
  RenderTextureDisplay();
  ~RenderTextureDisplay() override;

  // Overridden methods from base Display class
  void onInitialize() override;
  void onEnable() override;
  void onDisable() override;
  void update(float wall_dt, float ros_dt) override;
  void reset() override;

private Q_SLOTS:
  // Slots for reacting to property changes in the RViz2 UI
  void onTopicChanged();
  void onResolutionChanged();

private:
  void setupRenderTexture();
  void destroyRenderTexture();
  void publishImage();
  void syncCameraWithRviz();
  void createCamera();
  void setupViewport();

  // Properties that will appear in the RViz2 panel
  rviz_common::properties::StringProperty * topic_property_;
  rviz_common::properties::IntProperty * width_property_;
  rviz_common::properties::IntProperty * height_property_;

  // OGRE members
  Ogre::TexturePtr rtt_texture_;
  Ogre::RenderTexture * render_target_{nullptr};
  Ogre::Camera * rtt_camera_{nullptr};

  // ROS 2 members
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
  rclcpp::Node::SharedPtr rviz_node_;

  // Texture dimensions
  int width_{1280};
  int height_{720};
};

}  // namespace rviz_camera_stream

#endif  // RVIZ_CAMERA_STREAM__CAMERA_DISPLAY_HPP_
