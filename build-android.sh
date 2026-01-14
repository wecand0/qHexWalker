#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-android"

# Paths
QT_PATH="$HOME/Qt/6.9.0"
QT_ANDROID="$QT_PATH/android_arm64_v8a"
QT_HOST="$QT_PATH/gcc_64"
ANDROID_SDK="$HOME/Android/Sdk"
ANDROID_NDK="$ANDROID_SDK/ndk/26.1.10909125"
VCPKG_ROOT="$HOME/.vcpkg-clion/vcpkg"

echo "=== Building qHexWalker for Android ARM64 ==="

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure if needed
if [ ! -f "Makefile" ] || [ "$1" == "--clean" ]; then
    if [ "$1" == "--clean" ]; then
        echo "Cleaning build directory..."
        rm -rf *
    fi

    echo "Configuring CMake..."
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET=arm64-android \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
        -DCMAKE_FIND_ROOT_PATH="$QT_ANDROID;$VCPKG_ROOT/installed/arm64-android" \
        -DQT_HOST_PATH="$QT_HOST" \
        -DANDROID_SDK_ROOT="$ANDROID_SDK" \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-28 \
        -DBUILD_TESTS=OFF \
        -DBENCHMARK_ENABLE=OFF
fi

# Build
echo "Building..."
cmake --build . -j$(nproc)

# Copy Qt Java bindings if missing
BINDINGS_DIR="$BUILD_DIR/src/android-build/src/org/qtproject/qt/android/bindings"
if [ ! -d "$BINDINGS_DIR" ]; then
    echo "Copying Qt Java bindings..."
    mkdir -p "$BINDINGS_DIR"
    cp "$QT_ANDROID/src/android/java/src/org/qtproject/qt/android/bindings/"*.java "$BINDINGS_DIR/"
fi

# Build APK
echo "Building APK..."
cmake --build . --target apk

APK_PATH="$BUILD_DIR/src/android-build/build/outputs/apk/debug/android-build-debug.apk"

cd "$BUILD_DIR/src/android-build/build/outputs/apk/debug/"
keytool -genkey -v   -keystore my-upload-key.jks   -keyalg RSA -keysize 2048 -validity 10000   -alias upload   -dname "CN=Your Name, OU=YourOrg, O=Company, L=City, S=State, C=NL"
echo > "123456"
echo > "123456"
/home/user/Android/Sdk/build-tools/34.0.0/apksigner sign -ks my-upload-key.jks android-build-release-unsigned.apk
adb install -r android-build-release-unsigned.apk

echo ""
echo "=== Build complete ==="
echo "APK: $APK_PATH"
echo ""
echo "To install: adb install -r \"$APK_PATH\""