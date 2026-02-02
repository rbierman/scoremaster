cmake_minimum_required(VERSION 4.1)
project(hockey-scoreboard)

set(CMAKE_CXX_STANDARD 26)

add_executable(hockey-scoreboard main.cpp
        display/DoubleFramebuffer.cpp
        display/IDisplay.h
        display/SFMLDisplay.cpp
        display/ColorLightDisplay.cpp
        display/ColorLightDisplay.h)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)
