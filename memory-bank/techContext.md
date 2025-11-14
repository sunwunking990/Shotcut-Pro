# Shotcut C++ Transformation - Technical Context

## Technology Stack Overview

### Core Technologies

#### Programming Language
- **Primary:** Modern C++23
- **Features:** Modules, coroutines, concepts, ranges, structured bindings
- **Compilers:** GCC 13+, Clang 16+, MSVC 2022+
- **Standards Compliance:** Full C++23 with backward compatibility to C++20

#### Graphics APIs
- **Primary:** Vulkan 1.3+ (cross-platform)
- **Secondary:** DirectX 12 (Windows optimization)
- **Rendering:** Custom 2D rendering engine
- **Shaders:** SPIR-V with compute shader support

#### Hardware Acceleration
- **Video Codecs:** NVIDIA Video Codec SDK 12.2+
- **GPU Compute:** CUDA 12.0+ for effects processing
- **Memory Management:** Zero-copy GPU operations
- **Fallback:** Software codecs for non-NVIDIA hardware

#### Build System
- **Primary:** CMake 3.20+
- **Package Management:** Conan (optional for third-party)
- **CI/CD:** GitHub Actions with multi-platform builds
- **Testing:** Catch2 or Google Test framework

## Development Environment Setup

### Required Tools

#### Development Tools
```bash
# Core development tools
- CMake 3.20+ (build system)
- Git 2.30+ (version control)
- Python 3.10+ (build scripts)
- Node.js 18+ (build tools)

# Compilers by platform
# Windows
- Visual Studio 2022 with C++23 support
- Windows SDK 10.0.22000+

# Linux
- GCC 13+ with full C++23 support
- Clang 16+ (alternative)
- Build-essential package
```

#### SDK Requirements
```bash
# Graphics APIs
- Vulkan SDK 1.3.250+ (mandatory)
- DirectX 12 SDK (Windows only)
- NVIDIA Video Codec SDK 12.2+
- CUDA Toolkit 12.0+

# Media Processing
- FFmpeg development libraries
- FreeType 2.12+ (text rendering)
- HarfBuzz 8.0+ (text shaping)
- PortAudio (audio I/O)

# Security & Validation
- OpenSSL 3.0+ (cryptographic operations)
- Google Test/Sanitizers (testing)
- Valgrind (Linux memory debugging)
```

### Platform-Specific Requirements

#### Windows Development
```cmake
# Windows-specific requirements
find_package(Vulkan REQUIRED)
find_package(DirectX12 REQUIRED)
find_package(NVIDIACodecSDK REQUIRED)

# MSVC security flags
add_compile_options(/W4 /WX)
add_compile_options(/GS /guard:cf /DYNAMICBASE /HIGHENTROPYVA)
```

#### Linux Development
```cmake
# Linux-specific requirements
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil)

# GCC security flags
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-fstack-protector-strong -fcf-protection=full)
```

## Technical Constraints

### Hardware Requirements

#### Minimum Development Hardware
- **CPU:** x86-64 with AVX2 support (Intel i7-10700K / AMD Ryzen 7 3700X)
- **GPU:** NVIDIA GTX 1660+ (for CUDA development)
- **RAM:** 16GB DDR4 (32GB+ recommended)
- **Storage:** 100GB NVMe SSD (500GB+ recommended)
- **OS:** Windows 10 22H2+ or Ubuntu 22.04+

#### Target User Hardware
- **Minimum:** NVIDIA GTX 1060, 16GB RAM, 4-core CPU
- **Recommended:** NVIDIA RTX 4070, 32GB RAM, 8-core CPU
- **Professional:** NVIDIA RTX 4090, 64GB RAM, 16-core CPU

### Performance Constraints

#### Real-Time Requirements
```cpp
// Performance targets for all operations
constexpr int MAX_LATENCY_MS = 16;           // < 16ms for 60fps
constexpr int STARTUP_TIME_MS = 3000;        // < 3 seconds startup
constexpr int SCRUB_RESPONSE_MS = 10;         // < 10ms timeline response
constexpr int EFFECT_PREVIEW_MS = 16;         // < 16ms effect preview
```

#### Memory Constraints
```cpp
// Memory usage limits
constexpr size_t MAX_BASE_MEMORY_MB = 1024;   // Base application memory
constexpr size_t MAX_GPU_MEMORY_USAGE = 0.8;  // 80% of GPU memory
constexpr size_t MAX_CLIP_CACHE_MB = 2048;    // Clip cache limit
constexpr size_t MAX_UNDO_STACK_MB = 512;     // Undo history limit
```

#### File Format Support
```cpp
// Supported codec matrix
enum class SupportedCodecs {
    // Hardware accelerated
    H264_NVDEC, H265_NVDEC, AV1_NVDEC,
    H264_NVENC, H265_NVENC, AV1_NVENC,
    
    // Software fallback
    H264_LIBAV, H265_LIBAV, AV1_LIBAV,
    
    // Professional formats
    PRORES_422, PRORES_4444,
    DNxHD, DNxHR,
    
    // Audio formats
    AAC, AC3, FLAC, ALAC, PCM
};
```

