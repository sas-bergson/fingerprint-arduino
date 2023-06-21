# TODO 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

# TODO 2: Create command to generate documentation for the project
# add_custom_command(
#     OUTPUT
#         ../../docs/doxygen.stamp
#     DEPENDS
#         ../docs/doxygen.cfg
#     COMMAND
#         doxygen ../../docs/doxygen.cfg
#     COMMAND
#         cmake -E touch ../../../docs/doxygen.stamp
#     COMMENT
#         "Generating API documentation with Doxygen"
#     VERBATIM
# )

add_custom_command(
    OUTPUT
        ../../docs/doxygen.stamp
    DEPENDS
        ../docs/doxygen.cfg
    COMMAND
        doxygen ../../docs/doxygen.cfg
    COMMAND
        cmake -E touch ../../docs/doxygen.stamp
    COMMAND
        make -C ../../docs/latex
    COMMENT
        "Generating API documentation with Doxygen"
    VERBATIM
)

# TODO 3: Create a custom target
add_custom_target(
    docs ALL
    DEPENDS doxygen.stamp
)