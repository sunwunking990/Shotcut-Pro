# ShotcutCPP - Development Setup Guide

## ⚠️ Important Notice

**This project is under active development and is NOT production-ready.** This guide is intended for developers who want to contribute to or experiment with the ShotcutCPP codebase.

**Current Status:**
- The project has compilation issues that need resolution
- Many systems are framework-only and not yet functional
- Integration between components is incomplete
- This is a work-in-progress development environment

---

## Project Overview

### What is ShotcutCPP?

ShotcutCPP is a modern C++23 rewrite of the Shotcut video editor, designed to leverage cutting-edge technology for professional video editing:

- **Modern C++23**: Using latest language features for performance and safety
- **Vulkan Graphics**: GPU-accelerated rendering for UI and video processing
- **Hardware Acceleration**: NVIDIA NVENC/NVDEC for video encoding/decoding
- **Professional Architecture**: Entity Component System (ECS) for timeline management
- **Cross-Platform**: Targeting Windows, Linux, and potentially macOS

### Current Development Status

**What's Been Accomplished:**
- ✅ Project structure and build system established
- ✅ Core architectural framework designed and partially implemented
- ✅ Individual system components created (UI, video, audio, timeline, effects, export)
- ✅ Comprehensive documentation and development guides

**What Still Needs Work:**
- ❌ Resolve compilation errors across multiple modules
- ❌ Complete implementation of framework components
- ❌ System integration and end-to-end workflows
- ❌ Testing and validation infrastructure
- ❌ Performance optimization

**Completion Status:** ~75% architectural framework, ~25% functional implementation

---

## Development Environment Setup

### System Requirements

#### Minimum Requirements
- **OS**: Windows 10/11 or Linux (Ubuntu 20.04+, Arch Linux, etc.)
- **RAM**: 8GB (16GB recommended for comfortable development)
- **Storage**: 10GB free space (for source code, build artifacts, and dependencies)
- **GPU**: Any GPU with Vulkan 1.3+ support (NVIDIA GPU recommended for CUDA/NVENC features)

#### Recommended Development Setup
- **OS**: Linux (Ubuntu 22.04/24.04 or Arch Linux) or Windows 11
- **RAM**: 16GB+
- **GPU**: NVIDIA RTX series (for testing hardware acceleration)
- **CPU**: Modern x86-64 processor with AVX2 support
- **Storage**: NVMe SSD

### Required Development Tools

#### Compiler Support

**Linux:**
```bash
# Ubuntu/Debian - Install GCC 13+
sudo apt update
sudo apt install build-essential gcc-13 g++-13

# Or Clang 16+
sudo apt install clang-16 libc++-16-dev libc++abi-16-dev

# Arch Linux - Install GCC 13+
sudo pacman -S base-devel gcc
```

**Windows:**
- Visual Studio 2022 (Community Edition or higher)
- Ensure "Desktop development with C++" workload is installed
- Ensure Windows 10/11 SDK is installed

#### Build Tools

**CMake 3.20+:**
```bash
# Linux (Ubuntu/Debian)
sudo apt install cmake

# Arch Linux
sudo pacman -S cmake

# Windows
# Download from https://cmake.org/download/
# Or use Visual Studio's built-in CMake
```

#### Graphics APIs

**Vulkan SDK 1.3+:**
```bash
# Linux
# Download from https://vulkan.lunarg.com/
# Or use package manager:
sudo apt install vulkan-sdk  # Ubuntu/Debian
sudo pacman -S vulkan-devel  # Arch Linux

# Windows
# Download installer from https://vulkan.lunarg.com/
```

#### Optional Dependencies

**FFmpeg (for codec support):**
```bash
# Linux
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev  # Ubuntu/Debian
sudo pacman -S ffmpeg  # Arch Linux

# Windows
# Download from https://ffmpeg.org/
# Add to PATH or use vcpkg
```

**NVIDIA Video Codec SDK (for hardware acceleration):**
- Download from https://developer.nvidia.com/nvidia-video-codec-sdk
- Requires NVIDIA GPU and drivers
- Extract headers to a known location for CMake to find

---

## Building the Project

### Clone the Repository

```bash
git clone https://github.com/yourusername/Shotcut-Pro.git
cd Shotcut-Pro
```

### Build Configuration

#### Linux Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DCMAKE_CXX_COMPILER=g++-13

# Build (expect some compilation errors currently)
cmake --build . -j$(nproc)
```

#### Windows Build

```bash
# Open Developer Command Prompt for VS 2022

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release
```

### Build Options

| CMake Option | Description | Default |
|-------------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Release/Debug/RelWithDebInfo) | Release |
| `ENABLE_VULKAN` | Enable Vulkan rendering support | ON |
| `ENABLE_CUDA` | Enable CUDA acceleration features | ON |
| `ENABLE_FFMPEG` | Enable FFmpeg codec integration | ON |
| `ENABLE_SECURITY_HARDENING` | Enable compiler security flags | ON |

**Example:**
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_VULKAN=ON \
  -DENABLE_FFMPEG=ON \
  -DENABLE_CUDA=OFF
```

### Expected Build Issues

**Current Known Problems:**
- NVIDIA Video SDK integration has compilation errors
- Frame buffer system has type resolution issues
- Export system has missing forward declarations
- Some namespace resolution conflicts

**These issues are being actively worked on.** Check `docs/Project_Status_Report.md` for the latest status.

---

## Project Structure

