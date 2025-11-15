# Shotcut Pro - Critical Issues Checklist

**Generated:** 2025-11-15
**Purpose:** Actionable checklist for fixing identified issues

---

## CRITICAL - Fix Immediately (Cannot Ship Without These)

- [ ] **FIX MEMORY LEAK** - `src/rendering/vulkan_context.cpp:176, 187, 198`
  - Replace `new float{1.0f}` with stack-allocated array or vector
  - Test for leaks with Valgrind/AddressSanitizer

- [ ] **FIX SYNTAX ERROR** - `include/video/export_system.h:87`
  - Change `YouTube Shorts` to `YouTube_Shorts` (remove space)

- [ ] **FIX UNDEFINED VARIABLE** - `src/video/export_system.cpp:797`
  - Define `start_time` variable in `export_with_nvenc()` function scope

- [ ] **FIX BUILD SYSTEM** - `CMakeLists.txt:44-46`
  - Remove hardcoded Windows paths
  - Implement proper pkg-config or find_package for FFmpeg
  - Test build on Linux and macOS

- [ ] **FIX FALSE README CLAIMS** - `README.md:15`
  - Remove "95% Production Ready" claim
  - State actual project status honestly
  - List incomplete features

- [ ] **RENAME PROJECT** or **CLARIFY SCOPE**
  - Either: Integrate Qt + MLT to actually be a Shotcut rewrite
  - Or: Rename to something other than "Shotcut Pro"
  - Document why this is NOT compatible with Shotcut

---

## SEVERE - Fix Before Beta Release

- [ ] **REPLACE CONSOLE OUTPUT** - 751 occurrences across 22 files
  - Integrate logging framework (spdlog recommended)
  - Replace all `std::cout` / `std::cerr` / `printf`
  - Add log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
  - Remove emoji from log messages

- [ ] **ENABLE TESTS** - `CMakeLists.txt:88`
  - Uncomment `add_subdirectory(tests)`
  - Write unit tests for:
    - Vulkan context creation
    - Export system
    - Timeline operations
    - Audio processing
    - FFmpeg wrapper
  - Set up CI/CD pipeline (GitHub Actions recommended)

- [ ] **COMPLETE PLACEHOLDERS** - 64 occurrences
  - Priority placeholders to implement:
    - [ ] JKL navigation (`src/ui/timeline_widget.cpp:267`)
    - [ ] Time parsing (`src/ui/timeline_widget.cpp:511`)
    - [ ] Batch export pause/resume (`src/video/export_system.cpp:433-442`)
    - [ ] Custom preset management (`src/video/export_system.cpp:509-528`)
  - Remove or implement all TODO comments

- [ ] **FIX FFMPEG INTEGRATION** - `src/video/ffmpeg_wrapper.cpp`
  - Remove placeholder implementations
  - Remove `std::this_thread::sleep_for` from decode function (line 241)
  - Implement actual frame decoding
  - Add proper error handling

- [ ] **IMPLEMENT ERROR HANDLING STANDARD**
  - Choose one approach: exceptions, std::expected, or error codes
  - Apply consistently across entire codebase
  - Document error handling strategy

- [ ] **FIX SECURITY VULNERABILITIES**
  - [ ] Add path validation for export output paths
  - [ ] Add buffer size checks before memcpy operations
  - [ ] Validate user inputs
  - [ ] Enable sanitizers in Release builds for security testing

---

## MODERATE - Fix Before Stable Release

- [ ] **FIX RESOURCE CLEANUP** - `src/ui/timeline_widget.cpp:149`
  - Complete Vulkan buffer cleanup in destructor
  - Verify no resource leaks with validation layers

- [ ] **REFACTOR SINGLETON ABUSE**
  - Replace global singletons with dependency injection:
    - [ ] ExportManager
    - [ ] TimelineManager
    - [ ] EffectSystem
    - [ ] AudioProcessor
    - [ ] CodecManager
    - [ ] NVIDIAVideoSDKManager
  - Makes testing possible

- [ ] **SEPARATE CONCERNS IN UI WIDGETS**
  - Move business logic out of Widget classes
  - Create separate Controller classes
  - Move Vulkan resource management to Renderer classes
  - Apply MVC or MVVM pattern

- [ ] **CREATE ABSTRACT INTERFACES**
  - Define `IEncoder` interface for FFmpeg/NVENC
  - Define `IDecoder` interface for hardware/software decoders
  - Define `IRenderer` interface for Vulkan/DirectX/Metal
  - Use dependency injection

- [ ] **ADD API DOCUMENTATION**
  - Set up Doxygen
  - Document all public APIs
  - Add usage examples
  - Create architecture diagrams

- [ ] **FIX INCOMPLETE IMPLEMENTATIONS**
  - [ ] Complete `apply_effects_to_frame()` (export_system.cpp:1132)
  - [ ] Fix `convert_gpu_frame_to_avframe()` for proper YUV formats (export_system.cpp:1190)
  - [ ] Implement `export_audio_track()` properly (export_system.cpp:1151)

