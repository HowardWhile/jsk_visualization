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
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_rendering/objects/point_cloud.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>
#endif

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

class PointCloudXyzrgbDisplay : public rviz_common::Display
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
  void reset() override;

private Q_SLOTS:
  void updateConfiguration();
  void updateRenderProperties();

private:
  using Image = sensor_msgs::msg::Image;
  using CameraInfo = sensor_msgs::msg::CameraInfo;

  bool ensureRosNode();
  void subscribe();
  void unsubscribe();
  void processLatestMessages();
  bool makePointCloud(
    const Image::ConstSharedPtr& rgb,
    const Image::ConstSharedPtr& depth,
    const CameraInfo::ConstSharedPtr& depth_info,
    std::vector<rviz_rendering::PointCloud::Point>& points,
    std::string& error) const;
  bool readDepth(const Image& depth, uint32_t x, uint32_t y, float& z) const;
  bool readColor(const Image& rgb, uint32_t x, uint32_t y, float& r, float& g, float& b) const;
  void applyRenderProperties();
  void setDisplayStatus(rviz_common::properties::StatusProperty::Level level, const QString& text);

  PointCloudXyzrgbTopicProperty* rgb_image_property_;
  PointCloudXyzrgbTopicProperty* rgb_info_property_;
  PointCloudXyzrgbTopicProperty* depth_image_property_;
  PointCloudXyzrgbTopicProperty* depth_info_property_;
  rviz_common::properties::IntProperty* stride_property_;
  rviz_common::properties::IntProperty* max_points_property_;
  rviz_common::properties::FloatProperty* max_render_fps_property_;
  rviz_common::properties::FloatProperty* point_size_property_;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<Image>::SharedPtr rgb_sub_;
  rclcpp::Subscription<CameraInfo>::SharedPtr rgb_info_sub_;
  rclcpp::Subscription<Image>::SharedPtr depth_sub_;
  rclcpp::Subscription<CameraInfo>::SharedPtr depth_info_sub_;
  std::shared_ptr<rviz_rendering::PointCloud> point_cloud_;

  mutable std::mutex mutex_;
  Image::ConstSharedPtr latest_rgb_;
  CameraInfo::ConstSharedPtr latest_rgb_info_;
  Image::ConstSharedPtr latest_depth_;
  CameraInfo::ConstSharedPtr latest_depth_info_;
  bool new_data_;
  float render_elapsed_;
};

}  // namespace jsk_rviz_plugins

#endif  // JSK_RVIZ_PLUGINS_POINT_CLOUD_XYZRGB_DISPLAY_H_
