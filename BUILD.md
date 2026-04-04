# Building IanniX

IanniX uses CMake (≥ 3.17) and Qt 5. Qt 6 support is in progress but not yet complete.
Release builds are 64-bit only; 32-bit releases have been discontinued.

---

## Table of Contents

1. [Linux](#linux)
   - [openSUSE Tumbleweed](#opensuse-tumbleweed)
   - [Ubuntu 22.04 / 24.04](#ubuntu-2204--2404)
2. [macOS](#macos)
3. [Windows](#windows)
4. [CMake Options](#cmake-options)

---

## Linux

### openSUSE Tumbleweed

**Install dependencies**

```bash
sudo zypper install \
    cmake \
    gcc-c++ \
    libqt5-qtbase-devel \
    libqt5-qtdeclarative-devel \
    libqt5-qtserialport-devel \
    libqt5-qtwebsockets-devel \
    libqt5-qtx11extras-devel \
    muparser-devel \
    rtmidi-devel
```

**Configure and build**

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

The binary is written to `build/iannix`.

**Install system-wide (optional)**

```bash
sudo cmake --install build
```

This installs the binary to `/usr/local/bin/iannix`, the `.desktop` file to
`/usr/local/share/applications/`, and the icon to `/usr/share/pixmaps/`.

---

### Ubuntu 22.04 / 24.04

**Install dependencies**

```bash
sudo apt install \
    cmake \
    g++ \
    qtbase5-dev \
    libqt5opengl5-dev \
    qtdeclarative5-dev \
    libqt5serialport5-dev \
    libqt5websockets5-dev \
    libmuparser-dev \
    librtmidi-dev
```

**Configure and build**

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

The binary is written to `build/iannix`.

**Install system-wide (optional)**

```bash
sudo cmake --install build
```

---

## macOS

**Prerequisites**

- Xcode Command Line Tools: `xcode-select --install`
- [Homebrew](https://brew.sh)
- Syphon.framework placed in `/Library/Frameworks`
  (download from <http://syphon.v002.info/>)

**Install dependencies**

```bash
brew install cmake qt@5
```

Add Qt to your PATH so CMake can find it:

```bash
export PATH="$(brew --prefix qt@5)/bin:$PATH"
```

**Configure and build**

```bash
cmake -B build -S .
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

The application bundle is written to `build/IanniX.app`.

**Optional: Wacom tablet support**

```bash
cmake -B build -S . -DUSE_WACOM=ON
```

**Optional: Kinect support**

Install [libfreenect](https://openkinect.org/wiki/Getting_Started), then:

```bash
cmake -B build -S . -DUSE_KINECT=ON
```

---

## Windows

**Prerequisites**

- [CMake ≥ 3.17](https://cmake.org/download/)
- [Qt 5 for Windows](https://www.qt.io/download) — install the MSVC or MinGW
  variant to match your compiler
- Visual Studio 2019/2022 (MSVC) **or** MinGW-w64
- Use a 64-bit toolchain. 32-bit Windows releases are no longer supported.

**Dependency notes**

- `muParser` and `RtMidi` are no longer vendored — they must be provided as
  system packages.  On Windows, use [vcpkg](https://vcpkg.io) (see below).
- On Windows, RtMidi uses the native Multimedia backend and links against
  `winmm`, `setupapi`, `advapi32`, `user32`, and `opengl32` automatically.
- Optional features: `USE_FFMPEG=ON` requires FFmpeg dev libraries (also
  available via vcpkg).  `USE_KINECT` is gated to Unix builds; `USE_WACOM` is
  macOS-only.

**vcpkg (required for muParser, RtMidi, and optional FFmpeg)**

Windows has no system package manager, so [vcpkg](https://vcpkg.io) fills that
role.

Install vcpkg once:

```bat
git clone https://github.com/microsoft/vcpkg %USERPROFILE%\vcpkg
%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat
```

Install the required and optional packages:

```bat
:: Required
%USERPROFILE%\vcpkg\vcpkg install muparser rtmidi --triplet x64-windows

:: Core Qt dependencies (alternative to the Qt installer)
%USERPROFILE%\vcpkg\vcpkg install qt5-base qt5-declarative qt5-serialport qt5-websockets --triplet x64-windows

:: Optional: FFmpeg (enables USE_FFMPEG=ON)
%USERPROFILE%\vcpkg\vcpkg install ffmpeg --triplet x64-windows
```

Pass the vcpkg toolchain file to CMake:

```bat
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

**Configure and build (MSVC, Developer Command Prompt)**

```bat
cmake -B build -S . -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64
cmake --build build --config Release
```

**Configure and build (MinGW)**

```bat
cmake -B build -S . ^
    -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\mingw81_64
cmake --build build -j%NUMBER_OF_PROCESSORS%
```

The executable is written to `build\Release\IanniX.exe` (MSVC) or
`build\IanniX.exe` (MinGW).

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `QT_VERSION` | `5` | Qt major version (`5` or `6`). Qt 6 is experimental. |
| `USE_FFMPEG` | `OFF` | Enable FFmpeg video recording (requires `libavcodec`, `libavutil`, `libavformat`, `libswscale`). |
| `USE_KINECT` | `OFF` | Enable Kinect support via libfreenect (Linux/macOS only). |
| `USE_WACOM` | `OFF` | Enable Wacom tablet support (macOS only). |

Pass options at configure time:

```bash
cmake -B build -S . -DUSE_FFMPEG=ON
```
