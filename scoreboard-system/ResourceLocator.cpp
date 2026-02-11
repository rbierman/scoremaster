#include "ResourceLocator.h"
#include <cpplocate/cpplocate.h>
#include <iostream>

ResourceLocator::ResourceLocator() {
    // Use cpplocate to find the path of the current executable module
    _basePath = cpplocate::getModulePath();

    if (_basePath.empty()) {
        std::cerr << "Warning: Could not locate base resource path. Falling back to current directory." << std::endl;
        _basePath = ".";
    }
     std::cout << "Resource base path: " << _basePath << std::endl;
}

std::string ResourceLocator::getFontsDirPath() const {
    return _basePath + "/fonts";
}
