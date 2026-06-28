# TODO 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

# TODO 2: Add an executable to the project
add_library(serialport STATIC)
target_sources(serialport PRIVATE "${PROJECT_SOURCE_DIR}/src/serialport.cpp")
target_include_directories(serialport PUBLIC "${PROJECT_SOURCE_DIR}/include")

# In case you wan to specify those folders per target:
# set_target_properties(serialport
#   PROPERTIES
#   ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
#   LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
#   RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
# )