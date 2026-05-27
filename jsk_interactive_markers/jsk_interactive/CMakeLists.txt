cmake_minimum_required(VERSION 3.8)
project(jsk_interactive)

find_package(ament_cmake REQUIRED)

install(DIRECTORY euslisp
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
