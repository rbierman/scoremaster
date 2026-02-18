cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED SCOREMASTER_VERSION)
    set(SCOREMASTER_VERSION "1.0.0-dev")
endif()

project(scoremaster-controller VERSION ${SCOREMASTER_VERSION})

set(CMAKE_CXX_STANDARD 26)

option(ENABLE_SFML "Enable SFML display and simulation support" ON)

set(SOURCES
        main.cpp
        display/DoubleFramebuffer.cpp
        display/IDisplay.h
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
        network/Base64Coder.cpp
)

if(ENABLE_SFML)
    list(APPEND SOURCES 
        display/SFMLDisplay.cpp 
        display/SFMLDisplay.h 
        KeyboardControl.h 
        KeyboardControl.cpp)
endif()

add_executable(scoremaster-controller ${SOURCES})
target_compile_definitions(scoremaster-controller PRIVATE SCOREMASTER_VERSION="${SCOREMASTER_VERSION}")

if(ENABLE_SFML)
    target_compile_definitions(scoremaster-controller PRIVATE ENABLE_SFML)
    find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
    target_link_libraries(scoremaster-controller PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)
endif()

find_package(Threads REQUIRED)

find_package(ixwebsocket CONFIG REQUIRED)
target_link_libraries(scoremaster-controller PRIVATE ixwebsocket::ixwebsocket Threads::Threads)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(scoremaster-controller PRIVATE nlohmann_json::nlohmann_json)

find_package(blend2d CONFIG REQUIRED)
target_link_libraries(scoremaster-controller PRIVATE blend2d::blend2d)

find_package(cpplocate REQUIRED)
target_link_libraries(scoremaster-controller PRIVATE cpplocate::cpplocate)

# --- INSTALLATION ---

include(GNUInstallDirs)

# Install binary
install(TARGETS scoremaster-controller
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install assets (Fonts and Data)
install(DIRECTORY fonts DESTINATION ${CMAKE_INSTALL_DATADIR}/scoremaster-controller)
# Create data dir if it doesn't exist in source, or just ensure it's installed
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    install(DIRECTORY data DESTINATION ${CMAKE_INSTALL_DATADIR}/scoremaster-controller)
endif()

# Install systemd service
install(FILES packaging/scoremaster-controller.service 
              packaging/scoremaster-update.service
              packaging/scoremaster-update.timer
        DESTINATION lib/systemd/system)

# Install update script
install(PROGRAMS packaging/scoremaster-update.sh 
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME scoremaster-update)

# --- CPACK (DEB Generation) ---

set(CPACK_PACKAGE_NAME "scoremaster-controller")
set(CPACK_PACKAGE_VENDOR "ScoreMaster")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance hockey scoreboard renderer")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_CONTACT "admin@example.com")

# Debian specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ScoreMaster Maintainer")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

if(ENABLE_SFML)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsfml-graphics3.0, libsfml-window3.0, libsfml-system3.0, libsfml-network3.0, curl, jq, debconf, iproute2")
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "curl, jq, debconf, iproute2")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-headless-Linux")
endif()

# Attach scripts
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/preinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/config;${CMAKE_CURRENT_SOURCE_DIR}/packaging/templates")

include(CPack)
