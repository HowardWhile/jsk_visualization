// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2026, Howard Cheng
 *  All rights reserved.
 *********************************************************************/

#include "point_cloud_xyzrgb_display.h"

#include <algorithm>
#include <set>

#include <pluginlib/class_list_macros.hpp>
#include <rcl_interfaces/msg/parameter.hpp>
#include <rviz_common/properties/property.hpp>

namespace jsk_rviz_plugins
{

namespace
{
constexpr const char* kImageType = "sensor_msgs/msg/Image";
constexpr const char* kCameraInfoType = "sensor_msgs/msg/CameraInfo";
constexpr const char* kPointCloud2Type = "sensor_msgs/msg/PointCloud2";
constexpr const char* kDefaultRgbImage = "/camera/color/image_raw";
constexpr const char* kDefaultRgbInfo = "/camera/color/camera_info";
constexpr const char* kDefaultDepthImage = "/camera/depth/image_raw";
constexpr const char* kDefaultOutput = "/camera/color/points_xyzrgb";
constexpr const char* kDefaultLoadService = "/camera_processor_container/_container/load_node";

rcl_interfaces::msg::Parameter makeParameter(
  const std::string& name, const rclcpp::ParameterValue& value)
{
  return rclcpp::Parameter(name, value).to_parameter_msg();
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
  : rviz_default_plugins::displays::PointCloud2Display(),
    rgb_image_property_(nullptr),
    rgb_info_property_(nullptr),
    depth_image_property_(nullptr),
    depth_info_property_(nullptr),
    output_topic_property_(nullptr),
    container_service_property_(nullptr),
    auto_load_property_(nullptr),
    exact_sync_property_(nullptr),
    queue_size_property_(nullptr),
    loaded_node_id_(0),
    load_requested_(false),
    unload_requested_(false),
    load_after_unload_(false),
    retry_elapsed_(0.0f)
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
    "Depth Camera Info", kDefaultRgbInfo, "CameraInfo topic for the registered depth image.",
    kCameraInfoType, this, SLOT(updateConfiguration()));
  output_topic_property_ = new rviz_common::properties::StringProperty(
    "Point Cloud Output", kDefaultOutput, "Generated sensor_msgs/msg/PointCloud2 topic.",
    this, SLOT(updateConfiguration()));
  container_service_property_ = new rviz_common::properties::StringProperty(
    "Container Load Service", kDefaultLoadService,
    "composition_interfaces/srv/LoadNode service of an rclcpp_components container.",
    this, SLOT(updateConfiguration()));
  auto_load_property_ = new rviz_common::properties::BoolProperty(
    "Auto Load", true, "Load or reload the depth_image_proc component when enabled or edited.",
    this, SLOT(updateConfiguration()));
  exact_sync_property_ = new rviz_common::properties::BoolProperty(
    "Exact Sync", false, "Pass exact_sync to depth_image_proc::PointCloudXyzrgbNode.",
    this, SLOT(updateConfiguration()));
  queue_size_property_ = new rviz_common::properties::IntProperty(
    "Queue Size", 30, "Pass queue_size to depth_image_proc::PointCloudXyzrgbNode.",
    this, SLOT(updateConfiguration()));
  queue_size_property_->setMin(1);
}

PointCloudXyzrgbDisplay::~PointCloudXyzrgbDisplay()
{
}

void PointCloudXyzrgbDisplay::onInitialize()
{
  rviz_default_plugins::displays::PointCloud2Display::onInitialize();
  const auto rviz_ros_node = context_->getRosNodeAbstraction();
  rgb_image_property_->initialize(rviz_ros_node);
  rgb_info_property_->initialize(rviz_ros_node);
  depth_image_property_->initialize(rviz_ros_node);
  depth_info_property_->initialize(rviz_ros_node);
  ensureRosNode();
  configurePointCloudDisplay();
}

void PointCloudXyzrgbDisplay::onEnable()
{
  configurePointCloudDisplay();
  rviz_default_plugins::displays::PointCloud2Display::onEnable();
  if (auto_load_property_->getBool()) {
    requestLoad();
  }
}

void PointCloudXyzrgbDisplay::onDisable()
{
  rviz_default_plugins::displays::PointCloud2Display::onDisable();
}

void PointCloudXyzrgbDisplay::update(float wall_dt, float ros_dt)
{
  rviz_default_plugins::displays::PointCloud2Display::update(wall_dt, ros_dt);
  retry_elapsed_ += wall_dt;
  pollServiceFutures();

  if (unload_requested_ && !unload_future_.valid()) {
    processUnloadRequest();
  }
  if (load_requested_ && !load_future_.valid() && !unload_future_.valid()) {
    processLoadRequest();
  }
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
  load_client_.reset();
  unload_client_.reset();
  configurePointCloudDisplay();
  if (isEnabled() && auto_load_property_->getBool()) {
    requestReload();
  }
}

void PointCloudXyzrgbDisplay::requestLoad()
{
  load_requested_ = true;
  retry_elapsed_ = 1.0f;
}

void PointCloudXyzrgbDisplay::requestReload()
{
  if (loaded_node_id_ == 0) {
    requestLoad();
    return;
  }
  requestUnload(true);
}

void PointCloudXyzrgbDisplay::requestUnload(bool load_after_unload)
{
  unload_requested_ = true;
  load_after_unload_ = load_after_unload;
  retry_elapsed_ = 1.0f;
}

void PointCloudXyzrgbDisplay::processLoadRequest()
{
  if (retry_elapsed_ < 1.0f || !ensureRosNode()) {
    return;
  }
  retry_elapsed_ = 0.0f;

  if (!load_client_) {
    load_client_ = node_->create_client<LoadNode>(container_service_property_->getStdString());
  }
  if (!load_client_->service_is_ready()) {
    setDisplayStatus(
      rviz_common::properties::StatusProperty::Warn, "Container load service is not ready");
    return;
  }

  load_future_ = load_client_->async_send_request(makeLoadRequest()).future.share();
  load_requested_ = false;
  setDisplayStatus(
    rviz_common::properties::StatusProperty::Warn, "Loading RGB-D point cloud generator");
}

void PointCloudXyzrgbDisplay::processUnloadRequest()
{
  if (loaded_node_id_ == 0) {
    unload_requested_ = false;
    if (load_after_unload_) {
      load_after_unload_ = false;
      requestLoad();
    }
    return;
  }

  if (retry_elapsed_ < 1.0f || !ensureRosNode()) {
    return;
  }
  retry_elapsed_ = 0.0f;

  if (!unload_client_) {
    unload_client_ = node_->create_client<UnloadNode>(unloadServiceName());
  }
  if (!unload_client_->service_is_ready()) {
    setDisplayStatus(
      rviz_common::properties::StatusProperty::Warn, "Container unload service is not ready");
    return;
  }

  auto request = std::make_shared<UnloadNode::Request>();
  request->unique_id = loaded_node_id_;
  unload_future_ = unload_client_->async_send_request(request).future.share();
  unload_requested_ = false;
  setDisplayStatus(
    rviz_common::properties::StatusProperty::Warn, "Stopping RGB-D point cloud generator");
}

void PointCloudXyzrgbDisplay::pollServiceFutures()
{
  if (load_future_.valid() &&
      load_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    const auto response = load_future_.get();
    load_future_ = rclcpp::Client<LoadNode>::SharedFuture();
    if (response->success) {
      loaded_node_id_ = response->unique_id;
      configurePointCloudDisplay();
      setDisplayStatus(
        rviz_common::properties::StatusProperty::Ok,
        QString("Loaded %1").arg(QString::fromStdString(response->full_node_name)));
    } else {
      setDisplayStatus(
        rviz_common::properties::StatusProperty::Error,
        QString("Load failed: %1").arg(QString::fromStdString(response->error_message)));
    }
  }

  if (unload_future_.valid() &&
      unload_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    const auto response = unload_future_.get();
    unload_future_ = rclcpp::Client<UnloadNode>::SharedFuture();
    if (response->success) {
      loaded_node_id_ = 0;
      setDisplayStatus(rviz_common::properties::StatusProperty::Ok, "RGB-D point cloud generator stopped");
      if (load_after_unload_) {
        load_after_unload_ = false;
        requestLoad();
      }
    } else {
      setDisplayStatus(
        rviz_common::properties::StatusProperty::Error,
        QString("Stop failed: %1").arg(QString::fromStdString(response->error_message)));
    }
  }
}

std::string PointCloudXyzrgbDisplay::unloadServiceName() const
{
  std::string service = container_service_property_->getStdString();
  const std::string suffix = "/load_node";
  if (service.size() >= suffix.size() &&
      service.compare(service.size() - suffix.size(), suffix.size(), suffix) == 0) {
    service.replace(service.size() - suffix.size(), suffix.size(), "/unload_node");
  }
  return service;
}

PointCloudXyzrgbDisplay::LoadNode::Request::SharedPtr
PointCloudXyzrgbDisplay::makeLoadRequest() const
{
  auto request = std::make_shared<LoadNode::Request>();
  request->package_name = "depth_image_proc";
  request->plugin_name = "depth_image_proc::PointCloudXyzrgbNode";
  request->node_name = "points_xyzrgb_generator";
  request->node_namespace = "";
  request->remap_rules = {
    "rgb/image_rect_color:=" + rgb_image_property_->getTopicStd(),
    "rgb/camera_info:=" + rgb_info_property_->getTopicStd(),
    "depth_registered/image_rect:=" + depth_image_property_->getTopicStd(),
    "depth_registered/camera_info:=" + depth_info_property_->getTopicStd(),
    "points:=" + output_topic_property_->getStdString()
  };
  request->parameters.push_back(
    makeParameter("exact_sync", rclcpp::ParameterValue(exact_sync_property_->getBool())));
  request->parameters.push_back(
    makeParameter("queue_size", rclcpp::ParameterValue(queue_size_property_->getInt())));
  return request;
}

void PointCloudXyzrgbDisplay::configurePointCloudDisplay()
{
  const QString topic = output_topic_property_->getString().trimmed();
  if (topic.isEmpty()) {
    setDisplayStatus(rviz_common::properties::StatusProperty::Error, "Point cloud output topic is empty");
    return;
  }

  setTopic(topic, kPointCloud2Type);
  if (subProp("Style")) {
    subProp("Style")->setValue("Points");
  }
  if (subProp("Color Transformer")) {
    subProp("Color Transformer")->setValue("RGB8");
  }
}

void PointCloudXyzrgbDisplay::setDisplayStatus(
  rviz_common::properties::StatusProperty::Level level, const QString& text)
{
  setStatus(level, "PointCloudXyzrgb", text);
}

}  // namespace jsk_rviz_plugins

PLUGINLIB_EXPORT_CLASS(jsk_rviz_plugins::PointCloudXyzrgbDisplay, rviz_common::Display)
