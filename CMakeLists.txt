cmake_minimum_required(VERSION 3.20)
project(fcgs LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts SerialPort)

add_executable(fcgs
    src/main.cpp
    src/ui/main_window.cpp
    src/ui/telemetry_chart_widget.cpp
    src/ui/telemetry_3d_widget.cpp
    src/ui/trajectory_widget.cpp
    src/ui/telemetry_values_widget.cpp
    src/ui/magcalibrationwidget.cpp
    src/ui/telemetry_messages_widget.cpp
    src/datasource/serialreader.cpp
    src/calibration/magcalibrator.cpp
    src/calibration/calibrationcontroller.cpp
)

target_include_directories(fcgs PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fcgs
    Qt6::Widgets
    Qt6::Charts
    Qt6::SerialPort
)
