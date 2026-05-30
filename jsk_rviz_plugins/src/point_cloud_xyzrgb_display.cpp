// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2026, Howard Cheng
 *  All rights reserved.
 *********************************************************************/

#include "point_cloud_xyzrgb_display.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

#include <OGRE/OgreSceneNode.h>
#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/frame_manager_iface.hpp>
#include <sensor_msgs/image_encodings.hpp>

namespace jsk_rviz_plugins
{

namespace
{
constexpr const char* kImageType = "sensor_msgs/msg/Image";
constexpr const char* kCameraInfoType = "sensor_msgs/msg/CameraInfo";
constexpr const char* kDefaultRgbImage = "/camera/color/image_raw";
constexpr const char* kDefaultRgbInfo = "/camera/color/camera_info";
constexpr const char* kDefaultDepthImage = "/camera/depth/image_raw";
constexpr const char* kDefaultDepthInfo = "/camera/depth/camera_info";

bool hasEncoding(const sensor_msgs::msg::Image& image, const std::string& encoding)
{
  return image.encoding == encoding;
}
}  // namespace

PointCloudXyzrgbTopicProperty::PointCloudXyzrgbTopicProperty(
  const QString& name,
  const QString& default_value,
  const QString& description,
  const std::string& type_name,
  rviz_common::properties::Property* parent,
  const char* changed_slot)
  : rviz_common::properties::EditableEnumProperty(
      name, default_value, description, parent, changed_slot),
    type_name_(type_name)
{
  QObject::connect(
    this,
    &rviz_common::properties::EditableEnumProperty::requestOptions,
    this,
    [this](rviz_common::properties::EditableEnumProperty*) {
      fillTopicList();
    });
}

void PointCloudXyzrgbTopicProperty::initialize(
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr rviz_ros_node)
{
  rviz_ros_node_ = rviz_ros_node;
}

std::string PointCloudXyzrgbTopicProperty::getTopicStd() const
{
  return getStdString();
}

void PointCloudXyzrgbTopicProperty::fillTopicList()
{
  clearOptions();
  auto node_interface = rviz_ros_node_.lock();
  if (!node_interface) {
    return;
  }

  const auto node = node_interface->get_raw_node();
  const auto topic_names_and_types = node->get_topic_names_and_types();
  std::set<std::string> topics;
  for (const auto& topic_and_types : topic_names_and_types) {
    if (std::find(
          topic_and_types.second.begin(), topic_and_types.second.end(), type_name_) !=
        topic_and_types.second.end()) {
      topics.insert(topic_and_types.first);
    }
  }

  for (const auto& topic : topics) {
    addOptionStd(topic);
  }
}

PointCloudXyzrgbDisplay::PointCloudXyzrgbDisplay()
  : rviz_common::Display(),
    rgb_image_property_(nullptr),
    rgb_info_property_(nullptr),
    depth_image_property_(nullptr),
    depth_info_property_(nullptr),
    stride_property_(nullptr),
    max_points_property_(nullptr),
    max_render_fps_property_(nullptr),
    point_size_property_(nullptr),
    new_data_(false),
    render_elapsed_(0.0f)
{
  rgb_image_property_ = new PointCloudXyzrgbTopicProperty(
    "Color Image", kDefaultRgbImage, "RGB image topic.",
    kImageType, this, SLOT(updateConfiguration()));
  rgb_info_property_ = new PointCloudXyzrgbTopicProperty(
    "Color Camera Info", kDefaultRgbInfo, "CameraInfo topic for the RGB image.",
    kCameraInfoType, this, SLOT(updateConfiguration()));
  depth_image_property_ = new PointCloudXyzrgbTopicProperty(
    "Depth Image", kDefaultDepthImage, "Depth image topic.",
    kImageType, this, SLOT(updateConfiguration()));
  depth_info_property_ = new PointCloudXyzrgbTopicProperty(
    "Depth Camera Info", kDefaultDepthInfo, "CameraInfo topic for the registered depth image.",
    kCameraInfoType, this, SLOT(updateConfiguration()));
  stride_property_ = new rviz_common::properties::IntProperty(
    "Stride", 2, "Use every Nth depth pixel to reduce rendering cost.",
    this, SLOT(updateConfiguration()));
  stride_property_->setMin(1);
  max_points_property_ = new rviz_common::properties::IntProperty(
    "Max Points", 100000, "Maximum points rendered per frame. 0 means unlimited.",
    this, SLOT(updateConfiguration()));
  max_points_property_->setMin(0);
  max_render_fps_property_ = new rviz_common::properties::FloatProperty(
    "Max Render FPS", 15.0, "Limit point cloud rebuild rate. 0 means unlimited.",
    this, SLOT(updateConfiguration()));
  max_render_fps_property_->setMin(0.0);
  point_size_property_ = new rviz_common::properties::FloatProperty(
    "Point Size", 0.01, "Rendered point size in meters.",
    this, SLOT(updateRenderProperties()));
  point_size_property_->setMin(0.001);
}

PointCloudXyzrgbDisplay::~PointCloudXyzrgbDisplay()
{
  unsubscribe();
  if (point_cloud_ && scene_node_) {
    scene_node_->detachObject(point_cloud_.get());
  }
}

void PointCloudXyzrgbDisplay::onInitialize()
{
  rviz_common::Display::onInitialize();
  const auto rviz_ros_node = context_->getRosNodeAbstraction();
  rgb_image_property_->initialize(rviz_ros_node);
  rgb_info_property_->initialize(rviz_ros_node);
  depth_image_property_->initialize(rviz_ros_node);
  depth_info_property_->initialize(rviz_ros_node);
  ensureRosNode();

  point_cloud_ = std::make_shared<rviz_rendering::PointCloud>();
  scene_node_->attachObject(point_cloud_.get());
  applyRenderProperties();
}

void PointCloudXyzrgbDisplay::onEnable()
{
  if (point_cloud_) {
    point_cloud_->setVisible(true);
  }
  subscribe();
}

void PointCloudXyzrgbDisplay::onDisable()
{
  unsubscribe();
  if (point_cloud_) {
    point_cloud_->setVisible(false);
  }
}

void PointCloudXyzrgbDisplay::reset()
{
  rviz_common::Display::reset();
  std::lock_guard<std::mutex> lock(mutex_);
  latest_rgb_.reset();
  latest_rgb_info_.reset();
  latest_depth_.reset();
  latest_depth_info_.reset();
  new_data_ = false;
  if (point_cloud_) {
    point_cloud_->clearAndRemoveAllPoints();
  }
}

void PointCloudXyzrgbDisplay::update(float wall_dt, float)
{
  render_elapsed_ += wall_dt;
  const float max_fps = max_render_fps_property_->getFloat();
  if (max_fps > 0.0f && render_elapsed_ < 1.0f / max_fps) {
    return;
  }
  render_elapsed_ = 0.0f;
  processLatestMessages();
}

bool PointCloudXyzrgbDisplay::ensureRosNode()
{
  if (node_) {
    return true;
  }

  auto abstraction = context_->getRosNodeAbstraction().lock();
  if (!abstraction) {
    setDisplayStatus(rviz_common::properties::StatusProperty::Error, "RViz ROS node is not available");
    return false;
  }

  node_ = abstraction->get_raw_node();
  return static_cast<bool>(node_);
}

void PointCloudXyzrgbDisplay::updateConfiguration()
{
  if (isEnabled()) {
    subscribe();
  }
}

void PointCloudXyzrgbDisplay::updateRenderProperties()
{
  applyRenderProperties();
}

void PointCloudXyzrgbDisplay::subscribe()
{
  if (!ensureRosNode() || !isEnabled()) {
    return;
  }

  unsubscribe();
  const auto qos = rclcpp::SensorDataQoS();
  rgb_sub_ = node_->create_subscription<Image>(
    rgb_image_property_->getTopicStd(), qos,
    [this](Image::ConstSharedPtr msg) {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_rgb_ = msg;
      new_data_ = true;
    });
  rgb_info_sub_ = node_->create_subscription<CameraInfo>(
    rgb_info_property_->getTopicStd(), qos,
    [this](CameraInfo::ConstSharedPtr msg) {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_rgb_info_ = msg;
    });
  depth_sub_ = node_->create_subscription<Image>(
    depth_image_property_->getTopicStd(), qos,
    [this](Image::ConstSharedPtr msg) {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_depth_ = msg;
      new_data_ = true;
    });
  depth_info_sub_ = node_->create_subscription<CameraInfo>(
    depth_info_property_->getTopicStd(), qos,
    [this](CameraInfo::ConstSharedPtr msg) {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_depth_info_ = msg;
      new_data_ = true;
    });
}

void PointCloudXyzrgbDisplay::unsubscribe()
{
  rgb_sub_.reset();
  rgb_info_sub_.reset();
  depth_sub_.reset();
  depth_info_sub_.reset();
}

void PointCloudXyzrgbDisplay::processLatestMessages()
{
  Image::ConstSharedPtr rgb;
  Image::ConstSharedPtr depth;
  CameraInfo::ConstSharedPtr depth_info;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!new_data_ || !latest_rgb_ || !latest_depth_ || !latest_depth_info_) {
      return;
    }
    rgb = latest_rgb_;
    depth = latest_depth_;
    depth_info = latest_depth_info_;
    new_data_ = false;
  }

  Ogre::Vector3 position;
  Ogre::Quaternion orientation;
  if (!context_->getFrameManager()->getTransform(depth->header, position, orientation)) {
    setDisplayStatus(
      rviz_common::properties::StatusProperty::Error,
      QString("No transform from %1").arg(QString::fromStdString(depth->header.frame_id)));
    return;
  }

  std::vector<rviz_rendering::PointCloud::Point> points;
  std::string error;
  if (!makePointCloud(rgb, depth, depth_info, points, error)) {
    setDisplayStatus(
      rviz_common::properties::StatusProperty::Error, QString::fromStdString(error));
    return;
  }

  scene_node_->setPosition(position);
  scene_node_->setOrientation(orientation);
  point_cloud_->clearAndRemoveAllPoints();
  if (!points.empty()) {
    point_cloud_->addPoints(points.begin(), points.end());
  }
  setDisplayStatus(
    rviz_common::properties::StatusProperty::Ok,
    QString("%1 points").arg(static_cast<int>(points.size())));
  context_->queueRender();
}

