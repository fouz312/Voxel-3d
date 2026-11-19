# Voxel3D in GitHub Codespaces

This project is configured for GitHub Codespaces.

## Open it

1. Upload this repository to GitHub.
2. Open the repository and choose **Code → Codespaces → Create codespace on main**.
3. The dev container installs the C++/CMake environment and configures the project.
4. In the Codespaces terminal, build the desktop version with:

```bash
cmake --build build --config Release -j2
```

## Android APK

You do not need Android Studio to build the Android version in GitHub's cloud CI.

1. Open the repository's **Actions** tab.
2. Select **Build Voxel3D Android APK**.
3. Choose **Run workflow**.
4. When it finishes, open the workflow run and download the `Voxel3D-debug-apk` artifact.
5. Extract the artifact and install `app-debug.apk` on your Android phone.

The Android workflow installs the Android SDK, NDK and CMake automatically, then builds `main.cpp` (with `game.h` and `art.h`) using raylib 5.5. The first Android build downloads raylib from its official GitHub repository.

## Notes

- The game uses original stylized voxel colors, not Mojang/Minecraft proprietary assets.
- The Android build is currently a debug APK.
- If the Android toolchain exposes a raylib/API compatibility issue, the failed Actions log identifies the exact compiler step that needs adjustment.
