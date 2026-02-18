#include "ResourceLocator.h"
#include <cpplocate/cpplocate.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ResourceLocator::ResourceLocator() {
    // 1. Try local path (development)
    std::string modulePath = cpplocate::getModulePath();
    
    if (!modulePath.empty() && fs::exists(modulePath + "/fonts")) {
        _basePath = modulePath;
    } 
    // 2. Try standard installation path
    else if (fs::exists("/usr/share/hockey-scoreboard/fonts")) {
        _basePath = "/usr/share/hockey-scoreboard";
    }
    // 3. Fallback
    else {
        std::cerr << "Warning: Could not locate resources in local or install paths. Falling back to current directory." << std::endl;
        _basePath = ".";
    }
    
    std::cout << "Resource base path selected: " << _basePath << std::endl;
}

std::string ResourceLocator::getFontsDirPath() const {
    return _basePath + "/fonts";
}

std::string ResourceLocator::getDataDirPath() const {
    return _basePath + "/data";
}
