# Android 16KB Page Size Alignment Guide

## Overview

Starting November 1st, 2025, Google Play requires all new apps and updates targeting Android 15+ (API level 35) to support 16KB page sizes on 64-bit devices. This document explains the changes made to LibPitaya to ensure compliance.

## Background

Android devices are transitioning from 4KB to 16KB memory page sizes for better performance on devices with more RAM. Apps with native libraries (`.so` files) must have their ELF LOAD segments aligned to 16KB boundaries to work properly on these devices.

## Architecture Requirements

| Architecture | 16KB Alignment | Status |
|--------------|----------------|---------|
| **ARM64** (`arm64-v8a`) | **Mandatory** | ✅ Required for Android 15+ |
| **ARMv7** (`armeabi-v7a`) | Optional | 🔄 Recommended for future-proofing |
| **x86_64** | **Mandatory** | N/A (not used in this project) |
| **x86** | Optional | N/A (not used in this project) |

## Changes Made

### 1. CMakeLists.txt Updates

Added 16KB alignment linker flags to the Android build configuration:

```cmake
elseif(ANDROID)
    find_package(Threads REQUIRED)

    # Add 16KB page size alignment for Android (required for Android 15+ devices)
    # This ensures compatibility with devices that use 16KB memory pages
    # Required for ARM64, optional but recommended for ARMv7
    target_link_options(pitaya PRIVATE "-Wl,-z,max-page-size=16384")
```

### 2. Verification Script

Created `verify_16kb_alignment.sh` to check alignment of built libraries:

```bash
# Check all project .so files
./verify_16kb_alignment.sh

# Check specific .so file
./verify_16kb_alignment.sh path/to/library.so
```

## Building with 16KB Alignment

### Prerequisites
```bash
# Install verification tools (macOS)
brew install binutils
```

### Build Commands
```bash
# For ARM64 (mandatory 16KB alignment)
cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 ..
make

# For ARMv7 (optional 16KB alignment)
cmake -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-21 ..
make

# Verify alignment after build
./verify_16kb_alignment.sh
```

## Unity Integration

### Architecture Selection Strategy

**Option 1: ARM64 Only (Recommended)**
- Target only `arm64-v8a` in Unity Player Settings
- Covers 95%+ of modern Android devices
- Simplifies 16KB compliance (only need ARM64)
- Smaller APK size

**Option 2: Dual Architecture**
- Include both `arm64-v8a` and `armeabi-v7a`
- Broader device compatibility
- Both architectures should be 16KB aligned for consistency

### Unity Player Settings
1. Go to **Player Settings > Android > Publishing Settings**
2. Set **Target Architectures** to:
   - ARM64 only: Check only `ARM64`
   - Dual arch: Check both `ARMv7` and `ARM64`

## Verification

### Manual Verification
```bash
# Check alignment with readelf
export PATH="/opt/homebrew/opt/binutils/bin:$PATH"
readelf -l your_library.so | grep -A 1 "LOAD"

# Look for alignment values:
# ✅ 0x4000 (16384) or higher = 16KB aligned
# ❌ 0x1000 (4096) = 4KB aligned (needs fixing)
```

### Automated Verification
```bash
# Use the provided script
./verify_16kb_alignment.sh
```

## Troubleshooting

### Build Issues
- Ensure you're using Android NDK r21 or later
- Verify CMake version 3.7+
- Check that the linker flag is being applied correctly

### Runtime Issues on 16KB Devices
- App fails to install: Library not 16KB aligned
- Performance warnings: Running in compatibility mode
- Crashes on startup: Severe alignment issues

### Testing on 16KB Devices
- **Emulator**: Use Android 15+ system images with 16KB page size
- **Physical devices**: Pixel 8/9 series with Android 15 QPR1+
- **Developer options**: Enable "Boot with 16KB page size" on supported devices

## References

- [Android 16KB Page Size Guide](https://developer.android.com/guide/practices/page-sizes)
- [Google Play 16KB Requirement](https://android-developers.googleblog.com/2023/09/16kb-page-sizes-in-android-15.html)
- [CMake Android Documentation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-android)

## Next Steps

1. **Rebuild libraries** with the updated CMakeLists.txt
2. **Verify alignment** using the provided script
3. **Test on 16KB devices** or emulators
4. **Update Unity packages** with 16KB-aligned libraries
5. **Consider ARM64-only** strategy for new projects

---

**Note**: The changes in this project ensure compliance with Google Play's 16KB page size requirement while maintaining backward compatibility.
