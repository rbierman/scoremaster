# ScoreMaster Controller

High-performance hockey scoreboard renderer and game logic controller.

## Features

- **High Performance**: Built with C++26 and Blend2D for efficient 2D rendering.
- **Dual Display**: Supports both SFML (local window) and ColorLight LED controllers.
- **mDNS Discovery**: Automatically advertises itself on the network for easy connection from the mobile app.
- **Remote Control**: Managed via a WebSocket-based protocol.
- **Headless Mode**: Can run on resource-constrained devices without a local display.

## System Architecture

The controller acts as the central hub for the ScoreMaster system:
1. **Game Logic**: Manages scores, period clock, and penalties.
2. **Rendering**: Produces a real-time visual representation of the scoreboard.
3. **Networking**: Hosts a WebSocket server for control commands and broadcasts state updates to connected apps.
4. **Discovery**: Uses mDNS to allow mobile apps to find the controller without manual IP entry.

## Build Instructions

### Prerequisites
- CMake 3.28+
- Modern C++ compiler (supporting C++26)
- [vcpkg](https://github.com/microsoft/vcpkg) for dependency management
- Ninja (recommended)

### Local Development
To configure the project with the specific toolchain and settings:

```bash
cmake -B cmake-build-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]/scripts/buildsystems/vcpkg.cmake
```

Build the binary:
```bash
ninja -C cmake-build-debug
```

## Running

Run the controller from the build directory:
```bash
./cmake-build-debug/scoremaster-controller
```

### Command Line Options
- `-s, --sfml`: Enable/Disable SFML local display.
- `-c, --colorlight [interface]`: Enable ColorLight LED output on a specific network interface.
- `-h, --help`: Show all available options.

## Installation

The project supports generating Debian packages for easy deployment on Raspberry Pi or other Linux systems:

```bash
cd build
cpack -G DEB
sudo dpkg -i scoremaster-controller-*.deb
```

When installed as a package, it runs as a systemd service (`scoremaster-controller.service`).

## Development & Releasing

For a detailed list of changes between versions, see the [CHANGELOG.md](CHANGELOG.md).

### CI/CD Workflow
This project uses GitHub Actions for automated releases. The workflow:
1. Compiles the controller for both Standard (SFML) and Headless targets.
2. Packages the binaries into `.deb` files.
3. Creates a new GitHub Release with these artifacts.

To trigger a release using the GitHub CLI:
```bash
gh workflow run release.yml
```
By default, this auto-increments the patch version. You can specify a version manually with `-f version=X.Y.Z`.
