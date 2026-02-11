#pragma once

#include <string>
#include <vector>

class CommandLineArgs {
public:
    CommandLineArgs(int argc, char* argv[]);

    [[nodiscard]] bool enableSFML() const { return m_enableSFML; }
    [[nodiscard]] bool enableColorLight() const { return m_enableColorLight; }
    [[nodiscard]] const std::string& colorLightInterface() const { return m_colorLightInterface; }
    [[nodiscard]] bool showHelp() const { return m_showHelp; }
    void printHelp(const char* appName) const;

private:
    bool m_enableSFML = false;
    bool m_enableColorLight = false;
    std::string m_colorLightInterface = "enx00e04c68012e";
    bool m_showHelp = false;

    void parseArgs(int argc, char* argv[]);
};
