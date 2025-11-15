# ShotcutCPP - Modern Video Editor Rewrite

<div align="center">

**A modern C++23 rewrite of Shotcut video editor - Currently Under Active Development**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B23)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3+-purple.svg)](https://www.vulkan.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-blue.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)](#platforms)

**Status:** 🚧 **Active Development** - Not Production Ready

</div>

---

## ⚠️ Development Status Notice

This project is **currently under active development** and is **not yet functional** for production use. While significant architectural work has been completed, the codebase has compilation issues and incomplete integrations that are being actively resolved.

**What works:**
- Core architecture and framework design
- Individual system components (with some compilation issues)
- Project structure and build system foundation

**What doesn't work yet:**
- Complete compilation across all modules
- Full system integration
- End-to-end video editing workflow
- Many advertised features are framework-only (not yet functional)

**Current Focus:** Resolving compilation errors and establishing a working minimal viable product (MVP).

---

## Overview

### What is ShotcutCPP?

ShotcutCPP is an ambitious project to create a modern, professional-grade video editor by reimagining Shotcut from the ground up using cutting-edge C++23 features and modern graphics APIs. This is a complete rewrite, not an update to the existing Shotcut application.

### Vision & Goals

The goal of ShotcutCPP is to build a video editor that:

- **Leverages Modern Technology**: Built with C++23, Vulkan graphics API, and modern architecture patterns
- **Maximizes Performance**: Hardware-accelerated processing using GPU compute (NVENC/NVDEC, CUDA, Vulkan)
- **Professional Capabilities**: Aims to provide features competitive with Adobe Premiere Pro and DaVinci Resolve
- **Cross-Platform**: Designed for Windows, Linux, and potentially macOS
- **Memory Safe**: Modern C++ practices with RAII, smart pointers, and comprehensive error handling

### Key Architectural Features (Planned/In Development)

- **Vulkan-First Rendering**: Direct GPU control for both UI and video processing
- **Entity Component System (ECS)**: Modern timeline architecture for scalability
- **Hardware Acceleration**: NVIDIA Video Codec SDK integration for encoding/decoding
- **Professional Audio**: 32-bit float processing with low-latency mixing
- **Modular Design**: Clean separation between UI, video processing, audio, effects, and timeline systems

---

## 🎯 Current Development Phase

### Project Completion: ~75% (Framework Architecture)

The project has established the architectural framework for most systems but requires significant implementation work to become functional:

#### System Status Overview

| Component | Architecture | Implementation | Integration | Status |
|-----------|-------------|----------------|-------------|--------|
| **Core Framework** | ✅ Complete | ✅ Complete | ✅ Complete | **Working** |
| **Build System** | ✅ Complete | ✅ Complete | ✅ Complete | **Working** |
| **UI Framework** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |
| **Video Pipeline** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |
| **Audio Engine** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |
| **Timeline ECS** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |
| **Effects System** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |
| **Export System** | ✅ Complete | ⚠️ Partial | ❌ Incomplete | **In Progress** |

### Known Critical Issues

As documented in `docs/Project_Status_Report.md`, the project currently has:

- **Compilation Failures**: Multiple components have unresolved compilation errors
- **Type Resolution Issues**: Forward declaration and namespace issues in several modules
- **Integration Gaps**: Individual systems work in isolation but integration is incomplete
- **Missing Implementations**: Many framework components need actual functionality implemented

---

## 🏗️ Architecture Overview

### Technology Stack

- **Language**: C++23 (requires GCC 13+, Clang 16+, or MSVC 2022+)
- **Graphics API**: Vulkan 1.3+ (primary), DirectX 12 (Windows fallback planned)
- **Build System**: CMake 3.20+ with modular targets
- **GPU Acceleration**: NVIDIA Video Codec SDK 12+ (optional)
- **Audio Processing**: Custom 32-bit float pipeline
- **Video Codecs**: FFmpeg integration (optional)

### Core Systems Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   UI Framework  │    │ Video Pipeline  │    │ Audio Engine    │
│                 │    │                 │    │                 │
│ • Vulkan UI     │◄──►│ • Decode/Encode │◄──►│ • Multi-track   │
│ • Layout Engine │    │ • Frame Buffer  │    │ • Effects       │
│ • Widgets       │    │ • GPU Processing│    │ • Mixing        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │  Timeline ECS   │
                    │                 │
                    │ • Entities      │
                    │ • Components    │
                    │ • Systems       │
                    └─────────────────┘
```

---

## 🚀 Building from Source

### Prerequisites

#### System Requirements (Development)
- **OS**: Windows 10/11 or Linux (Ubuntu 20.04+, Arch, etc.)
- **GPU**: NVIDIA GPU recommended for hardware acceleration testing
- **RAM**: 8GB+ (16GB recommended)
- **Storage**: 5GB for source and build artifacts

#### Development Dependencies
- **Compiler**:
  - Linux: GCC 13+ or Clang 16+
  - Windows: MSVC 2022+ (Visual Studio 2022)
- **CMake**: 3.20 or higher
- **Vulkan SDK**: 1.3 or higher
- **NVIDIA SDK**: Video Codec SDK 12+ (optional, for hardware acceleration)
- **FFmpeg**: 4.4+ (optional, for codec support)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/Shotcut-Pro.git
cd Shotcut-Pro

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (note: currently has compilation errors)
cmake --build . --config Release -j$(nproc)
```

### Build Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Release/Debug/RelWithDebInfo | Release |
| `ENABLE_VULKAN` | Enable Vulkan support | ON |
| `ENABLE_CUDA` | Enable CUDA acceleration | ON |
| `ENABLE_FFMPEG` | Enable FFmpeg integration | ON |
| `ENABLE_SECURITY_HARDENING` | Enable compiler security flags | ON |

**Note**: Due to current compilation issues, not all targets will build successfully.

---

## 📁 Project Structure

```
Shotcut-Pro/
├── src/                    # Source code
│   ├── core/              # Core application framework
│   ├── ui/                # User interface components
│   ├── video/             # Video processing pipeline
│   ├── audio/             # Audio processing engine
│   ├── timeline/          # Timeline ECS system
│   ├── effects/           # GPU effects processing
│   ├── rendering/         # Vulkan rendering
│   └── platform/          # Platform abstraction layer
├── include/               # Public headers
├── cmake/                 # CMake modules
├── docs/                  # Documentation
│   ├── guides/           # Development phase guides
│   ├── CODEBASE_ANALYSIS.md
│   └── Project_Status_Report.md
├── memory-bank/           # Project context and planning
├── external/              # External dependencies (planned)
├── tests/                 # Test suite (to be implemented)
└── CMakeLists.txt         # Root build configuration
```

---

## 📖 Documentation

- **Project Status**: See `docs/Project_Status_Report.md` for detailed current status
- **Codebase Analysis**: See `docs/CODEBASE_ANALYSIS.md` for architecture details
- **Phase Guides**: See `docs/guides/` for development phase documentation
- **Progress Tracking**: See `memory-bank/progress.md` for historical progress

---

## 🤝 Contributing

**Note**: This is currently a development project. The codebase is not yet stable enough for external contributions.

If you're interested in following development or contributing in the future:

1. Check the `docs/Project_Status_Report.md` for current status
2. Review open issues and development roadmap
3. Reach out to discuss potential contributions

### Development Workflow (When Ready)
1. Create feature branch from `develop`
2. Implement changes following C++23 best practices
3. Ensure code compiles on target platforms
4. Submit pull request with detailed description

---

## 🎯 Roadmap

### Immediate Priorities (Next 1-2 Months)

1. **Resolve Compilation Issues**
   - Fix NVIDIA SDK integration errors
   - Resolve frame buffer system issues
   - Fix export system type resolution
   - Address namespace and type conflicts

2. **Establish MVP**
   - Get all modules compiling
   - Basic integration between systems
   - Simple end-to-end workflow (import → edit → export)

3. **Core Functionality**
   - Functional video playback
   - Basic timeline editing
   - Simple effects application
   - Basic export capability

### Medium Term (3-6 Months)

- Complete hardware acceleration integration
- Advanced timeline features
- Professional effects pipeline
- Multi-track audio mixing
- Cross-platform testing and optimization

### Long Term (6-12 Months)

- Performance optimization for 4K/8K workflows
- Advanced color grading tools
- Plugin system architecture
- Comprehensive testing suite
- User documentation and tutorials

---

## 📄 License

This project is proprietary software. All rights reserved.

See [LICENSE](LICENSE) file for details.

---

## ❓ FAQ

### Is this ready to use?
**No.** This project is in active development and is not functional for production use.

### When will it be ready?
The timeline depends on resolving current technical issues. Check `docs/Project_Status_Report.md` for updates.

### How does this relate to the original Shotcut?
This is a complete rewrite from scratch, not based on the existing Shotcut codebase. It's inspired by Shotcut's philosophy but built with modern technology.

### Can I try it?
You can attempt to build from source, but expect compilation errors and incomplete functionality.

### How can I help?
The project isn't ready for external contributions yet, but feedback and interest are appreciated.

---

<div align="center">

**Built with modern C++23 and Vulkan**

*Under Active Development - Check back for updates*

© 2024-2025 ShotcutCPP Development Team

</div>
