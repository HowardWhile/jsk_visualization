cmake_minimum_required(VERSION 3.8)
project(jsk_interactive_test)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch scripts spacenav
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS
)

file(GLOB PYTHON_SCRIPTS scripts/*.py)
install(PROGRAMS ${PYTHON_SCRIPTS}
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
