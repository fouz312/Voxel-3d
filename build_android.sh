#!/usr/bin/env bash
# Builds a debug APK of Voxel3D. Run by .github/workflows/build-android.yml (needs JDK 17, Android SDK + NDK, CMake 3.22, Gradle 8.9).
set -euo pipefail

RAYLIB_TAG="5.5"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/android-app"
CPP="$APP/app/src/main/cpp"
mkdir -p "$CPP"

# 1. raylib source (built from source for Android by CMake)
if [ ! -d "$APP/raylib" ]; then
  git clone --depth 1 --branch "$RAYLIB_TAG" https://github.com/raysan5/raylib.git "$APP/raylib"
fi

# 2. the game sources
cp "$ROOT/main.cpp" "$ROOT/game.h" "$ROOT/art.h" "$CPP/"

# 3. Gradle project files
cat > "$APP/settings.gradle" <<'GRADLE'
pluginManagement { repositories { google(); mavenCentral(); gradlePluginPortal() } }
dependencyResolutionManagement { repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS); repositories { google(); mavenCentral() } }
rootProject.name = 'Voxel3D'
include ':app'
GRADLE

cat > "$APP/build.gradle" <<'GRADLE'
plugins { id 'com.android.application' version '8.7.3' apply false }
GRADLE

cat > "$APP/gradle.properties" <<'PROPS'
org.gradle.jvmargs=-Xmx3g -Dfile.encoding=UTF-8
android.useAndroidX=true
PROPS

mkdir -p "$APP/app/src/main"
cat > "$APP/app/src/main/AndroidManifest.xml" <<'XML'
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <uses-feature android:glEsVersion="0x00020000" android:required="true" />
    <application android:label="Voxel3D" android:hasCode="false" android:allowBackup="true"
                 android:theme="@android:style/Theme.NoTitleBar.Fullscreen">
        <activity android:name="android.app.NativeActivity"
                  android:exported="true"
                  android:screenOrientation="sensorLandscape"
                  android:configChanges="orientation|keyboardHidden|screenSize|screenLayout|uiMode|density"
                  android:launchMode="singleTask">
            <meta-data android:name="android.app.lib_name" android:value="voxel3d" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
XML

# 4. native build: raylib for ANDROID + our game + the NDK's native_app_glue
cat > "$CPP/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.22.1)
project(voxel3d LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLATFORM "Android" CACHE STRING "" FORCE)      # tell raylib to build its Android backend (not desktop GLFW)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

set(NDK_GLUE ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../raylib raylib-build)
target_include_directories(raylib PRIVATE ${NDK_GLUE})

add_library(voxel3d SHARED main.cpp ${NDK_GLUE}/android_native_app_glue.c)
target_include_directories(voxel3d PRIVATE ${NDK_GLUE} ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(voxel3d PRIVATE raylib log android EGL GLESv2 OpenSLES dl m)
target_compile_definitions(voxel3d PRIVATE PLATFORM_ANDROID)
# keep the NativeActivity entry point; tolerate raylib also compiling the glue
target_link_options(voxel3d PRIVATE -u ANativeActivity_onCreate -Wl,--allow-multiple-definition)
CMAKE

cat > "$APP/app/build.gradle" <<'GRADLE'
plugins { id 'com.android.application' }

android {
    namespace 'com.voxel3d.game'
    compileSdk 35
    buildToolsVersion '35.0.0'
    ndkVersion '27.2.12479018'
    defaultConfig {
        applicationId 'com.voxel3d.game'
        minSdk 24
        targetSdk 35
        versionCode 4
        versionName '4.0'
        ndk { abiFilters 'arm64-v8a', 'armeabi-v7a' }   // nearly every Android phone/tablet from the last 8 years
        externalNativeBuild { cmake { cppFlags '-std=c++17'; arguments '-DANDROID_STL=c++_static' } }
    }
    externalNativeBuild { cmake { path 'src/main/cpp/CMakeLists.txt'; version '3.22.1' } }
}
GRADLE

# 5. build (debug-signed APK, installable without a Play Store account)
cd "$APP"
gradle :app:assembleDebug --no-daemon --stacktrace
ls -la app/build/outputs/apk/debug/
