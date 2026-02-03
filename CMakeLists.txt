cmake_minimum_required(VERSION 3.20)
project(fcgs LANGUAGES CXX)

set(CMAKE_CXX_STANDART 17)
set(CMAKE_CXX_STANDART_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(fcgs
	src/main.cpp
	src/ui/main_window.cpp
)

target_link_libraries(fcgs
	Qt6::Widgets
)
