// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2026, Howard Cheng
 *  All rights reserved.
 *********************************************************************/

#ifndef JSK_RVIZ_PLUGINS_POINT_CLOUD_XYZRGB_DISPLAY_H_
#define JSK_RVIZ_PLUGINS_POINT_CLOUD_XYZRGB_DISPLAY_H_

#ifndef Q_MOC_RUN
#include <cstdint>
#include <memory>
#include <string>

#include <composition_interfaces/srv/load_node.hpp>
#include <composition_interfaces/srv/unload_node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/string_property.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_default_plugins/displays/pointcloud/point_cloud2_display.hpp>
#endif

namespace rviz_common
{
class Display;
}  // namespace rviz_common

namespace jsk_rviz_plugins
{

class PointCloudXyzrgbTopicProperty : public rviz_common::properties::EditableEnumProperty
{
public:
  PointCloudXyzrgbTopicProperty(
    const QString& name,
    const QString& default_value,
    const QString& description,
    const std::string& type_name,
    rviz_common::properties::Property* parent,
    const char* changed_slot);

  void initialize(rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr rviz_ros_node);
  std::string getTopicStd() const;

private:
  void fillTopicList();

  std::string type_name_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr rviz_ros_node_;
};

class PointCloudXyzrgbDisplay : public rviz_default_plugins::displays::PointCloud2Display
{
  Q_OBJECT

public:
  PointCloudXyzrgbDisplay();
  ~PointCloudXyzrgbDisplay() override;

protected:
  void onInitialize() override;
  void onEnable() override;
  void onDisable() override;
  void update(float wall_dt, float ros_dt) override;

private Q_SLOTS:
  void updateConfiguration();

private:
  using LoadNode = composition_interfaces::srv::LoadNode;
  using UnloadNode = composition_interfaces::srv::UnloadNode;

  bool ensureRosNode();
  void requestLoad();
  void requestReload();
  void requestUnload(bool load_after_unload);
  void processLoadRequest();
  void processUnloadRequest();
  void pollServiceFutures();
  std::string resolveLoadServiceName();
  std::string resolveUnloadServiceName() const;
  std::string unloadServiceName() const;
  LoadNode::Request::SharedPtr makeLoadRequest() const;
  void configurePointCloudDisplay();
  void setDisplayStatus(rviz_common::properties::StatusProperty::Level level, const QString& text);

  PointCloudXyzrgbTopicProperty* rgb_image_property_;
  PointCloudXyzrgbTopicProperty* rgb_info_property_;
  PointCloudXyzrgbTopicProperty* depth_image_property_;
  PointCloudXyzrgbTopicProperty* depth_info_property_;
  rviz_common::properties::StringProperty* output_topic_property_;
  rviz_common::properties::StringProperty* container_service_property_;
  rviz_common::properties::BoolProperty* auto_load_property_;
  rviz_common::properties::BoolProperty* exact_sync_property_;
  rviz_common::properties::IntProperty* queue_size_property_;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<LoadNode>::SharedPtr load_client_;
  rclcpp::Client<UnloadNode>::SharedPtr unload_client_;
  rclcpp::Client<LoadNode>::SharedFuture load_future_;
  rclcpp::Client<UnloadNode>::SharedFuture unload_future_;
  uint64_t loaded_node_id_;
  bool load_requested_;
  bool unload_requested_;
  bool load_after_unload_;
  float retry_elapsed_;
  std::string active_load_service_name_;
};

}  // namespace jsk_rviz_plugins

#endif  // JSK_RVIZ_PLUGINS_POINT_CLOUD_XYZRGB_DISPLAY_H_
