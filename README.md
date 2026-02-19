# PuckPulse Monorepo

This repository contains the PuckPulse system, a professional-grade hockey scoreboard solution.

## Project Structure

- **`puckpulse-controller/`**: High-performance C++26 scoreboard engine. Handles game logic, 2D rendering (Blend2D), and network communication.
- **`puckpulse-app/`**: Flutter mobile application for remote scoreboard management.

## Installation

Pre-built binaries for all platforms are available on the [GitHub Releases](https://github.com/rbierman/PuckPulse/releases) page.

### 1. PuckPulse Controller (Linux)

The controller is distributed as a Debian package for Raspberry Pi and other Linux systems.

- **Standard**: Includes a local SFML window for display.
- **Headless**: Optimized for devices without a display attached.

To install:
1. Download the appropriate `.deb` package.
2. Install using `dpkg`:
   ```bash
   sudo dpkg -i puckpulse-controller-*.deb
   sudo apt-get install -f # Install missing dependencies
   ```
3. The controller runs as a systemd service:
   ```bash
   sudo systemctl start puckpulse-controller
   ```

### 2. PuckPulse App

#### Android
- Download and open the `.apk` file on your Android device to install.

#### Linux Desktop
1. Download `puckpulse-app-linux.tar.gz`.
2. Extract and run:
   ```bash
   tar -xzvf puckpulse-app-linux.tar.gz
   ./puckpulse_app
   ```

## Development & Releasing

For detailed development instructions, build guides, and release processes for each component, please refer to their respective README files:

- [PuckPulse Controller (C++)](puckpulse-controller/README.md)
- [PuckPulse App (Flutter)](puckpulse-app/README.md)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
