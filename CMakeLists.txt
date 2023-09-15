cmake_minimum_required(VERSION 3.16.0)
project(fuse-loader)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC include/fuse-loader.h source/loader.c)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC dl)
target_compile_options(${PROJECT_NAME} PRIVATE -pedantic -Wall -Wextra)

# Add fuse3 support
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)
target_link_libraries(${PROJECT_NAME} PRIVATE ${FUSE3_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE ${FUSE3_CFLAGS})

if (${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    add_executable(example source/example.c)
    target_link_libraries(example PRIVATE ${PROJECT_NAME})

    add_library(test SHARED source/test.cpp)
endif()