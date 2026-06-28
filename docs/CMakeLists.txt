cmake_minimum_required(VERSION 3.10)

# Locate Doxygen; the optional dot component enables call/dependency graphs.
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    # Track all source files so that `make` regenerates docs whenever code changes.
    file(GLOB_RECURSE DOC_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/include/*.hpp"
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/*.ino"
    )

    set(DOXYGEN_STAMP  "${PROJECT_SOURCE_DIR}/docs/doxygen.stamp")
    set(DOXYGEN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg")

    # Doxygen resolves relative INPUT/OUTPUT paths from the working directory
    # (CMAKE_CURRENT_BINARY_DIR = build/docs/), so "../../include" and
    # "../../src" correctly resolve to the project-root include/ and src/.
    add_custom_command(
        OUTPUT
            ${DOXYGEN_STAMP}
        DEPENDS
            ${DOXYGEN_CONFIG}
            ${DOC_SOURCE_FILES}
        COMMAND
            ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
        COMMAND
            cmake -E touch ${DOXYGEN_STAMP}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT
            "Generating API documentation with Doxygen (src/ + include/)"
        VERBATIM
    )

    # ALL makes docs part of the default build: plain `make` also regenerates docs.
    add_custom_target(
        docs ALL
        DEPENDS ${DOXYGEN_STAMP}
    )
else()
    message(WARNING
        "Doxygen not found — API docs will not be generated. "
        "Install with: sudo apt-get install doxygen graphviz"
    )
    # Provide an empty target so dependents referencing `docs` still work.
    add_custom_target(docs)
endif()