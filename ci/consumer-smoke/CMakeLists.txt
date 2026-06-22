cmake_minimum_required(VERSION 3.16)
project(ross_consumer_smoke C)

# Validates the supported CMake-config consumer surface against an installed
# ROSS: find_package(ROSS) resolves ROSSConfig.cmake, and the ROSS::ROSS
# imported target carries the include dirs, the MPI dependency, and the link
# libraries. Point CMAKE_PREFIX_PATH at the ROSS install prefix when
# configuring this project.
find_package(ROSS REQUIRED)

add_executable(consumer_smoke main.c)
target_link_libraries(consumer_smoke PRIVATE ROSS::ROSS)
