# Shotcut Technology Analysis & Modernization Plan

## Repository Information
- **Official Repository**: https://github.com/mltframework/shotcut
- **Repository Size**: 171 MB
- **License**: GPLv3
- **Primary Author**: Dan Dennedy
- **Current Status**: Active open-source project

## Current Technology Stack

### Core Technologies
1. **Programming Languages**:
   - C++ (192 source files) - C++11 standard
   - QML (410 files) - Qt Quick declarative UI framework
   - 150+ video filter modules

2. **Build System**:
   - CMake (minimum version 3.12)
   - Supports Linux, macOS, and Windows
   - Automated builds via GitHub Actions

3. **UI Framework**:
   - Qt 6.4+ (minimum requirement)
   - Qt Quick/QML for declarative UI
   - Qt Widgets for some legacy UI components
   - Qt Components: Charts, Multimedia, Network, OpenGL, QuickControls2, WebSockets, Sql, Xml

4. **Multimedia Framework**:
   - **MLT Framework** (Media Lovin' Toolkit) v7.32.0+ - Core multimedia engine
   - **FFmpeg** - Format and codec support
   - **Frei0r** - Video plugin framework
   - **SDL** - Audio playback
   - **FFTW** - Fast Fourier Transform library

5. **Platform-Specific Components**:
   - Direct3D video widget (Windows)
   - Metal video widget (macOS)
   - OpenGL video widget (cross-platform)
   - X11 support (Linux)

## Project Structure

```
shotcut-original/
├── src/
│   ├── qml/                  # 410+ QML UI files
│   │   ├── filters/          # 150 filter modules
│   │   ├── views/            # Main view components
│   │   ├── modules/          # Reusable QML modules
│   │   └── scopes/           # Audio/video scopes
│   ├── widgets/              # Qt Widget-based UI components
│   ├── models/               # Data models
│   ├── controllers/          # Business logic controllers
│   ├── commands/             # Command pattern implementations
│   ├── docks/                # Dockable panels
│   ├── dialogs/              # Dialog windows
│   ├── jobs/                 # Background job system
│   └── main.cpp              # Application entry point
├── CuteLogger/               # Logging library
├── icons/                    # Application icons
├── translations/             # i18n support
├── packaging/                # Platform-specific packaging
└── CMakeLists.txt           # Build configuration
```

## Key Components Analysis

### 1. Core Application
- **mainwindow.cpp** (244KB) - Main application window and orchestration
- **mltcontroller.cpp** (67KB) - MLT framework integration
- **player.cpp** (56KB) - Media playback engine
- **settings.cpp** (43KB) - Application settings management
- **util.cpp** (44KB) - Utility functions

### 2. Video Rendering
- Multiple rendering backends:
  - Metal (macOS) - Modern, efficient
  - Direct3D (Windows) - Platform-native
  - OpenGL - Cross-platform fallback

### 3. Filter System
- 150+ QML-based filters
- Declarative filter UI definitions
- Separation of UI and processing logic

## Modernization Opportunities

### 1. C++ Standards Upgrade
**Current**: C++11
**Target**: C++17 or C++20
**Benefits**:
- Better performance with modern optimizations
- Improved type safety (std::optional, std::variant)
- Filesystem library (std::filesystem)
- Parallel algorithms (std::execution)
- Better memory management (smart pointers enhancements)

### 2. Replace MLT Framework Dependencies
**Current**: Heavy reliance on MLT framework
**Opportunity**: Build proprietary engine
**Approach**:
- Create custom video processing pipeline
- Direct FFmpeg integration for better control
- Modern GPU-accelerated effects engine
- Custom timeline and clip management

### 3. Modern Graphics APIs
**Current**: OpenGL, Metal, Direct3D separate implementations
**Target**: Unified modern graphics
**Options**:
- Vulkan for cross-platform high-performance
- WebGPU for future web compatibility
- Enhanced Metal/Direct3D 12 usage
- Compute shaders for effects processing

### 4. Architecture Improvements
**Current**: Mixed Qt Widgets and QML
**Target**: Unified modern architecture
**Approach**:
- Full QML/Qt Quick interface (or custom UI framework)
- Clean separation of concerns (Model-View-ViewModel)
- Plugin architecture for effects and transitions
- Real-time collaboration support

### 5. Advanced Features to Implement
Based on industry-leading editors:
- **AI-Powered Features**:
  - Auto-reframe/smart crop
  - Scene detection and categorization
  - Audio ducking and enhancement
  - Color matching across clips
  - Speech-to-text and auto-captioning

- **Professional Effects**:
  - Motion tracking
  - Advanced color grading (HDR support)
  - 3D compositing
  - Particle systems
  - Advanced masking and rotoscoping

- **Performance Enhancements**:
  - Proxy workflow improvements
  - Background rendering
  - GPU-accelerated everything
  - Multi-threaded timeline scrubbing
  - Smart caching system

- **Modern Workflow**:
  - Cloud integration
  - Team collaboration features
  - Version control for projects
  - Asset management system
  - Template marketplace

### 6. Technology Replacements

| Current | Modernize To | Reason |
|---------|-------------|---------|
| C++11 | C++20 | Modern features, better performance |
| MLT Framework | Custom Engine | Full control, proprietary features |
| Mixed UI (Widgets/QML) | Unified Modern UI | Consistency, performance |
| OpenGL | Vulkan/Metal/DX12 | Better GPU utilization |
| Single-user | Collaboration-ready | Modern workflow needs |
| Desktop-only | Desktop + Web | Broader accessibility |

## Immediate Next Steps

### Phase 1: Foundation (Weeks 1-4)
1. Set up modern C++ build environment
2. Upgrade to C++17/20 standard
3. Analyze and document MLT framework usage points
4. Create abstraction layer for video engine
5. Set up modern testing infrastructure

### Phase 2: Core Engine (Weeks 5-12)
1. Design proprietary video processing engine
2. Implement custom timeline/clip management
3. Direct FFmpeg integration layer
4. GPU-accelerated rendering pipeline
5. Frame-accurate seeking and playback

### Phase 3: Effects & Features (Weeks 13-20)
1. Plugin architecture for effects
2. Port essential filters to new engine
3. Implement advanced color grading
4. Motion tracking foundation
5. GPU compute shader effects

### Phase 4: UI Modernization (Weeks 21-28)
1. Design modern UI/UX
2. Implement new interface layer
3. Responsive timeline component
4. Enhanced preview system
5. Customizable workspace

## Technical Challenges to Address

1. **Performance Parity**: Ensure new engine matches or exceeds MLT performance
2. **Format Support**: Maintain broad codec/format compatibility
3. **Stability**: Match Shotcut's reliability while adding features
4. **Cross-Platform**: Support Windows, macOS, Linux equally
5. **Migration Path**: Consider importing Shotcut projects
6. **Learning Curve**: Balance power with usability

## Competitive Analysis Targets

Features to match or exceed:
- **DaVinci Resolve**: Color grading, Fusion effects
- **Adobe Premiere Pro**: AI features, integration ecosystem
- **Final Cut Pro**: Magnetic timeline, optimization
- **Kdenlive**: Open-source collaboration
- **CapCut**: Modern UI, templates, effects

## Resources Required

1. **Development Team**:
   - C++ engine developers (2-3)
   - UI/UX designers (1-2)
   - Graphics programming specialists (1-2)
   - QA/Testing (1-2)

2. **Infrastructure**:
   - CI/CD pipeline
   - Testing farm (multiple platforms)
   - Performance profiling tools
   - Documentation system

3. **Third-Party Libraries** (Potential):
   - FFmpeg (LGPL/GPL)
   - Vulkan SDK
   - GPU compute libraries
   - Modern C++ libraries (Boost, etc.)

## License Considerations

**Important**: Shotcut is GPLv3. Your new project can:
1. Use it as reference/inspiration (clean room approach)
2. Fork and modify (must remain GPLv3)
3. Build from scratch (your choice of license)

**Recommendation**: Build from scratch with clean-room design to allow proprietary licensing if desired, while maintaining ability to study Shotcut's approaches.

---

**Document Version**: 1.0
**Created**: 2025-11-17
**Next Review**: After Phase 1 completion