bool PointCloudXyzrgbDisplay::makePointCloud(
  const Image::ConstSharedPtr& rgb,
  const Image::ConstSharedPtr& depth,
  const CameraInfo::ConstSharedPtr& depth_info,
  std::vector<rviz_rendering::PointCloud::Point>& points,
  std::string& error) const
{
  if (depth_info->k[0] == 0.0 || depth_info->k[4] == 0.0) {
    error = "Depth camera info has invalid focal length";
    return false;
  }
  if (depth->width == 0 || depth->height == 0 || rgb->width == 0 || rgb->height == 0) {
    error = "Image width or height is zero";
    return false;
  }

  const uint32_t stride = static_cast<uint32_t>(std::max(1, stride_property_->getInt()));
  const size_t max_points = static_cast<size_t>(std::max(0, max_points_property_->getInt()));
  const double fx = depth_info->k[0];
  const double fy = depth_info->k[4];
  const double cx = depth_info->k[2];
  const double cy = depth_info->k[5];

  const size_t estimated =
    static_cast<size_t>((depth->width + stride - 1) / stride) *
    static_cast<size_t>((depth->height + stride - 1) / stride);
  points.clear();
  points.reserve(max_points == 0 ? estimated : std::min(estimated, max_points));

  for (uint32_t v = 0; v < depth->height; v += stride) {
    for (uint32_t u = 0; u < depth->width; u += stride) {
      if (max_points > 0 && points.size() >= max_points) {
        return true;
      }

      float z = 0.0f;
      if (!readDepth(*depth, u, v, z) || z <= 0.0f || !std::isfinite(z)) {
        continue;
      }

      const uint32_t rgb_u = std::min(
        rgb->width - 1, static_cast<uint32_t>(
          static_cast<uint64_t>(u) * static_cast<uint64_t>(rgb->width) / depth->width));
      const uint32_t rgb_v = std::min(
        rgb->height - 1, static_cast<uint32_t>(
          static_cast<uint64_t>(v) * static_cast<uint64_t>(rgb->height) / depth->height));

      float r = 1.0f;
      float g = 1.0f;
      float b = 1.0f;
      readColor(*rgb, rgb_u, rgb_v, r, g, b);

      rviz_rendering::PointCloud::Point point;
      point.position.x = static_cast<float>((static_cast<double>(u) - cx) * z / fx);
      point.position.y = static_cast<float>((static_cast<double>(v) - cy) * z / fy);
      point.position.z = z;
      point.setColor(r, g, b, 1.0f);
      points.push_back(point);
    }
  }

  return true;
}