- [ ] **ENABLE LTO** - `cmake/CompilerHardening.cmake:73-87`
  - Fix linker issues that caused LTO to be disabled
  - Re-enable Link Time Optimization for Release builds

---

## PLATFORM COMPATIBILITY

- [ ] **LINUX SUPPORT**
  - Test build on Ubuntu 22.04 LTS
  - Test build on Arch Linux
  - Implement VA-API support for Intel GPUs
  - Test Vulkan on AMD and NVIDIA GPUs

- [ ] **MACOS SUPPORT**
  - Test build on macOS 13+
  - Implement Metal backend (claimed but not implemented)
  - Test on both Intel and Apple Silicon Macs
  - Fix any Objective-C++ integration issues

- [ ] **CROSS-PLATFORM CI/CD**
  - Set up GitHub Actions for:
    - Ubuntu 22.04 (GCC 13, Clang 16)
    - Windows Server 2022 (MSVC 2022)
    - macOS 13 (Clang)
  - Run tests on all platforms

---

## CODE QUALITY IMPROVEMENTS

- [ ] **STATIC ANALYSIS**
  - Run clang-tidy with strict warnings
  - Run cppcheck
  - Fix all warnings
  - Integrate into CI/CD

- [ ] **CODE FORMATTING**
  - Choose style guide (Google C++, LLVM, etc.)
  - Set up clang-format
  - Format entire codebase
  - Add pre-commit hook

- [ ] **PERFORMANCE PROFILING**
  - Profile with:
    - Valgrind/Cachegrind (Linux)
    - Instruments (macOS)
    - Visual Studio Profiler (Windows)
  - Identify and fix bottlenecks
  - Verify claims of "<16ms UI latency"

---

## DOCUMENTATION

- [ ] **USER DOCUMENTATION**
  - Write user manual
  - Create video tutorials
  - Document keyboard shortcuts
  - Create FAQ

- [ ] **DEVELOPER DOCUMENTATION**
  - Architecture overview
  - Build instructions for all platforms
  - Contributing guidelines
  - Code style guide
  - Plugin development guide (if applicable)

- [ ] **PROJECT MANAGEMENT**
  - Create realistic roadmap
  - Set up GitHub issues for tracking
  - Define release milestones
  - Establish semantic versioning

---

## TESTING STRATEGY

### Unit Tests Needed
- [ ] VulkanContext initialization and cleanup
- [ ] Export system configuration validation
- [ ] Timeline clip management
- [ ] Audio buffer operations
- [ ] FFmpeg codec detection
- [ ] File format validation

### Integration Tests Needed
- [ ] Full video export pipeline
- [ ] Timeline to renderer integration
- [ ] Effects application
- [ ] Audio-video synchronization

### System Tests Needed
- [ ] Import various video formats
- [ ] Export to all supported formats
- [ ] 4K and 8K workflow testing
- [ ] Memory usage under sustained load
- [ ] GPU acceleration validation

### Performance Tests Needed
- [ ] Frame rate stability
- [ ] Memory leak detection
- [ ] Startup time
- [ ] Export speed benchmarks

---

## DEFINITION OF DONE

A feature/fix is considered "DONE" when:

1. ✅ Code is written and compiles without warnings
2. ✅ Unit tests written and passing
3. ✅ Integration tests passing
4. ✅ Code reviewed by at least one other developer
5. ✅ Documentation updated
6. ✅ No memory leaks (verified with sanitizers)
7. ✅ Works on Windows, Linux, and macOS
8. ✅ Performance meets stated requirements
9. ✅ Security review completed (for security-sensitive code)

---

## Progress Tracking

**Total Critical Issues:** 6
**Total Severe Issues:** 6
**Total Moderate Issues:** 6
**Total Platform Issues:** 3

**Fixed:** 0 / 21
**In Progress:** 0 / 21
**Not Started:** 21 / 21

**Completion Percentage:** 0%
**Estimated Effort:** 12-18 months with dedicated team

---

## Priority Matrix

| Priority | Issue | Impact | Effort | Order |
|----------|-------|--------|--------|-------|
| P0 | Memory leak | High | Low | 1 |
| P0 | Syntax error | High | Low | 2 |
| P0 | Undefined variable | High | Low | 3 |
| P0 | Build system | High | Medium | 4 |
| P0 | False claims | Medium | Low | 5 |
| P0 | Project identity | Very High | Very High | 6 |
| P1 | Console output | High | Medium | 7 |
| P1 | Enable tests | High | Low | 8 |
| P1 | Placeholders | Very High | High | 9 |
| P1 | FFmpeg integration | Very High | High | 10 |

---

**Last Updated:** 2025-11-15
**Next Review:** After first 10 issues are resolved
