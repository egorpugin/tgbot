cmake_minimum_required(VERSION 3.13)

project(report_detection_bot CXX)

set(CMAKE_CXX_STANDARD 26)
#set(SW_BUILD_SHARED_LIBS 1)
set(DEPENDENCIES
    org.sw.demo.tgbot
    pub.egorpugin.primitives.http
    pub.egorpugin.primitives.main
    pub.egorpugin.primitives.sw.settings
)

find_package(SW REQUIRED)
sw_add_package(${DEPENDENCIES})
sw_execute()

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