bool PointCloudXyzrgbDisplay::readDepth(
  const Image& depth, uint32_t x, uint32_t y, float& z) const
{
  if (x >= depth.width || y >= depth.height) {
    return false;
  }
  const size_t offset = static_cast<size_t>(y) * depth.step +
    static_cast<size_t>(x) * (depth.encoding == sensor_msgs::image_encodings::TYPE_16UC1 ? 2 : 4);

  if (depth.encoding == sensor_msgs::image_encodings::TYPE_16UC1 ||
      depth.encoding == sensor_msgs::image_encodings::MONO16) {
    if (offset + sizeof(uint16_t) > depth.data.size()) {
      return false;
    }
    uint16_t raw = 0;
    std::memcpy(&raw, &depth.data[offset], sizeof(uint16_t));
    z = static_cast<float>(raw) * 0.001f;
    return raw != 0;
  }

  if (depth.encoding == sensor_msgs::image_encodings::TYPE_32FC1) {
    if (offset + sizeof(float) > depth.data.size()) {
      return false;
    }
    std::memcpy(&z, &depth.data[offset], sizeof(float));
    return std::isfinite(z);
  }

  return false;
}

bool PointCloudXyzrgbDisplay::readColor(
  const Image& rgb, uint32_t x, uint32_t y, float& r, float& g, float& b) const
{
  if (x >= rgb.width || y >= rgb.height) {
    return false;
  }

  const size_t offset = static_cast<size_t>(y) * rgb.step + static_cast<size_t>(x) * rgb.step / rgb.width;
  if (offset >= rgb.data.size()) {
    return false;
  }

  if (hasEncoding(rgb, sensor_msgs::image_encodings::RGB8)) {
    if (offset + 2 >= rgb.data.size()) {
      return false;
    }
    r = rgb.data[offset] / 255.0f;
    g = rgb.data[offset + 1] / 255.0f;
    b = rgb.data[offset + 2] / 255.0f;
    return true;
  }
  if (hasEncoding(rgb, sensor_msgs::image_encodings::BGR8)) {
    if (offset + 2 >= rgb.data.size()) {
      return false;
    }
    b = rgb.data[offset] / 255.0f;
    g = rgb.data[offset + 1] / 255.0f;
    r = rgb.data[offset + 2] / 255.0f;
    return true;
  }
  if (hasEncoding(rgb, sensor_msgs::image_encodings::RGBA8)) {
    if (offset + 3 >= rgb.data.size()) {
      return false;
    }
    r = rgb.data[offset] / 255.0f;
    g = rgb.data[offset + 1] / 255.0f;
    b = rgb.data[offset + 2] / 255.0f;
    return true;
  }
  if (hasEncoding(rgb, sensor_msgs::image_encodings::BGRA8)) {
    if (offset + 3 >= rgb.data.size()) {
      return false;
    }
    b = rgb.data[offset] / 255.0f;
    g = rgb.data[offset + 1] / 255.0f;
    r = rgb.data[offset + 2] / 255.0f;
    return true;
  }
  if (hasEncoding(rgb, sensor_msgs::image_encodings::MONO8)) {
    const float gray = rgb.data[offset] / 255.0f;
    r = gray;
    g = gray;
    b = gray;
    return true;
  }

  return false;
}

void PointCloudXyzrgbDisplay::applyRenderProperties()
{
  if (!point_cloud_) {
    return;
  }
  point_cloud_->setRenderMode(rviz_rendering::PointCloud::RM_FLAT_SQUARES);
  const float size = point_size_property_->getFloat();
  point_cloud_->setDimensions(size, size, size);
  point_cloud_->setAlpha(1.0f);
}

void PointCloudXyzrgbDisplay::setDisplayStatus(
  rviz_common::properties::StatusProperty::Level level, const QString& text)
{
  setStatus(level, "PointCloudXyzrgb", text);
}

}  // namespace jsk_rviz_plugins

PLUGINLIB_EXPORT_CLASS(jsk_rviz_plugins::PointCloudXyzrgbDisplay, rviz_common::Display)
