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
        ScoreboardState.h
        ScoreboardRenderer.h
        ScoreboardRenderer.cpp
        GoalCelebrationRenderer.h
        GoalCelebrationRenderer.cpp
        IRenderer.h
        KeyboardControl.h
        KeyboardControl.cpp
        network/NetworkManager.h
        network/NetworkManager.cpp
        network/WebSocketManager.h
        network/WebSocketManager.cpp
        CommandLineArgs.h
        CommandLineArgs.cpp
        ResourceLocator.h
        ResourceLocator.cpp
        TeamManager.h
        TeamManager.cpp
        network/Base64Coder.h
        network/Base64Coder.cpp)

find_package(ixwebsocket CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE ixwebsocket::ixwebsocket)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE nlohmann_json::nlohmann_json)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)

find_package(blend2d CONFIG REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE blend2d::blend2d)

find_package(cpplocate REQUIRED)
target_link_libraries(hockey-scoreboard PRIVATE cpplocate::cpplocate)

# --- INSTALLATION ---

include(GNUInstallDirs)

# Install binary
install(TARGETS hockey-scoreboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install assets (Fonts and Data)
install(DIRECTORY fonts DESTINATION ${CMAKE_INSTALL_DATADIR}/hockey-scoreboard)
# Create data dir if it doesn't exist in source, or just ensure it's installed
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    install(DIRECTORY data DESTINATION ${CMAKE_INSTALL_DATADIR}/hockey-scoreboard)
endif()

# Install systemd service
install(FILES packaging/hockey-scoreboard.service 
        DESTINATION lib/systemd/system)

# --- CPACK (DEB Generation) ---

set(CPACK_PACKAGE_NAME "hockey-scoreboard")
set(CPACK_PACKAGE_VENDOR "HockeyScoreboard")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance hockey scoreboard renderer")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_CONTACT "admin@example.com")

# Debian specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Scoreboard Maintainer")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsfml-graphics3.0, libsfml-window3.0, libsfml-system3.0, libsfml-network3.0")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# Attach scripts
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/preinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst")

include(CPack)
