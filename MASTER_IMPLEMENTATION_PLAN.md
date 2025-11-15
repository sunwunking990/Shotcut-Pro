# Shotcut Pro - Master Implementation Plan
**Created:** 2025-11-15
**Purpose:** Systematic, phase-based approach to fixing all identified issues
**Estimated Timeline:** 12-18 months to production-ready
**Immediate Goal:** Working demo in 2.5-3.5 workdays

---

## Table of Contents

1. [Strategic Overview](#strategic-overview)
2. [Phase-Based Implementation](#phase-based-implementation)
3. [Work Session Structure](#work-session-structure)
4. [Detailed Phase Breakdown](#detailed-phase-breakdown)
5. [Testing & Validation Strategy](#testing--validation-strategy)
6. [Risk Management](#risk-management)
7. [Progress Tracking](#progress-tracking)

---

## Strategic Overview

### Core Philosophy

**Fix in layers, validate continuously, build incrementally.**

We will organize fixes into **logical phases** where each phase:
1. Has clear entry/exit criteria
2. Groups related issues together
3. Validates with working demos
4. Builds on previous phase

### Critical Success Factors

✅ **Every phase must produce a working demo**
✅ **Fix compilation blockers before features**
✅ **Test infrastructure before implementation**
✅ **Validate dependencies before building on them**

---

## Phase-Based Implementation

### Overview of Phases

```
PHASE 0: Pre-Work Assessment (Current)
         ↓
PHASE 1: Build System Foundation [WEEK 1]
         ├── Fix compilation blockers
         ├── Configure external dependencies
         └── Demo: minimal_headless_demo
         ↓
PHASE 2: Core Infrastructure [WEEK 2-3]
         ├── Memory management fixes
         ├── Namespace standardization
         ├── Vulkan initialization
         └── Demo: vulkan_init_test
         ↓
PHASE 3: Rendering Stack [WEEK 4-5]
         ├── Rendering pipeline
         ├── Window management
         ├── Basic UI primitives
         └── Demo: simple_window_demo
         ↓
PHASE 4: Logging & Error Handling [WEEK 6-7]
         ├── Replace console output
         ├── Implement logging framework
         ├── Standardize error handling
         └── Demo: Updated demos with logging
         ↓
PHASE 5: Testing Infrastructure [WEEK 8-9]
         ├── Enable test framework
         ├── Write unit tests
         ├── Set up CI/CD
         └── Demo: Test suite passing
         ↓
PHASE 6: Video Processing Core [WEEK 10-12]
         ├── FFmpeg integration
         ├── Frame buffer management
         ├── Decode/encode pipeline
         └── Demo: video_processing_demo
         ↓
PHASE 7: Timeline System [WEEK 13-15]
         ├── Clip management
         ├── Track operations
         ├── Playback control
         └── Demo: timeline_demo
         ↓
PHASE 8: Effects & Audio [WEEK 16-18]
         ├── Effects pipeline
         ├── Audio processing
         ├── Real-time preview
         └── Demo: effects_audio_demo
         ↓
PHASE 9: Export System [WEEK 19-21]
         ├── Export configuration
         ├── Batch processing
         ├── Hardware acceleration
         └── Demo: export_demo
         ↓
PHASE 10: Platform Compatibility [WEEK 22-24]
         ├── Linux testing
         ├── macOS testing
         ├── Cross-platform CI/CD
         └── Demo: All platforms working
         ↓
PHASE 11: Security & Hardening [WEEK 25-27]
         ├── Input validation
         ├── Security audit
         ├── Sanitizer testing
         └── Demo: Security test suite
         ↓
PHASE 12: Polish & Documentation [WEEK 28-30]
         ├── Code cleanup
         ├── API documentation
         ├── User documentation
         └── Demo: Full application
```

---

## Work Session Structure

### Session Template

Each work session follows this structure:

```
┌─────────────────────────────────────────┐
│ WORK SESSION: [Name]                    │
│ Phase: [#] | Duration: [X hours]        │
└─────────────────────────────────────────┘

1. PRE-SESSION SETUP (10 min)
   - Review objectives
   - Verify dependencies ready
   - Check environment

2. IMPLEMENTATION (70-80% of time)
   - Fix issues in priority order
   - Document changes
   - Handle blockers

3. TESTING & VALIDATION (15-20% of time)
   - Run tests
   - Verify demo works
   - Check for regressions

4. SESSION RETROSPECTIVE (10 min)
   - Document what worked
   - Note blockers encountered
   - Plan next session
```

### Session Groupings

Sessions are grouped by **system dependencies**, not arbitrary categories:

- **Build System Sessions**: All CMake, dependency, compilation issues
- **Memory Management Sessions**: All memory leaks, RAII, resource cleanup
- **Rendering Sessions**: All Vulkan, graphics pipeline, UI rendering
- **Integration Sessions**: Connecting multiple systems together

---

## Detailed Phase Breakdown

---

### PHASE 0: Pre-Work Assessment ✓ COMPLETE

**Status:** ✅ DONE
**Duration:** Completed
**Output:**
- Critical Analysis Report
- Issues Checklist
- Master Implementation Plan (this document)

---

### PHASE 1: Build System Foundation [WEEK 1]

**Goal:** Get codebase to compile on primary development platform
**Duration:** 19-27 hours (2.5-3.5 workdays)
**Success Criteria:** `minimal_headless_demo` compiles and runs

#### Session 1.1: External Dependencies Setup (3-4 hours)

**Objective:** Install and configure all required external libraries

**Issues Addressed:**
- Missing GLM library
- GLFW availability
- FFmpeg detection

**Tasks:**

1. **Install GLM** (1 hour)
   ```bash
   # Linux
   sudo apt install libglm-dev

   # macOS
   brew install glm

   # Windows
   vcpkg install glm:x64-windows
   ```

   - Update `CMakeLists.txt` to find GLM properly
   - Add: `find_package(glm REQUIRED)`

2. **Configure GLFW** (30 min)
   ```bash
   # Linux
   sudo apt install libglfw3-dev

   # macOS
   brew install glfw

   # Windows
   vcpkg install glfw3:x64-windows
   ```

   - Make GLFW optional in UI module
   - Update `src/ui/CMakeLists.txt`

3. **Fix FFmpeg Detection** (1.5 hours)

   **File:** `CMakeLists.txt:44-46`

   **Replace:**
   ```cmake
   set(FFMPEG_INCLUDE_DIRS "C:/ffmpeg/include")
   set(FFMPEG_LIBRARIES "C:/ffmpeg/lib/avcodec.lib;...")
   set(FFMPEG_FOUND TRUE)
   ```

   **With:**
   ```cmake
   find_package(PkgConfig)
   if(PkgConfig_FOUND)
       pkg_check_modules(FFMPEG
           libavcodec>=58.0
           libavformat>=58.0
           libavutil>=56.0
           libswscale>=5.0
       )
   endif()

   if(FFMPEG_FOUND)
       message(STATUS "FFmpeg found: ${FFMPEG_VERSION}")
       add_compile_definitions(HAVE_FFMPEG=1)
   else()
       message(WARNING "FFmpeg not found - using mock implementation")
       add_compile_definitions(HAVE_FFMPEG=0)
   endif()
   ```

4. **Test Build Configuration** (30 min)
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   # Should complete without errors
   ```

**Validation:**
- CMake configuration completes without errors
- All required packages found or gracefully degraded
- No hardcoded paths remain

**Files Modified:**
- `CMakeLists.txt`
- `src/ui/CMakeLists.txt`

---

#### Session 1.2: Namespace Standardization (2-3 hours)

**Objective:** Fix namespace inconsistencies across entire codebase

**Issues Addressed:**
- Mixed `shotcut` vs `Shotcut` usage
- Compilation errors from name lookup failures

**Current State Analysis:**

| Namespace | Files Using It | Correct? |
|-----------|---------------|----------|
| `shotcut` (lowercase) | application.h, platform.h | ❌ Inconsistent |
| `Shotcut::Video` | video/*.cpp, video/*.h | ✅ Standard |
| `Shotcut::UI` | ui/*.cpp, ui/*.h | ✅ Standard |
| `Shotcut::Audio` | audio/*.cpp, audio/*.h | ✅ Standard |
| `Shotcut::Effects` | effects/*.cpp, effects/*.h | ✅ Standard |
| `Shotcut::Timeline` | timeline/*.cpp, timeline/*.h | ✅ Standard |

**Decision:** Standardize on **`Shotcut::`** (capitalized) everywhere

**Tasks:**

1. **Update Core Module** (30 min)

   **File:** `src/core/application.h:6`
   ```cpp
   // BEFORE
   namespace shotcut {

   // AFTER
   namespace Shotcut::Core {
   ```

   **Files to update:**
   - `src/core/application.h`
   - `src/core/application.cpp`
   - `src/main.cpp`

2. **Update Platform Module** (30 min)

   **File:** `include/platform/platform.h:6`
   ```cpp
   // BEFORE
   namespace shotcut::platform {

   // AFTER
   namespace Shotcut::Platform {
   ```

   **Files to update:**
   - `include/platform/platform.h`
   - `src/platform/platform.cpp`

3. **Update Demo Files** (1 hour)

   All demo files currently use mixed namespaces:
   - `src/simple_working_demo.cpp`
   - `src/ui_demo.cpp`
   - `src/video_demo.cpp`
   - `src/effects_demo.cpp`
   - `src/timeline_demo.cpp`

   Update all references to use `Shotcut::` consistently

4. **Verify with Grep** (30 min)
   ```bash
   # Should find ZERO results
   grep -r "namespace shotcut" src/ include/

   # Should find ALL namespace declarations
   grep -r "namespace Shotcut" src/ include/
   ```

**Validation:**
- No mixed namespace usage remains
- Code still compiles
- All modules use consistent naming

**Files Modified:** ~20 files across all modules

---

#### Session 1.3: Fix Class Definition Errors (1-2 hours)

**Objective:** Fix missing class definitions in demo files

**Issues Addressed:**
- `Shotcut::Platform` class doesn't exist (only free functions)
- `Shotcut::Video::VideoSystem` class doesn't exist (only free functions)

**Current State:**

**File:** `src/simple_working_demo.cpp:27-31`
```cpp
// WRONG - These classes don't exist!
auto platform = std::make_unique<Shotcut::Platform>();
auto video_system = std::make_unique<Shotcut::Video::VideoSystem>();
```

**Strategy:** Two options:

**Option A:** Create the missing classes (more work, better architecture)
**Option B:** Rewrite demos to use free functions (faster, matches current implementation)

**Recommendation:** Option B for Phase 1 (faster), defer proper class design to Phase 2

**Tasks:**

1. **Update simple_working_demo.cpp** (30 min)

   ```cpp
   // AFTER
   if (!Shotcut::Platform::initialize()) {
       std::cerr << "Failed to initialize platform" << std::endl;
       return -1;
   }

   if (!Shotcut::Video::initialize()) {
       std::cerr << "Failed to initialize video system" << std::endl;
       return -1;
   }
   ```

2. **Update all other demos** (30 min)
   - `src/video_demo.cpp`
   - `src/ui_demo.cpp`
   - `src/effects_demo.cpp`

   Same pattern: replace object construction with function calls

3. **Test Compilation** (30 min)
   ```bash
   cd build
   cmake --build . --target simple_working_demo
   cmake --build . --target video_demo
   ```

**Validation:**
- All demos compile without errors
- Demos use correct API (free functions, not non-existent classes)

**Files Modified:**
- `src/simple_working_demo.cpp`
- `src/video_demo.cpp`
- `src/ui_demo.cpp`
- `src/effects_demo.cpp`

**Future Work:** Consider creating proper manager classes in Phase 2

---

#### Session 1.4: Fix CMake Security Functions (30 min)

**Objective:** Ensure security hardening functions are accessible

**Issues Addressed:**
- `apply_default_hardening()` not found
- `configure_security()` not found

**Tasks:**

1. **Verify Include Paths** (15 min)

   **File:** `CMakeLists.txt:40-41`
   ```cmake
   # Current
   include(cmake/CompilerHardening.cmake)
   include(cmake/SecurityFlags.cmake)
   ```

   Verify these files exist and are loaded before subdirectories

2. **Test Function Availability** (15 min)

   Create test CMakeLists.txt:
   ```cmake
   message(STATUS "Testing security functions...")

   if(COMMAND apply_default_hardening)
       message(STATUS "✓ apply_default_hardening found")
   else()
       message(FATAL_ERROR "✗ apply_default_hardening NOT FOUND")
   endif()
   ```

   If functions not found, add proper guards:
   ```cmake
   if(COMMAND apply_default_hardening)
       apply_default_hardening(${target})
   endif()
   ```

**Validation:**
- CMake configure succeeds
- Functions are available to all subdirectories
- No "command not found" errors

**Files Modified:**
- `CMakeLists.txt` (potentially)
- All module `CMakeLists.txt` (add guards if needed)

---

#### Session 1.5: Create Minimal Headless Demo (2-3 hours)

**Objective:** Create first working executable to validate build system

**Issues Addressed:**
- No simple demo that works without GPU/window
- Need baseline validation

**Tasks:**

1. **Create New Demo File** (1 hour)

   **File:** `src/minimal_headless_demo.cpp`

   ```cpp
   #include <iostream>
   #include "platform/platform.h"
   #include "utils/utils.h"
   #include "audio/audio_processor.h"

   int main(int argc, char* argv[]) {
       std::cout << "=== Shotcut Pro - Minimal Headless Demo ===" << std::endl;

       // Initialize platform
       std::cout << "\n[1/4] Initializing platform..." << std::endl;
       if (!Shotcut::Platform::initialize()) {
           std::cerr << "ERROR: Platform initialization failed" << std::endl;
           return -1;
       }
       std::cout << "✓ Platform initialized" << std::endl;

       // Query system info
       std::cout << "\n[2/4] Querying system capabilities..." << std::endl;
       auto platform_info = Shotcut::Platform::get_platform_info();
       std::cout << "  OS: " << platform_info.os_name << std::endl;
       std::cout << "  CPU Cores: " << platform_info.cpu_cores << std::endl;
       std::cout << "  RAM: " << platform_info.total_memory_mb << " MB" << std::endl;

       // Initialize audio system
       std::cout << "\n[3/4] Initializing audio system..." << std::endl;
       auto& audio_processor = Shotcut::Audio::AudioProcessor::get();
       if (!audio_processor.initialize(48000, 2)) {
           std::cerr << "ERROR: Audio initialization failed" << std::endl;
           return -1;
       }
       std::cout << "✓ Audio system initialized" << std::endl;

       // List available devices
       std::cout << "\n[4/4] Available audio devices:" << std::endl;
       auto devices = audio_processor.get_available_devices();
       for (const auto& device : devices) {
           std::cout << "  - " << device.name << std::endl;
       }

       std::cout << "\n=== Demo completed successfully ===" << std::endl;

       audio_processor.cleanup();
       Shotcut::Platform::shutdown();

       return 0;
   }
   ```

2. **Add CMake Target** (30 min)

   **File:** `CMakeLists.txt` (add before installation section)

   ```cmake
   # Demo applications
   add_executable(minimal_headless_demo src/minimal_headless_demo.cpp)
   target_link_libraries(minimal_headless_demo PRIVATE
       shotcut_platform
       shotcut_utils
       shotcut_audio
   )
   apply_default_hardening(minimal_headless_demo)
   ```

3. **Build and Test** (30 min)
   ```bash
   cd build
   cmake --build . --target minimal_headless_demo
   ./minimal_headless_demo
   ```

   Expected output:
   ```
   === Shotcut Pro - Minimal Headless Demo ===

   [1/4] Initializing platform...
   ✓ Platform initialized

   [2/4] Querying system capabilities...
     OS: Linux
     CPU Cores: 8
     RAM: 16384 MB

   [3/4] Initializing audio system...
   ✓ Audio system initialized

   [4/4] Available audio devices:
     - Default Audio Device

   === Demo completed successfully ===
   ```

4. **Document Success** (30 min)

   Create `docs/BUILD_VALIDATION.md`:
   ```markdown
   # Build Validation

   ## Minimal Headless Demo

   **Purpose:** Validate build system without GPU/window dependencies

   **How to run:**
   ```bash
   cd build
   ./minimal_headless_demo
   ```

   **Expected behavior:** Should print system info and exit cleanly

   **If it fails:** Check that platform and audio modules compiled correctly
   ```

**Validation:**
- Demo compiles successfully
- Demo runs without crashes
- All systems initialize properly
- Clean shutdown with no memory leaks

**Success Criteria for PHASE 1:**
✅ All code compiles without errors
✅ External dependencies properly configured
✅ Namespaces consistent throughout
✅ `minimal_headless_demo` runs successfully
✅ No memory leaks in demo (verify with valgrind)

**Files Created:**
- `src/minimal_headless_demo.cpp`
- `docs/BUILD_VALIDATION.md`

**Files Modified:**
- `CMakeLists.txt` (root)
- `src/ui/CMakeLists.txt`
- ~20 files for namespace fixes

**Deliverable:** Working build system + first executable demo

---

### PHASE 2: Core Infrastructure [WEEK 2-3]

**Goal:** Fix critical memory issues and establish Vulkan foundation
**Duration:** 40-50 hours (1-1.5 weeks)
**Success Criteria:** `vulkan_init_test` runs without errors or leaks

#### Session 2.1: Fix Memory Leaks in Vulkan Context (2 hours)

**Objective:** Eliminate all memory leaks in Vulkan initialization

**Issues Addressed:**
- CRITICAL: Memory leak in vulkan_context.cpp:176, 187, 198

**Current Code:**

**File:** `src/rendering/vulkan_context.cpp:165-240`

```cpp
// Lines 176, 187, 198 - MEMORY LEAKS!
VkDeviceQueueCreateInfo queue_create_infos[4];

// Graphics queue
queue_info.pQueuePriorities = new float{1.0f};  // LEAK!

// Present queue
queue_info.pQueuePriorities = new float{1.0f};  // LEAK!

// Compute queue
queue_info.pQueuePriorities = new float{1.0f};  // LEAK!
```

**Tasks:**

1. **Fix Queue Priority Allocations** (1 hour)

   **Replace with:**
   ```cpp
   bool VulkanContext::create_logical_device() {
       // Stack-allocated priorities (no leak possible)
       std::vector<float> queue_priorities = {1.0f};

       VkDeviceQueueCreateInfo queue_create_infos[4];
       uint32_t queue_create_count = 0;
       std::set<uint32_t> unique_queue_families;

       // Graphics queue
       if (queue_families_.graphics_family.has_value()) {
           VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
           queue_info = {};  // Zero initialize
           queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
           queue_info.queueFamilyIndex = *queue_families_.graphics_family;
           queue_info.queueCount = 1;
           queue_info.pQueuePriorities = queue_priorities.data();  // No allocation!
           unique_queue_families.insert(*queue_families_.graphics_family);
       }

       // Present queue
       if (queue_families_.present_family.has_value() &&
           unique_queue_families.find(*queue_families_.present_family) == unique_queue_families.end()) {
           VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
           queue_info = {};  // Zero initialize
           queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
           queue_info.queueFamilyIndex = *queue_families_.present_family;
           queue_info.queueCount = 1;
           queue_info.pQueuePriorities = queue_priorities.data();  // No allocation!
           unique_queue_families.insert(*queue_families_.present_family);
       }

       // Compute queue
       if (queue_families_.compute_family.has_value() &&
           unique_queue_families.find(*queue_families_.compute_family) == unique_queue_families.end()) {
           VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
           queue_info = {};  // Zero initialize
           queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
           queue_info.queueFamilyIndex = *queue_families_.compute_family;
           queue_info.queueCount = 1;
           queue_info.pQueuePriorities = queue_priorities.data();  // No allocation!
           unique_queue_families.insert(*queue_families_.compute_family);
       }

       // Rest of function continues...
   }
   ```

2. **Test for Leaks with Valgrind** (30 min)

   ```bash
   # Build with debug symbols
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . --target vulkan_init_test

   # Run with valgrind
   valgrind --leak-check=full --show-leak-kinds=all ./vulkan_init_test
   ```

   Expected output:
   ```
   ==12345== LEAK SUMMARY:
   ==12345==    definitely lost: 0 bytes in 0 blocks
   ==12345==    indirectly lost: 0 bytes in 0 blocks
   ```

3. **Test with AddressSanitizer** (30 min)

   ```bash
   # Build with ASan
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
   cmake --build . --target vulkan_init_test
   ./vulkan_init_test
   ```

   Should complete without any ASan errors

**Validation:**
- Valgrind reports ZERO memory leaks
- AddressSanitizer reports no errors
- Vulkan initialization still works correctly

**Files Modified:**
- `src/rendering/vulkan_context.cpp`

---

#### Session 2.2: Fix Timeline Widget Resource Cleanup (1 hour)

**Objective:** Complete Vulkan resource cleanup in destructor

**Issues Addressed:**
- Incomplete cleanup comment in timeline_widget.cpp:149

**Current Code:**

**File:** `src/ui/timeline_widget.cpp:142-150`

```cpp
TimelineWidget::~TimelineWidget() {
    // Cleanup Vulkan resources
    if (track_vertex_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkan_context_->device(), track_vertex_buffer_, nullptr);
    }
    if (track_vertex_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan_context_->device(), track_vertex_memory_, nullptr);
    }
    // ... cleanup other buffers  // <-- TODO: Actually implement!
}
```

**Tasks:**

1. **Identify All Vulkan Resources** (15 min)

   Search timeline_widget.h for all VkBuffer, VkDeviceMemory, VkImage members:
   ```bash
   grep -E "Vk(Buffer|DeviceMemory|Image|ImageView)" include/ui/timeline_widget.h
   ```

2. **Implement Complete Cleanup** (30 min)

   ```cpp
   TimelineWidget::~TimelineWidget() {
       if (!vulkan_context_) {
           return;  // Already cleaned or never initialized
       }

       VkDevice device = vulkan_context_->device();

       // Wait for device to be idle before cleanup
       vkDeviceWaitIdle(device);

       // Cleanup track rendering resources
       if (track_vertex_buffer_ != VK_NULL_HANDLE) {
           vkDestroyBuffer(device, track_vertex_buffer_, nullptr);
           track_vertex_buffer_ = VK_NULL_HANDLE;
       }
       if (track_vertex_memory_ != VK_NULL_HANDLE) {
           vkFreeMemory(device, track_vertex_memory_, nullptr);
           track_vertex_memory_ = VK_NULL_HANDLE;
       }

       // Cleanup clip rendering resources
       if (clip_vertex_buffer_ != VK_NULL_HANDLE) {
           vkDestroyBuffer(device, clip_vertex_buffer_, nullptr);
           clip_vertex_buffer_ = VK_NULL_HANDLE;
       }
       if (clip_vertex_memory_ != VK_NULL_HANDLE) {
           vkFreeMemory(device, clip_vertex_memory_, nullptr);
           clip_vertex_memory_ = VK_NULL_HANDLE;
       }

       // Cleanup playhead rendering resources
       if (playhead_vertex_buffer_ != VK_NULL_HANDLE) {
           vkDestroyBuffer(device, playhead_vertex_buffer_, nullptr);
           playhead_vertex_buffer_ = VK_NULL_HANDLE;
       }
       if (playhead_vertex_memory_ != VK_NULL_HANDLE) {
           vkFreeMemory(device, playhead_vertex_memory_, nullptr);
           playhead_vertex_memory_ = VK_NULL_HANDLE;
       }

       // Cleanup marker rendering resources
       if (marker_vertex_buffer_ != VK_NULL_HANDLE) {
           vkDestroyBuffer(device, marker_vertex_buffer_, nullptr);
           marker_vertex_buffer_ = VK_NULL_HANDLE;
       }
       if (marker_vertex_memory_ != VK_NULL_HANDLE) {
           vkFreeMemory(device, marker_vertex_memory_, nullptr);
           marker_vertex_memory_ = VK_NULL_HANDLE;
       }

       // Cleanup ruler rendering resources
       if (ruler_vertex_buffer_ != VK_NULL_HANDLE) {
           vkDestroyBuffer(device, ruler_vertex_buffer_, nullptr);
           ruler_vertex_buffer_ = VK_NULL_HANDLE;
       }
       if (ruler_vertex_memory_ != VK_NULL_HANDLE) {
           vkFreeMemory(device, ruler_vertex_memory_, nullptr);
           ruler_vertex_memory_ = VK_NULL_HANDLE;
       }
   }
   ```

3. **Add Validation Layer Checks** (15 min)

   Run with Vulkan validation layers enabled:
   ```bash
   export VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d
   VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./timeline_demo
   ```

   Should see no "Object leak" warnings

**Validation:**
- No Vulkan validation errors about leaked objects
- Valgrind shows no leaks related to Vulkan memory
- Widget can be created and destroyed multiple times without issues

**Files Modified:**
- `src/ui/timeline_widget.cpp`
- `include/ui/timeline_widget.h` (potentially, to add missing members)

---

#### Session 2.3: Fix Export System Compilation Issues (1.5 hours)

**Objective:** Fix undefined variables and syntax errors in export system

**Issues Addressed:**
- Undefined variable `start_time` in export_system.cpp:797
- Syntax error `YouTube Shorts` in export_system.h:87

**Tasks:**

1. **Fix Syntax Error in Header** (15 min)

   **File:** `include/video/export_system.h:87`

   ```cpp
   // BEFORE - SYNTAX ERROR!
   YouTube Shorts,       // Vertical 9:16 format

   // AFTER
   YouTube_Shorts,       // Vertical 9:16 format
   ```

   Also check for any code referencing the old name and update it

2. **Fix Undefined Variable in NVENC Export** (30 min)

   **File:** `src/video/export_system.cpp:762-807`

   ```cpp
   bool ExportSystem::export_with_nvenc(const ExportConfig& config) {
       if (!current_session_->nvenc_encoder) {
           return false;
       }

       // ADD THIS - Define start_time!
       auto start_time = std::chrono::steady_clock::now();

       // Initialize NVENC encoder
       NVENCConfig nvenc_config = NVENCConfig::create_default();
       // ... rest of function
   ```

3. **Search for Similar Issues** (45 min)

   Search entire export_system.cpp for other undefined variables:
   ```bash
   # Build and look for undefined variable errors
   cd build
   cmake --build . --target shotcut_video 2>&1 | grep "was not declared"
   ```

   Fix any additional issues found

**Validation:**
- export_system.h compiles without syntax errors
- export_system.cpp compiles without undefined variable errors
- shotcut_video module builds successfully

**Files Modified:**
- `include/video/export_system.h`
- `src/video/export_system.cpp`

---

#### Session 2.4: Create Vulkan Initialization Test (3-4 hours)

**Objective:** Create comprehensive Vulkan testing demo

**Issues Addressed:**
- Need to validate Vulkan works before building UI on top of it

**Tasks:**

1. **Create Test Application** (2 hours)

   **File:** `src/vulkan_init_test.cpp`

   ```cpp
   #include <iostream>
   #include <iomanip>
   #include "rendering/vulkan_context.h"

   void print_separator() {
       std::cout << std::string(60, '=') << std::endl;
   }

   void print_section(const std::string& title) {
       std::cout << "\n";
       print_separator();
       std::cout << "  " << title << std::endl;
       print_separator();
   }

   int main() {
       std::cout << "Shotcut Pro - Vulkan Initialization Test" << std::endl;

       try {
           print_section("Step 1: Creating Vulkan Context");
           auto vulkan_context = std::make_unique<VulkanContext>(true);
           std::cout << "✓ VulkanContext created (validation layers enabled)" << std::endl;

           print_section("Step 2: Initializing Vulkan (Headless)");
           if (!vulkan_context->initialize(nullptr)) {
               std::cerr << "✗ Failed to initialize Vulkan context" << std::endl;
               return -1;
           }
           std::cout << "✓ Vulkan context initialized" << std::endl;

           print_section("Step 3: Querying Physical Device");
           VkPhysicalDevice physical_device = vulkan_context->physical_device();

           VkPhysicalDeviceProperties properties;
           vkGetPhysicalDeviceProperties(physical_device, &properties);

           std::cout << "GPU Information:" << std::endl;
           std::cout << "  Device Name: " << properties.deviceName << std::endl;
           std::cout << "  Device Type: ";
           switch (properties.deviceType) {
               case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                   std::cout << "Discrete GPU"; break;
               case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                   std::cout << "Integrated GPU"; break;
               case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                   std::cout << "Virtual GPU"; break;
               case VK_PHYSICAL_DEVICE_TYPE_CPU:
                   std::cout << "CPU"; break;
               default:
                   std::cout << "Other";
           }
           std::cout << std::endl;

           std::cout << "  Vulkan API Version: "
                     << VK_VERSION_MAJOR(properties.apiVersion) << "."
                     << VK_VERSION_MINOR(properties.apiVersion) << "."
                     << VK_VERSION_PATCH(properties.apiVersion) << std::endl;

           std::cout << "  Driver Version: " << properties.driverVersion << std::endl;

           print_section("Step 4: Querying Device Limits");
           std::cout << "Maximum Limits:" << std::endl;
           std::cout << "  Max Image Dimension 2D: "
                     << properties.limits.maxImageDimension2D << std::endl;
           std::cout << "  Max Framebuffer Width: "
                     << properties.limits.maxFramebufferWidth << std::endl;
           std::cout << "  Max Framebuffer Height: "
                     << properties.limits.maxFramebufferHeight << std::endl;
           std::cout << "  Max Viewports: "
                     << properties.limits.maxViewports << std::endl;

           print_section("Step 5: Querying Device Features");
           VkPhysicalDeviceFeatures features;
           vkGetPhysicalDeviceFeatures(physical_device, &features);

           std::cout << "Supported Features:" << std::endl;
           std::cout << "  Geometry Shader: "
                     << (features.geometryShader ? "YES" : "NO") << std::endl;
           std::cout << "  Tessellation Shader: "
                     << (features.tessellationShader ? "YES" : "NO") << std::endl;
           std::cout << "  Sampler Anisotropy: "
                     << (features.samplerAnisotropy ? "YES" : "NO") << std::endl;
           std::cout << "  Independent Blend: "
                     << (features.independentBlend ? "YES" : "NO") << std::endl;

           print_section("Step 6: Querying Memory Properties");
           VkPhysicalDeviceMemoryProperties memory_properties;
           vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

           std::cout << "Memory Heaps: " << memory_properties.memoryHeapCount << std::endl;
           for (uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i) {
               std::cout << "  Heap " << i << ": "
                         << (memory_properties.memoryHeaps[i].size / (1024 * 1024))
                         << " MB";
               if (memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                   std::cout << " (Device Local)";
               }
               std::cout << std::endl;
           }

           print_section("Step 7: Querying Queue Families");
           auto queue_families = vulkan_context->queue_families();
           std::cout << "Queue Family Indices:" << std::endl;
           if (queue_families.graphics_family.has_value()) {
               std::cout << "  Graphics Queue: " << *queue_families.graphics_family << std::endl;
           }
           if (queue_families.compute_family.has_value()) {
               std::cout << "  Compute Queue: " << *queue_families.compute_family << std::endl;
           }
           if (queue_families.transfer_family.has_value()) {
               std::cout << "  Transfer Queue: " << *queue_families.transfer_family << std::endl;
           }
           if (queue_families.present_family.has_value()) {
               std::cout << "  Present Queue: " << *queue_families.present_family << std::endl;
           }

           print_section("Step 8: Cleanup");
           vulkan_context->cleanup();
           std::cout << "✓ Vulkan context cleaned up" << std::endl;

           print_section("TEST PASSED");
           std::cout << "All Vulkan initialization steps completed successfully!" << std::endl;
           std::cout << "GPU is compatible with Shotcut Pro rendering requirements." << std::endl;

           return 0;

       } catch (const std::exception& e) {
           std::cerr << "\n✗ EXCEPTION: " << e.what() << std::endl;
           return -1;
       }
   }
   ```

2. **Add CMake Target** (30 min)

   **File:** `CMakeLists.txt`

   ```cmake
   # Vulkan initialization test
   add_executable(vulkan_init_test src/vulkan_init_test.cpp)
   target_link_libraries(vulkan_init_test PRIVATE
       shotcut_core
       shotcut_rendering
       Vulkan::Vulkan
   )
   apply_default_hardening(vulkan_init_test)
   ```

3. **Build and Test** (1 hour)

   ```bash
   cd build
   cmake --build . --target vulkan_init_test

   # Run test
   ./vulkan_init_test

   # Run with validation layers
   VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./vulkan_init_test

   # Check for memory leaks
   valgrind --leak-check=full ./vulkan_init_test
   ```

4. **Document Requirements** (30 min)

   Update `docs/BUILD_VALIDATION.md`:
   ```markdown
   ## Vulkan Initialization Test

   **Purpose:** Validate Vulkan is properly configured and GPU is compatible

   **Requirements:**
   - Vulkan SDK 1.3+ installed
   - Compatible GPU (NVIDIA, AMD, or Intel with Vulkan support)
   - Vulkan validation layers (optional but recommended)

   **How to run:**
   ```bash
   cd build
   ./vulkan_init_test
   ```

   **Expected output:** Should display GPU information and complete all 8 steps

   **If it fails:**
   - "Failed to initialize Vulkan context" → Check Vulkan SDK installation
   - "No compatible GPU found" → Update GPU drivers
   - Validation errors → Check GPU supports required features
   ```

**Validation:**
- Test runs without crashes
- All 8 steps complete successfully
- No Vulkan validation errors
- No memory leaks
- Works on discrete and integrated GPUs

**Success Criteria for PHASE 2:**
✅ Zero memory leaks in Vulkan context
✅ Complete resource cleanup in all widgets
✅ No compilation errors in export system
✅ `vulkan_init_test` passes on target hardware
✅ Validation layers report no errors

**Files Created:**
- `src/vulkan_init_test.cpp`

**Files Modified:**
- `src/rendering/vulkan_context.cpp` (memory leak fixes)
- `src/ui/timeline_widget.cpp` (resource cleanup)
- `include/video/export_system.h` (syntax fix)
- `src/video/export_system.cpp` (undefined variable fix)
- `CMakeLists.txt` (add vulkan_init_test target)
- `docs/BUILD_VALIDATION.md` (updated)

**Deliverable:** Memory-safe core infrastructure + Vulkan validation

---

### PHASE 3: Rendering Stack [WEEK 4-5]

**Goal:** Create working window with Vulkan rendering
**Duration:** 30-40 hours (1 week)
**Success Criteria:** `simple_window_demo` displays rendered UI

#### Session 3.1: Window Creation & Swap Chain (4-6 hours)

[Details of window creation, swap chain setup, etc.]

#### Session 3.2: Graphics Pipeline Configuration (4-6 hours)

[Details of pipeline setup, shaders, etc.]

#### Session 3.3: UI Primitive Rendering (6-8 hours)

[Details of rendering rectangles, basic shapes, etc.]

#### Session 3.4: Integration Testing (4 hours)

[Details of creating simple_window_demo and testing]

**Success Criteria for PHASE 3:**
✅ Window displays without crashes
✅ Swap chain works correctly
✅ Can render colored rectangles
✅ Frame rate is stable (60 FPS)
✅ No rendering artifacts

---

### PHASE 4-12: [Additional Phases]

[Continue with similar detailed breakdowns for remaining phases...]

---

## Testing & Validation Strategy

### Validation at Each Phase

Every phase must produce a **working demo** that validates the work:

| Phase | Demo Name | What It Validates |
|-------|-----------|-------------------|
| 1 | minimal_headless_demo | Build system works |
| 2 | vulkan_init_test | Vulkan initializes |
| 3 | simple_window_demo | Rendering works |
| 4 | logging_demo | Logging framework works |
| 5 | test_suite | Tests pass |
| 6 | video_processing_demo | Video decode/encode works |
| 7 | timeline_demo | Timeline operations work |
| 8 | effects_audio_demo | Effects and audio work |
| 9 | export_demo | Export pipeline works |
| 10 | platform_tests | Cross-platform compatibility |
| 11 | security_tests | Security hardening works |
| 12 | full_application | Complete integrated system |

### Continuous Integration

After Phase 5 (Testing Infrastructure), every commit must:
1. ✅ Compile without warnings
2. ✅ Pass all unit tests
3. ✅ Pass integration tests
4. ✅ Have zero memory leaks (valgrind)
5. ✅ Have zero undefined behavior (ASan/UBSan)
6. ✅ Pass static analysis (clang-tidy)

---

## Risk Management

### High-Risk Areas

1. **Vulkan Availability** - Mitigated by early testing in Phase 2
2. **FFmpeg Integration** - Mitigated by conditional compilation
3. **Platform Differences** - Mitigated by CI/CD in Phase 5

### Contingency Plans

If Vulkan initialization fails:
- Fall back to software rendering (SwiftShader)
- Or defer to Phase 10 for platform-specific solutions

If FFmpeg unavailable:
- Continue with mock implementations
- Document FFmpeg as required dependency

---

## Progress Tracking

### Current Status

- ✅ **PHASE 0: Complete**
- ⏳ **PHASE 1: Ready to start**
- ⬜ **PHASE 2-12: Not started**

### Completion Metrics

Track progress with:
```
Total Sessions: 0 / 100+
Total Hours: 0 / 500+
Phases Complete: 0 / 12
Demos Working: 0 / 12
Test Coverage: 0%
```

### Weekly Reviews

Every Friday:
1. Review completed sessions
2. Update completion metrics
3. Identify blockers
4. Plan next week's sessions

---

## Summary

This implementation plan provides:

✅ **Phased approach** - Clear progression from foundation to features
✅ **Work session structure** - Organized, focused work periods
✅ **Validation strategy** - Working demo at each phase
✅ **Risk management** - Identified risks and mitigations
✅ **Progress tracking** - Clear metrics and milestones

**Next Step:** Begin PHASE 1, Session 1.1 (External Dependencies Setup)

---

**Document Version:** 1.0
**Last Updated:** 2025-11-15
**Next Review:** After Phase 1 completion