## Dependency Management

### Core Dependencies

#### Essential Libraries
```cmake
# Graphics and Rendering
find_package(Vulkan 1.3 REQUIRED)
find_package(glfw3 REQUIRED)                    # Window management
find_package(freetype REQUIRED)                  # Text rendering
find_package(harfbuzz REQUIRED)                  # Text shaping

# Video Processing
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED 
    libavcodec libavformat libavutil libswscale)

# Audio Processing
find_package(PortAudio REQUIRED)                 # Audio I/O

# Utilities
find_package(Threads REQUIRED)                    # Multi-threading
find_package(Filesystem REQUIRED)                 # File system operations
```

#### Optional Dependencies
```cmake
# Professional features (optional)
option(ENABLE_VST3 "Enable VST3 plugin support" ON)
if(ENABLE_VST3)
    find_package(VST3 SDK)
endif()

option(ENABLE_CUDA "Enable CUDA acceleration" ON)
if(ENABLE_CUDA)
    find_package(CUDA 12.0)
endif()

option(ENABLE_MFX "Enable Intel Quick Sync" OFF)
if(ENABLE_MFX)
    find_package(MFX)
endif()
```

### Third-Party Integration

#### NVIDIA SDK Integration
```cpp
// NVIDIA Video Codec SDK
class NVIDIACodecManager {
    // NVDEC for hardware decoding
    NVDECContext decoder_context_;
    
    // NVENC for hardware encoding
    NVENCContext encoder_context_;
    
    // CUDA interop for effects
    CUDAContext cuda_context_;
    
    // Memory management
    GPUMemoryPool memory_pool_;
    
public:
    Result initialize_decoder(const VideoFormat& format);
    Result initialize_encoder(const EncodeParameters& params);
    Result process_frame_with_effects(GPUFrame& frame);
};
```

#### FFmpeg Integration
```cpp
// FFmpeg integration for codec support
class FFmpegBackend {
    AVFormatContext* format_context_;
    AVCodecContext* codec_context_;
    SwsContext* sws_context_;
    
    // Hardware frame mapping
    AVBufferRef* hw_device_context_;
    AVBufferRef* hw_frame_context_;
    
public:
    Result load_media_file(const std::string& path);
    Result decode_frame_to_gpu(AVFrame* frame, GPUFrame& gpu_frame);
    Result encode_frame_from_gpu(const GPUFrame& frame, AVPacket* packet);
};
```

## Build Configuration

### CMake Configuration

#### Main CMakeLists.txt Structure
```cmake
cmake_minimum_required(VERSION 3.20)
project(ShotcutCPP VERSION 1.0.0 LANGUAGES CXX)

# C++23 Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Security and performance flags
include(cmake/CompilerHardening.cmake)
include(cmake/SecurityFlags.cmake)

# Platform detection
include(cmake/PlatformDetection.cmake)

# Package finding
include(cmake/FindDependencies.cmake)

# Subdirectories
add_subdirectory(src)
add_subdirectory(external)
add_subdirectory(tests)
```

#### Compiler-Specific Configuration
```cmake
# GCC/Clang configuration
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Security hardening
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -fstack-protector-strong
        -fstack-clash-protection
        -fcf-protection=full
        -D_FORTIFY_SOURCE=3
        -fPIE
    )
    
    # Performance optimizations
    add_compile_options(
        -O3 -march=native -mtune=native
        -flto -ffast-math
    )
endif()

# MSVC configuration
if(MSVC)
    # Security hardening
    add_compile_options(
        /W4 /WX
        /GS /guard:cf
        /DYNAMICBASE /HIGHENTROPYVA
        /sdl
    )
    
    # Performance optimizations
    add_compile_options(
        /O2 /Ob2 /Oi /Ot
        /GL
    )
endif()
```

### Build Targets

#### Main Application Target
```cmake
# ShotcutCPP executable
add_executable(ShotcutCPP
    src/main.cpp
    src/core/application.cpp
    src/rendering/vulkan_context.cpp
    src/ui/framework/*.cpp
    src/video/processing/*.cpp
    src/audio/processing/*.cpp
    src/platform/${PLATFORM}/*.cpp
)

# Link libraries
target_link_libraries(ShotcutCPP
    PRIVATE
    Vulkan::Vulkan
    glfw
    freetype
    harfbuzz
    ${FFMPEG_LIBRARIES}
    PortAudio::PortAudio
    Threads::Threads
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(ShotcutCPP PRIVATE d3d12 dxgi)
endif()
```

