# Android toolchain for armeabi-v7a

set(CMAKE_SYSTEM_NAME Android)

# Path to your NDK
set(ANDROID_NDK /lib/android-sdk/ndk/25.2.9519653)

# Target ABI
set(ANDROID_ABI armeabi-v7a)

# Android API level
set(ANDROID_PLATFORM android-16)

# Use clang
set(ANDROID_TOOLCHAIN clang)

# STL
set(ANDROID_STL c++_shared)

# Enable NEON (recommended)
set(ANDROID_ARM_NEON TRUE)

# Official Android toolchain
set(CMAKE_TOOLCHAIN_FILE
    ${ANDROID_NDK}/build/cmake/android.toolchain.cmake
    CACHE FILEPATH "Android toolchain file"
)

