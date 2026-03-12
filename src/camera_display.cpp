#include "rviz_camera_stream/camera_display.hpp"

#include <Ogre.h>
#include <OgreTextureManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreViewport.h>

#include "rviz_common/display_context.hpp"
#include "rviz_common/view_manager.hpp"

// Macro to export the class as an RViz2 plugin
#include "pluginlib/class_list_macros.hpp"

namespace rviz_camera_stream
{

RenderTextureDisplay::RenderTextureDisplay()
{
  // Create properties that will appear in the RViz2 interface
  topic_property_ = new rviz_common::properties::StringProperty(
    "Topic", "/rviz_render", "The topic to publish the rendered image on.", this);

  width_property_ = new rviz_common::properties::IntProperty(
    "Width", 1280, "Width of the output image.", this);
  width_property_->setMin(1);

  height_property_ = new rviz_common::properties::IntProperty(
    "Height", 720, "Height of the output image.", this);
  height_property_->setMin(1);
}

RenderTextureDisplay::~RenderTextureDisplay()
{
  // Ensure OGRE resources are released
  destroyRenderTexture();
}

void RenderTextureDisplay::onInitialize()
{
  // Get the ROS node from the RViz2 context to create a publisher
  rviz_node_ = context_->getRosNodeAbstraction().lock()->get_raw_node();

  // Connect property change signals — done here rather than in the constructor
  // to ensure the QObject is fully initialized and MOC metadata is available.
  connect(topic_property_, SIGNAL(changed()), this, SLOT(onTopicChanged()));
  connect(width_property_, SIGNAL(changed()), this, SLOT(onResolutionChanged()));
  connect(height_property_, SIGNAL(changed()), this, SLOT(onResolutionChanged()));

  onTopicChanged();
  onResolutionChanged();
}

void RenderTextureDisplay::onEnable()
{
  // Setup the texture when the display is enabled
  setupRenderTexture();

  // Force initial camera synchronization
  syncCameraWithRviz();

  RCLCPP_INFO(rviz_node_->get_logger(), "rviz_camera_stream enabled");
}

void RenderTextureDisplay::onDisable()
{
  // Release resources when the display is disabled
  destroyRenderTexture();
}

void RenderTextureDisplay::update(float /*wall_dt*/, float /*ros_dt*/)
{
  // Synchronize with the main RViz camera
  syncCameraWithRviz();

  // Publish every frame (rate is governed by RViz2's update loop)
  if (isEnabled()) {
    publishImage();
  }
}

void RenderTextureDisplay::reset()
{
  Display::reset();
  destroyRenderTexture();
  setupRenderTexture();
}

void RenderTextureDisplay::onTopicChanged()
{
  // Create the publisher with the topic name defined in the UI
  publisher_ = rviz_node_->create_publisher<sensor_msgs::msg::Image>(
    topic_property_->getStdString(), 10);
}

void RenderTextureDisplay::onResolutionChanged()
{
    width_ = width_property_->getInt();
    height_ = height_property_->getInt();
    if (isEnabled()) {
        destroyRenderTexture();
        setupRenderTexture();
    }
}

void RenderTextureDisplay::setupRenderTexture()
{
  // Avoid duplicate setup
  if (render_target_) {
    return;
  }

  // Unique name for the texture
  std::string tex_name = "RttTex_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));

  // Create the texture using RGB format
  rtt_texture_ = Ogre::TextureManager::getSingleton().createManual(
    tex_name,
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    Ogre::TEX_TYPE_2D, width_, height_, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

  // Get the RenderTarget (the surface where we will draw) from the texture
  render_target_ = rtt_texture_->getBuffer()->getRenderTarget();
  render_target_->setAutoUpdated(false);
  render_target_->setActive(false);  // Initially inactive

  // Create a dedicated camera for the render texture
  createCamera();

  // Setup viewport with visibility mask
  setupViewport();

  RCLCPP_INFO(rviz_node_->get_logger(), "Render texture configured: %dx%d", width_, height_);
}

void RenderTextureDisplay::destroyRenderTexture()
{
  if (render_target_) {
    render_target_->removeAllViewports();
    render_target_ = nullptr;
  }

  // Destroy the camera created specifically for the render texture
  if (rtt_camera_ && context_->getSceneManager()) {
    context_->getSceneManager()->destroyCamera(rtt_camera_);
    rtt_camera_ = nullptr;
  }

  if (rtt_texture_)
  {
    Ogre::TextureManager::getSingleton().remove(rtt_texture_->getName());
    rtt_texture_.reset();
  }
}

void RenderTextureDisplay::syncCameraWithRviz()
{
  if (!isEnabled() || !render_target_ || !rtt_camera_) {
    return;
  }

  // Get the main RViz camera
  auto view_controller = context_->getViewManager()->getCurrent();
  if (!view_controller) {
    return;
  }

  Ogre::Camera* main_camera = view_controller->getCamera();
  if (!main_camera) {
    return;
  }

  // Synchronize position, orientation and camera settings
  rtt_camera_->setPosition(main_camera->getDerivedPosition());
  rtt_camera_->setOrientation(main_camera->getDerivedOrientation());
  rtt_camera_->setNearClipDistance(main_camera->getNearClipDistance());
  rtt_camera_->setFarClipDistance(main_camera->getFarClipDistance());
  rtt_camera_->setFOVy(main_camera->getFOVy());
  rtt_camera_->setAspectRatio(static_cast<float>(width_) / static_cast<float>(height_));
}

void RenderTextureDisplay::setupViewport()
{
  if (!render_target_ || !rtt_camera_) {
    return;
  }

  // Remove existing viewport if any
  render_target_->removeAllViewports();

  // Create viewport with the camera
  Ogre::Viewport* viewport = render_target_->addViewport(rtt_camera_);
  viewport->setBackgroundColour(Ogre::ColourValue(0.129f, 0.129f, 0.129f, 1.0f));
  viewport->setClearEveryFrame(true);
  viewport->setOverlaysEnabled(false);

  // CRITICAL: Configure visibility mask to see all objects
  auto visibility_bits = context_->visibilityBits();
  if (visibility_bits) {
    // Use mask that allows seeing all displays
    uint32_t vis_mask = 0xFFFFFFFF;  // See everything
    viewport->setVisibilityMask(vis_mask);
  }
}

void RenderTextureDisplay::publishImage()
{
  if (!isEnabled() || !render_target_ || !rtt_camera_) {
    return;
  }

  // Activate the render target before rendering
  render_target_->setActive(true);

  // Force scene update
  auto scene_manager = context_->getSceneManager();
  if (scene_manager) {
    scene_manager->_updateSceneGraph(nullptr);
  }

  // Update and render to our texture
  render_target_->update();

  // Deactivate the render target after rendering
  render_target_->setActive(false);

  // Get the texture data
  Ogre::HardwarePixelBufferSharedPtr pixel_buffer = rtt_texture_->getBuffer();
  pixel_buffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);

  const Ogre::PixelBox& pixel_box = pixel_buffer->getCurrentLock();
  size_t data_size = pixel_box.getConsecutiveSize();

  // Create the image message
  auto image_msg = std::make_unique<sensor_msgs::msg::Image>();
  image_msg->header.stamp = rviz_node_->get_clock()->now();
  image_msg->header.frame_id = context_->getFixedFrame().toStdString();
  image_msg->width = width_;
  image_msg->height = height_;
  image_msg->encoding = "bgr8";

  // For RGB8, 3 bytes per pixel
  image_msg->step = width_ * 3;
  image_msg->is_bigendian = false;

  // Copy the data
  image_msg->data.resize(data_size);
  memcpy(image_msg->data.data(), pixel_box.data, data_size);

  pixel_buffer->unlock();

  // Publish the message
  publisher_->publish(std::move(image_msg));

  RCLCPP_DEBUG(rviz_node_->get_logger(), "Image published: %dx%d, encoding: bgr8",
               width_, height_);
}

void RenderTextureDisplay::createCamera()
{
  if (rtt_camera_ || !context_->getSceneManager()) {
    return;
  }

  // Create a unique camera for this render texture
  std::string camera_name = "RttCamera_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
  rtt_camera_ = context_->getSceneManager()->createCamera(camera_name);

  // Basic camera settings
  rtt_camera_->setNearClipDistance(0.01f);
  rtt_camera_->setFarClipDistance(1000.0f);
  rtt_camera_->setAspectRatio(static_cast<float>(width_) / static_cast<float>(height_));
}


}  // namespace rviz_camera_stream

// Export the class so that pluginlib can find it
PLUGINLIB_EXPORT_CLASS(rviz_camera_stream::RenderTextureDisplay, rviz_common::Display)