```
Shotcut-Pro/
├── CMakeLists.txt              # Root build configuration
├── LICENSE                     # Proprietary license
├── README.md                   # Project overview
├── SETUP_GUIDE.md             # This file
│
├── src/                        # Source code implementation
│   ├── core/                  # Core application framework
│   ├── rendering/             # Vulkan rendering context
│   ├── ui/                    # User interface framework
│   ├── video/                 # Video processing pipeline
│   ├── audio/                 # Audio processing engine
│   ├── timeline/              # Timeline ECS system
│   ├── effects/               # GPU effects processing
│   ├── platform/              # Platform abstraction layer
│   └── utils/                 # Utility functions
│
├── include/                    # Public header files
│   ├── core/
│   ├── ui/
│   ├── video/
│   ├── audio/
│   ├── timeline/
│   ├── effects/
│   └── rendering/
│
├── cmake/                      # CMake modules
│   └── SecurityFlags.cmake    # Security hardening flags
│
├── docs/                       # Documentation
│   ├── Project_Status_Report.md       # Current project status
│   ├── CODEBASE_ANALYSIS.md          # Architecture analysis
│   ├── GRAPHICS_IMPLEMENTATION_STATUS.md
│   └── guides/                        # Development phase guides
│       ├── Phase2_UI_Framework.md
│       ├── Phase3_Video_Processing.md
│       └── ...
│
├── memory-bank/                # Project context and planning
│   ├── progress.md            # Historical progress tracking
│   ├── productContext.md      # Product vision and context
│   └── projectbrief.md        # Original project brief
│
├── tests/                      # Test suite (to be implemented)
└── external/                   # External dependencies (planned)
```

---

## Development Workflow

### Understanding the Codebase

1. **Start with Documentation:**
   - Read `README.md` for project overview
   - Review `docs/Project_Status_Report.md` for current status
   - Check `docs/CODEBASE_ANALYSIS.md` for architecture details

2. **Explore the Code:**
   - Core systems in `src/core/` and `src/rendering/`
   - Individual modules in `src/ui/`, `src/video/`, `src/audio/`, etc.
   - Public APIs in `include/`

3. **Check Phase Guides:**
   - `docs/guides/` contains detailed documentation for each development phase
   - Useful for understanding design decisions and implementation details

### Building Individual Components

The project uses modular CMake targets:

```bash
# Build only the UI framework
cmake --build . --target shotcut_ui

# Build only the video processing system
cmake --build . --target shotcut_video

# Build demo applications
cmake --build . --target shotcut_ui_demo
cmake --build . --target shotcut_video_demo
```

### Running Demo Applications

Once built (when compilation issues are resolved):

```bash
# From the build directory

# UI Framework Demo
./bin/shotcut_ui_demo

# Video Processing Demo
./bin/shotcut_video_demo

# Timeline System Demo
./bin/shotcut_timeline_demo

# Effects & Audio Demo
./bin/shotcut_effects_demo
```

**Note:** Many demos are currently non-functional due to compilation issues.

---

## Troubleshooting

### Common Build Issues

**Issue: CMake can't find Vulkan SDK**
```bash
# Set VULKAN_SDK environment variable
export VULKAN_SDK=/path/to/vulkan/sdk  # Linux
set VULKAN_SDK=C:\VulkanSDK\1.3.xxx.x  # Windows

# Or specify manually
cmake .. -DVULKAN_SDK=/path/to/vulkan/sdk
```

**Issue: Compiler version too old**
```bash
# Verify compiler version
gcc --version  # Should be 13+ for GCC
clang --version  # Should be 16+ for Clang

# Update compiler or specify newer version
cmake .. -DCMAKE_CXX_COMPILER=g++-13
```

**Issue: FFmpeg not found**
```bash
# Install development packages
sudo apt install libavcodec-dev libavformat-dev libavutil-dev

# Or disable FFmpeg support
cmake .. -DENABLE_FFMPEG=OFF
```

**Issue: NVIDIA SDK compilation errors**
```bash
# This is a known issue being worked on
# You can disable NVIDIA SDK integration for now:
cmake .. -DENABLE_CUDA=OFF
```

### Getting Help

- Check `docs/Project_Status_Report.md` for known issues
- Review relevant phase guides in `docs/guides/`
- Open an issue on GitHub (when available)

---

## Next Steps After Setup

### For Contributors

1. **Familiarize with the architecture:**
   - Read architectural documentation in `docs/`
   - Understand the ECS pattern used for timeline
   - Review the Vulkan rendering pipeline

2. **Pick a focus area:**
   - Check `docs/Project_Status_Report.md` for priority tasks
   - Choose a module to work on (UI, video, audio, etc.)
   - Start with smaller, isolated issues

3. **Development best practices:**
   - Follow C++23 modern practices (RAII, smart pointers)
   - Write clean, documented code
   - Test your changes on target platforms

### For Experimenters

1. **Explore the codebase:**
   - Try building different components
   - Read through the implementation code
   - Experiment with modifying demo applications

2. **Learn from the architecture:**
   - Study the ECS implementation
   - Understand Vulkan integration patterns
   - Review hardware acceleration strategies

---

## Additional Resources

### Documentation
- **Project Status**: `docs/Project_Status_Report.md`
- **Architecture**: `docs/CODEBASE_ANALYSIS.md`
- **Phase Guides**: `docs/guides/Phase*.md`
- **Progress**: `memory-bank/progress.md`

### External Resources
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [C++23 Reference](https://en.cppreference.com/)
- [NVIDIA Video Codec SDK](https://developer.nvidia.com/nvidia-video-codec-sdk)
- [FFmpeg Documentation](https://ffmpeg.org/documentation.html)
- [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system)

---

## License

This project is proprietary software. All rights reserved.

See [LICENSE](LICENSE) for details.

---

## Contact and Support

This is a development project. For questions or discussions:

- Check project documentation first
- Review existing issues and discussions
- Open new issues for bugs or feature requests (when available)

---

**Last Updated:** November 2025
**Status:** Active Development - Not Production Ready
