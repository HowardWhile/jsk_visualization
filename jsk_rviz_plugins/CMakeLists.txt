cmake_minimum_required(VERSION 3.8)
project(jsk_rviz_plugins)

find_package(ament_cmake REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(image_geometry REQUIRED)
find_package(image_transport REQUIRED)
find_package(pluginlib REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_ogre_vendor REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

get_filename_component(OGRE_INCLUDE_PARENT_DIR "${OGRE_INCLUDE_DIRS}" DIRECTORY)

rosidl_generate_interfaces(${PROJECT_NAME}
  msg/ObjectFitCommand.msg
  msg/OverlayMenu.msg
  msg/OverlayText.msg
  msg/Pictogram.msg
  msg/PictogramArray.msg
  msg/RecordCommand.msg
  msg/StringStamped.msg
  msg/TransformableMarkerOperate.msg
  srv/EusCommand.srv
  srv/RequestMarkerOperate.srv
  srv/Screenshot.srv
  DEPENDENCIES geometry_msgs std_msgs
)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

qt5_wrap_cpp(jsk_rviz_plugins_moc_files
  src/camera_info_display.h
  src/image_transport_hints_property.h
  src/overlay_text_display.h
  src/pie_chart_display.h
  src/string_display.h
  src/tf_trajectory_display.h
)

add_library(${PROJECT_NAME}_display_plugins SHARED
  src/camera_info_display.cpp
  src/image_transport_hints_property.cpp
  src/overlay_text_display.cpp
  src/overlay_utils.cpp
  src/pie_chart_display.cpp
  src/string_display.cpp
  src/tf_trajectory_display.cpp
  ${jsk_rviz_plugins_moc_files}
)

target_include_directories(${PROJECT_NAME}_display_plugins PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  ${OGRE_INCLUDE_DIRS}
  ${OGRE_INCLUDE_PARENT_DIR}
)

target_link_libraries(${PROJECT_NAME}_display_plugins
  ${cpp_typesupport_target}
  Qt5::Widgets
  rviz_common::rviz_common
  rviz_ogre_vendor::OgreMain
  rviz_ogre_vendor::OgreOverlay
  rviz_rendering::rviz_rendering
)

ament_target_dependencies(${PROJECT_NAME}_display_plugins
  cv_bridge
  image_geometry
  image_transport
  pluginlib
  rclcpp
  rviz_common
  rviz_ogre_vendor
  rviz_rendering
  sensor_msgs
  std_msgs
)

pluginlib_export_plugin_description_file(rviz_common plugin_description.xml)

set(PYTHON_INSTALL_DIR
  lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages
)

install(TARGETS ${PROJECT_NAME}_display_plugins
  EXPORT ${PROJECT_NAME}_display_plugins
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY python/${PROJECT_NAME}/
  DESTINATION ${PYTHON_INSTALL_DIR}/${PROJECT_NAME}
)

file(GLOB PYTHON_SCRIPTS scripts/*.py)
install(PROGRAMS ${PYTHON_SCRIPTS}
  DESTINATION lib/${PROJECT_NAME}
)

file(GLOB PYTHON_SAMPLES samples/*.py)
install(PROGRAMS ${PYTHON_SAMPLES}
  DESTINATION share/${PROJECT_NAME}/samples
)

install(FILES plugin_description.xml
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY cfg config icons launch test
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS
)

install(DIRECTORY src/
  DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h"
)

ament_export_targets(${PROJECT_NAME}_display_plugins HAS_LIBRARY_TARGET)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
