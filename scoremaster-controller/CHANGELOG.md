# Changelog

All notable changes to the ScoreMaster Controller will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-02-18

### Added
- **Mobile App Releases**: The Flutter mobile application is now automatically built and released alongside the controller.
- **Android APK**: Automated generation of release APKs for the mobile app.
- **Linux App Bundle**: Automated packaging of the Flutter app for Linux.

### Changed
- **CI/CD Pipeline**: Refactored release workflow into parallel jobs for faster builds and better reliability. Added caching for Gradle and Flutter Pub dependencies to significantly reduce Android build times.

## [1.0.0] - 2026-02-18

### Added
- **High Performance Rendering**: Initial release with Blend2D-based rendering engine.
- **Dual Display Support**: Support for local SFML window and ColorLight LED controllers.
- **mDNS Discovery**: Automatic service advertisement for mobile app connection.
- **Remote Control**: WebSocket-based protocol for managing game state.
- **Headless Mode**: Support for running on resource-constrained devices without local display.
- **Debian Packaging**: Support for building `.deb` packages for easy deployment.
- **CI/CD Integration**: Automated build and release workflow via GitHub Actions.
