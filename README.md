# Shotcut-Pro

A modern, high-performance video editor built with C++20, featuring proprietary technologies and advanced features inspired by industry-leading software.

## Project Status

**Current Phase**: Foundation & Analysis
**Version**: Pre-Alpha / Planning Stage
**Started**: November 2025

This project aims to create a next-generation video editor that:
- Leverages modern C++ (C++17/20) for better performance and safety
- Implements a proprietary video processing engine
- Provides advanced features found in professional editors
- Maintains cross-platform compatibility (Windows, macOS, Linux)
- Offers GPU-accelerated rendering and effects

## Project Goals

### Core Objectives
1. **Modern C++ Architecture**: Upgrade from C++11 to C++20 for improved performance and developer experience
2. **Proprietary Engine**: Replace dependency on MLT framework with custom-built video processing engine
3. **Advanced Features**: Implement professional-grade features including:
   - Motion tracking and stabilization
   - AI-powered editing tools (scene detection, auto-reframe, etc.)
   - Advanced color grading with HDR support
   - Real-time collaboration capabilities
   - GPU-accelerated effects and rendering

4. **Enhanced User Experience**: Modern, intuitive interface with customizable workspaces
5. **Professional Performance**: Frame-accurate editing with smooth 4K/8K preview

## Repository Structure

```
Shotcut-Pro/
├── shotcut-original/         # Reference: Original Shotcut codebase (for study)
├── SHOTCUT_ANALYSIS.md       # Detailed analysis of Shotcut's technology stack
├── ROADMAP.md                # 40-week development roadmap
├── README.md                 # This file
└── (future directories)
    ├── src/                  # Source code (coming soon)
    ├── docs/                 # Documentation
    ├── tests/                # Test suite
    └── examples/             # Example projects and plugins
```

## Documentation

### Essential Reading
1. **[SHOTCUT_ANALYSIS.md](SHOTCUT_ANALYSIS.md)** - Comprehensive analysis of Shotcut's current technology stack, architecture, and modernization opportunities
2. **[ROADMAP.md](ROADMAP.md)** - Detailed 40-week development plan with phases, milestones, and deliverables

### Key Findings from Analysis
- **Current Shotcut Stack**:
  - C++11 with Qt 6.4+
  - MLT Framework for multimedia
  - 192 C++ files, 410 QML files
  - 150+ video filters
  - 171 MB repository size

- **Modernization Opportunities**:
  - Upgrade to C++20
  - Replace MLT with custom engine
  - Unified modern graphics API (Vulkan/Metal/DX12)
  - AI-powered features
  - Cloud collaboration

## Development Phases

### Phase 1: Foundation (Weeks 1-4) - IN PROGRESS
- [x] Clone and analyze Shotcut repository
- [x] Document current technology stack
- [x] Create modernization roadmap
- [ ] Set up build system and CI/CD
- [ ] Finalize technology stack

### Phase 2: Core Video Engine (Weeks 5-12)
- Custom FFmpeg integration
- Timeline and clip management
- GPU-accelerated rendering pipeline

### Phase 3: Graphics & Effects (Weeks 13-20)
- GPU compute foundation
- Core and advanced effects library
- Plugin system

### Phase 4: Advanced Features (Weeks 21-28)
- Motion tracking
- Enhanced audio engine
- AI-powered tools
- Collaboration features

### Phase 5: User Interface (Weeks 29-36)
- Modern UI framework
- Professional timeline component
- Preview system and scopes

### Phase 6: Polish & Release (Weeks 37-40)
- Testing and optimization
- Documentation
- Beta program
- v1.0 Release

## Technology Stack (Planned)

### Core
- **Language**: C++20
- **Build System**: CMake 3.20+
- **Graphics**: Vulkan (primary), Metal (macOS), DirectX 12 (Windows)
- **UI Framework**: Qt 6.5+ or custom solution
- **Testing**: Google Test / Catch2

### Multimedia
- **Codecs**: Direct FFmpeg integration
- **Audio**: Custom audio engine with professional mixing
- **GPU Acceleration**: CUDA, Metal Compute, Vulkan Compute

### Advanced
- **AI/ML**: ONNX Runtime or TensorFlow Lite
- **Collaboration**: Custom protocol over WebSockets
- **Cloud**: Optional cloud storage integration

## Getting Started

### For Developers
This project is in the early planning/analysis phase. The current focus is on:
1. Studying the Shotcut codebase (in `shotcut-original/`)
2. Finalizing architecture and technology decisions
3. Setting up development infrastructure

### Prerequisites (Future)
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Vulkan SDK (or platform graphics SDK)
- Qt 6.5+ (if using Qt)
- FFmpeg development libraries

### Building (Future)
```bash
# To be defined once source code development begins
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Contributing

This project is in early planning stages. Contribution guidelines will be established once the core architecture is defined and initial implementation begins.

### Areas for Future Contribution
- Core engine development
- Effects and filters
- UI/UX design
- Documentation
- Testing and QA
- Platform-specific optimizations

## Reference: Shotcut

This project studies and draws inspiration from [Shotcut](https://shotcut.org/), an excellent open-source video editor:
- **Repository**: https://github.com/mltframework/shotcut
- **License**: GPLv3
- **Authors**: Dan Dennedy and contributors

**Important**: Shotcut-Pro is being developed as a new project with a clean-room design approach. We study Shotcut's architecture and features for inspiration while building our own proprietary systems.

## License

License to be determined. This project is currently in planning/research phase.

## Roadmap Highlights

### Short Term (Weeks 1-12)
- Complete architecture design
- Build core video engine
- Implement basic timeline functionality
- Set up GPU rendering pipeline

### Medium Term (Weeks 13-28)
- Advanced effects system
- Motion tracking
- AI features
- Professional audio tools
- Collaboration infrastructure

### Long Term (Weeks 29-40)
- Modern UI implementation
- Beta testing program
- Documentation and tutorials
- Version 1.0 release

## Performance Goals

- **Timeline Scrubbing**: 60 FPS on 4K footage
- **Preview Latency**: <100ms
- **Export Speed**: Real-time or faster for HD
- **Memory Usage**: <2GB for typical projects
- **Startup Time**: <3 seconds

## Contact & Support

Project maintained by: [Your Name/Organization]

- **Issues**: Use GitHub Issues for bug reports and feature requests
- **Discussions**: GitHub Discussions for questions and ideas
- **Email**: [To be added]

## Acknowledgments

- Shotcut team for creating an excellent open-source video editor to study
- MLT Framework developers
- FFmpeg project
- Qt Framework team
- The open-source video editing community

---

**Last Updated**: November 17, 2025
**Project Status**: Phase 1 - Foundation & Analysis
**Next Milestone**: Technology stack finalization and build system setup
