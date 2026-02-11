#!/bin/bash
/home/ron/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/home/ron/.local/share/JetBrains/Toolbox/apps/clion/bin/ninja/linux/x64/ninja -DCMAKE_TOOLCHAIN_FILE=/home/ron/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja -S . -B cmake-build-debug
