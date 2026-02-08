# Android toolchain for arm64-v8a (aarch64)

set(CMAKE_SYSTEM_NAME Android)

# Path to your NDK
set(ANDROID_NDK /lib/android-sdk/ndk/25.2.9519653)

# Target ABI
set(ANDROID_ABI arm64-v8a)

# Android API level (REQUIRED for 64-bit)
set(ANDROID_PLATFORM android-21)

# Use clang
set(ANDROID_TOOLCHAIN clang)

# STL
set(ANDROID_STL c++_shared)

# Official Android toolchain
set(CMAKE_TOOLCHAIN_FILE
    ${ANDROID_NDK}/build/cmake/android.toolchain.cmake
    CACHE FILEPATH "Android toolchain file"
)


