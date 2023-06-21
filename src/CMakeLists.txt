# TODO 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

# TODO 2: Add an executable to the project
# Hint: Be sure to specify the source file as main.cpp

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE "main.cpp")
target_link_libraries(${PROJECT_NAME} serialport)
target_link_directories(${PROJECT_NAME} BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/lib)
