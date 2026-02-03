#pragma once

#include <string>
#include <vector>
#include <cpplocate/cpplocate.h> // Include cpplocate

class ResourceLocator {
public:
    ResourceLocator(); // Modified constructor

    std::string getFontsDirPath() const;

private:
    std::string _basePath;
};