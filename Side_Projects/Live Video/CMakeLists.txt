cmake_minimum_required(VERSION 3.10)

set(Boost_INCLUDE_DIR /usr/include)
set(Boost_LIBRARY_DIR /usr/lib/aarch64-linux-gnu)
find_package(Boost 1.74 COMPONENTS program_options REQUIRED)

project(avionics-live-video)

add_executable(avionics-live-video main.cpp)

target_include_directories(avionics-live-video PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(avionics-live-video PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(avionics-live-video PUBLIC /usr/include/libcamera)
target_include_directories(avionics-live-video PUBLIC /usr/include/libcamera-apps)

link_directories(avionics-live-video PUBLIC ${Boost_LIBRARY_DIR})
link_directories(avionics-live-video PUBLIC /usr/lib/aarch64-linux-gnu)

target_sources(avionics-live-video PUBLIC ${CMAKE_SOURCE_DIR}/lv_output.cpp)

target_link_libraries(avionics-live-video PUBLIC libcamera-base.so)
target_link_libraries(avionics-live-video PUBLIC libcamera.so)
target_link_libraries(avionics-live-video PUBLIC libcamera_app.so)
target_link_libraries(avionics-live-video PUBLIC ${Boost_LIBRARIES})