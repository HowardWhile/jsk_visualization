cmake_minimum_required(VERSION 3.8)
project(jsk_rqt_plugins)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

ament_python_install_package(${PROJECT_NAME} PACKAGE_DIR src/${PROJECT_NAME})

file(GLOB PYTHON_SCRIPTS bin/*)
install(PROGRAMS ${PYTHON_SCRIPTS}
  DESTINATION lib/${PROJECT_NAME}
)

file(GLOB PYTHON_SAMPLES sample_scripts/*.py)
install(PROGRAMS ${PYTHON_SAMPLES}
  DESTINATION share/${PROJECT_NAME}/sample_scripts
)

file(GLOB PYTHON_TESTS test/*.py)
install(PROGRAMS ${PYTHON_TESTS}
  DESTINATION share/${PROJECT_NAME}/test
)

install(FILES plugin.xml
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY launch resource sample sample_scripts test
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS
)

ament_package()
