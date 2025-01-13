cmake_minimum_required(VERSION 3.16)
project(RocketControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Bluetooth Core Widgets)

add_executable(RocketControl
    main.cpp
    MainWindow.cpp
    MainWindow.h
    RocketController.cpp
    RocketController.h
)

target_link_libraries(RocketControl PRIVATE
    Qt6::Bluetooth
    Qt6::Core
    Qt6::Widgets
) 