#### Development Tools
```cmake
# Development and testing targets
add_executable(shotcut_test_runner
    tests/framework/*.cpp
    tests/unit/*.cpp
    tests/integration/*.cpp
)

# Performance benchmarking
add_executable(shotcut_benchmarks
    benchmarks/performance/*.cpp
    benchmarks/memory/*.cpp
    benchmarks/video/*.cpp
)
```

## Testing Strategy

### Test Framework

#### Unit Testing
```cpp
// Google Test based unit tests
#include <gtest/gtest.h>

class VulkanContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        context_ = std::make_unique<VulkanContext>();
    }
    
    void TearDown() override {
        context_.reset();
    }
    
    std::unique_ptr<VulkanContext> context_;
};

TEST_F(VulkanContextTest, InitializationSuccess) {
    EXPECT_TRUE(context_->initialize());
    EXPECT_TRUE(context_->is_valid());
}

TEST_F(VulkanContextTest, MemoryAllocation) {
    ASSERT_TRUE(context_->initialize());
    
    auto buffer = context_->allocate_buffer(1024 * 1024);
    EXPECT_NE(buffer, nullptr);
    EXPECT_GE(buffer->size(), 1024 * 1024);
}
```

#### Integration Testing
```cpp
// Integration tests for component interaction
class VideoPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        pipeline_ = std::make_unique<VideoProcessingPipeline>();
        ASSERT_TRUE(pipeline_->initialize());
    }
    
    std::unique_ptr<VideoProcessingPipeline> pipeline_;
};

TEST_F(VideoPipelineTest, EndToEndProcessing) {
    // Load test video
    auto video_file = load_test_video("test_1080p_h264.mp4");
    ASSERT_NE(video_file, nullptr);
    
    // Process through pipeline
    auto result = pipeline_->process_video(video_file);
    EXPECT_TRUE(result.success);
    
    // Verify output
    EXPECT_EQ(result.output_frame_count, video_file->frame_count);
    EXPECT_EQ(result.output_format, video_file->format);
}
```

#### Performance Testing
```cpp
// Performance benchmarks
class PerformanceBenchmark : public ::benchmark::Fixture {
protected:
    void SetUp(const benchmark::State& state) override {
        pipeline_ = std::make_unique<VideoProcessingPipeline>();
        pipeline_->initialize();
    }
    
    std::unique_ptr<VideoProcessingPipeline> pipeline_;
};

BENCHMARK_DEFINE_F(PerformanceBenchmark, VideoDecoding)(benchmark::State& state) {
    auto test_video = load_test_video("4k_h265.mp4");
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto frame = pipeline_->decode_frame(test_video, state.iteration());
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(duration.count() / 1000000.0);
    }
}

BENCHMARK_REGISTER_F(PerformanceBenchmark, VideoDecoding)->Unit(benchmark::kMicrosecond);
```

### Continuous Integration

#### GitHub Actions Configuration
```yaml
# .github/workflows/build.yml
name: Build and Test

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install Vulkan SDK
        run: choco install vulkan-sdk
      - name: Configure and Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build --config Release
      - name: Run Tests
        run: ./build/tests/shotcut_test_runner.exe

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install Dependencies
        run: |
          sudo apt update
          sudo apt install libvulkan-dev libffmpeg-dev libfreetype6-dev
      - name: Configure and Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build
      - name: Run Tests
        run: ./build/tests/shotcut_test_runner
```

## Debugging and Profiling

### Development Tools

#### Memory Safety
```bash
# AddressSanitizer for memory error detection
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build

# Run with ASAN
ASAN_OPTIONS=detect_leaks=1 ./build/ShotcutCPP

# Valgrind for Linux memory debugging
valgrind --tool=memcheck --leak-check=full ./build/ShotcutCPP
```

#### Performance Profiling
```bash
# GPU profiling with NVIDIA Nsight
nsys profile --trace=cuda,nvtx,osrt ./build/ShotcutCPP

# CPU profiling with perf (Linux)
perf record --call-graph=dwarf ./build/ShotcutCPP
perf report

# Windows ETW tracing
xperf -on Base -stackwalk Profile -f perf.etl
# Run application
xperf -d perf.etl
```

#### Debug Builds
```cmake
# Debug configuration with maximum checking
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON -DENABLE_UBSAN=ON -DENABLE_TSAN=ON

# Release with debug info for profiling
cmake -B build-relwithdebinfo -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## Documentation System

### Code Documentation

#### Doxygen Configuration
```cmake
# Doxygen documentation generation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in 
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()
```

#### Markdown Documentation
```markdown
# Architecture Documentation Structure

docs/
├── api/                    # API documentation (Doxygen)
├── architecture/           # System architecture
├── tutorials/              # Development tutorials
├── performance/            # Performance guides
├── security/               # Security documentation
└── deployment/             # Deployment guides
```

---

**Document Status:** Active  
**Last Updated:** November 13, 2025  
**Next Review:** Technology stack update (Quarterly)  
**Stakeholders:** Development team, DevOps, platform engineers
