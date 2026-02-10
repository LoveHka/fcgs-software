cmake_minimum_required(VERSION 3.20)
project(fcgs LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDART 17)
set(CMAKE_CXX_STANDART_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts SerialPort)


add_executable(fcgs
	src/main.cpp
	src/ui/main_window.cpp
    src/datasource/datasource.cpp
    src/datasource/serialreader.cpp
)

target_include_directories(fcgs PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fcgs
	Qt6::Widgets
    Qt6::Charts
    Qt6::SerialPort
)
