cmake_minimum_required(VERSION 3.8)
project(jsk_rviz_plugins)

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

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

execute_process(
  COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_path('purelib', vars={'base': '${CMAKE_INSTALL_PREFIX}', 'platbase': '${CMAKE_INSTALL_PREFIX}'}))"
  OUTPUT_VARIABLE PYTHON_INSTALL_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(RELATIVE_PATH PYTHON_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}" "${PYTHON_INSTALL_DIR}")

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

ament_export_dependencies(rosidl_default_runtime)
ament_package()
