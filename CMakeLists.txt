cmake_minimum_required(VERSION 4.1)
project(hockey-scoreboard)

set(CMAKE_CXX_STANDARD 26)

add_executable(hockey-scoreboard main.cpp
        display/DoubleFramebuffer.cpp
        display/IDisplay.h
        display/SFMLDisplay.cpp
        display/SFMLDisplay.h
        display/ColorLightDisplay.cpp
        display/ColorLightDisplay.h
        ScoreboardController.h
        ScoreboardController.cpp
        CommandLineArgs.h
        CommandLineArgs.cpp
        ResourceLocator.h
        ResourceLocator.cpp)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)

find_package(blend2d CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE blend2d::blend2d)

find_package(cpplocate REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE cpplocate::cpplocate)

# Copy fonts directory to the binary directory
add_custom_command(TARGET hockey-scoreboard PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/fonts
            ${CMAKE_BINARY_DIR}/fonts
    COMMENT "Copying fonts directory"
)
