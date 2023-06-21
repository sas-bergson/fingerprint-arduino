# TODO 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

# TODO 2: Create a project named fingerprintsensor
project("fingerprintsensor" 
        LANGUAGES CXX 
        VERSION 1.0
        DESCRIPTION "Fingerprint system based on Arduino"
        HOMEPAGE_URL "https://github.com/sas-bergson/fingerprint-arduino")

# TODO 3: Set the variable CMAKE_CXX_STANDARD to 11
#         and the variable CMAKE_CXX_STANDARD_REQUIRED to True
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(GNUInstallDirs)

set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# defines targets and sources
# add_subdirectory(docs)
 add_subdirectory(src)
 add_subdirectory(include)

# contains an "external" library we will link to
# add_subdirectory(external)

# enable testing and define tests
# enable_testing()
# add_subdirectory(tests)
