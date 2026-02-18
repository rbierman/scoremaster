# Application Icons

This directory contains the source image used for generating application launcher icons across all platforms (Android, iOS, Linux).

## How to Update the App Icon

1.  Replace `icon.png` in this directory with your new icon. 
    *   **Format**: PNG
    *   **Recommended Size**: 512x512 pixels
    *   **Design**: Keep the important content within a centered circular safe zone to ensure it looks good on all platforms (especially Android's adaptive icons).

2.  Run the following commands in your terminal from the `scoreboard-app` directory:

```bash
flutter pub get
dart run flutter_launcher_icons
```

## Configuration

The icon generation is configured in the `pubspec.yaml` file under the `flutter_launcher_icons` section.
