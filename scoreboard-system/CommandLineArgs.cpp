#include "CommandLineArgs.h"
#include <iostream>

CommandLineArgs::CommandLineArgs(const int argc, char* argv[]) {
    parseArgs(argc, argv);
}

void CommandLineArgs::parseArgs(const int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--sfml") {
            m_enableSFML = true;
        } else if (arg == "-c" || arg == "--colorlight") {
            m_enableColorLight = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                m_colorLightInterface = argv[++i];
            }
        } else if (arg == "-h" || arg == "--help") {
            m_showHelp = true;
            return; // Stop parsing if help is requested
        }
    }
}

void CommandLineArgs::printHelp(const char* appName) const {
    std::cout << "Usage: " << appName << " [OPTIONS]" << std::endl;
    std::cout << "  -s, --sfml         Enable SFML display (default: enabled)" << std::endl;
    std::cout << "  -c, --colorlight [interface] Enable ColorLight display (default: disabled). "
              << "Optionally specify network interface, e.g., -c eth0" << std::endl;
    std::cout << "  -h, --help         Show this help message" << std::endl;
}
