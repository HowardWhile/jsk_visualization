cmake_minimum_required(VERSION 3.8)
project(jsk_interactive_marker)

find_package(ament_cmake REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(jsk_recognition_msgs QUIET)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

set(interface_files
  msg/JointTrajectoryPointWithType.msg
  msg/JointTrajectoryWithType.msg
  msg/MarkerDimensions.msg
  msg/MarkerMenu.msg
  msg/MarkerPose.msg
  msg/MoveModel.msg
  msg/MoveObject.msg
  msg/PoseStampedWithName.msg
  msg/SnapFootPrintInput.msg
  srv/GetJointState.srv
  srv/GetMarkerDimensions.srv
  srv/GetTransformableMarkerColor.srv
  srv/GetTransformableMarkerExistence.srv
  srv/GetTransformableMarkerFocus.srv
  srv/GetTransformableMarkerPose.srv
  srv/GetType.srv
  srv/MarkerSetPose.srv
  srv/RemoveParentMarker.srv
  srv/SetHeuristic.srv
  srv/SetMarkerDimensions.srv
  srv/SetParentMarker.srv
  srv/SetPose.srv
  srv/SetTransformableMarkerColor.srv
  srv/SetTransformableMarkerFocus.srv
  srv/SetTransformableMarkerPose.srv
  srv/SnapFootPrint.srv
)

set(interface_dependencies
  builtin_interfaces
  geometry_msgs
  sensor_msgs
  std_msgs
  visualization_msgs
)

if(jsk_recognition_msgs_FOUND)
  list(APPEND interface_files srv/IndexRequest.srv)
  list(APPEND interface_dependencies jsk_recognition_msgs)
else()
  message(WARNING "jsk_recognition_msgs was not found; skipping srv/IndexRequest.srv")
endif()

rosidl_generate_interfaces(${PROJECT_NAME}
  ${interface_files}
  DEPENDENCIES
    ${interface_dependencies}
)

file(GLOB PYTHON_SCRIPTS scripts/*.py)
install(PROGRAMS ${PYTHON_SCRIPTS}
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES plugin_description.xml
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config euslisp launch models urdf
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION include/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
