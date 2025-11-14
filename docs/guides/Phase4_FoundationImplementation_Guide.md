# Phase 4: Foundation Implementation Guide - Shotcut C++ Transformation

**Document Created:** November 13, 2025  
**Implementation Focus:** Building the Core Foundation Infrastructure  
**Duration:** Months 1-3 (Foundation Phase)  
**Prerequisites:** Phases 0-3 completed (Research, Analysis, Architecture Design)

---

## PHASE 4 OVERVIEW

Phase 4 focuses on establishing the fundamental infrastructure that will support the entire C++ video editor application. This phase creates the technical foundation upon which all subsequent phases will build, emphasizing security, performance, and modern C++20/23 practices.

**Critical Success Factors:**
- **Security-First Development:** Compiler hardening and secure coding practices from day one
- **Modern C++20/23 Foundation:** Modules, concepts, and RAII patterns throughout
- **Cross-Platform Compatibility:** Platform abstraction layer implemented early
- **Performance Foundation:** Memory management and threading optimized from start
- **GPU-First Architecture:** Vulkan integration as core rendering foundation

---

## 4.1 CMAKE BUILD SYSTEM WITH SECURITY HARDENING

### 4.1.1 Project Structure Setup

**Directory Layout:**
```
ShotcutCPP/
├── CMakeLists.txt                 # Root CMake configuration
├── cmake/                         # CMake modules and utilities
│   ├── FindVulkan.cmake
│   ├── FindFFmpeg.cmake
│   ├── FindNVIDIA.cmake
│   ├── CompilerHardening.cmake
│   └── SecurityFlags.cmake
├── src/                          # Source code
│   ├── core/                      # Core application infrastructure
│   ├── platform/                   # Platform-specific code
│   ├── rendering/                  # Vulkan/DirectX12 rendering
│   ├── ui/                        # UI framework
│   ├── video/                      # Video processing
│   ├── audio/                      # Audio processing
│   └── utils/                     # Utilities and helpers
├── include/                       # Public headers
├── external/                      # Third-party dependencies
├── tests/                         # Unit and integration tests
├── shaders/                       # Vulkan/DirectX12 shaders
├── resources/                      # Application resources
└── docs/                          # Documentation
```

### 4.1.2 Root CMakeLists.txt Configuration

**Modern CMake with Security Hardening:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(ShotcutCPP 
    VERSION 1.0.0 
    DESCRIPTION "Modern C++ Video Editor"
    LANGUAGES CXX
)

# C++20/23 Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable C++ modules
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

# Security and hardening
include(cmake/CompilerHardening.cmake)
include(cmake/SecurityFlags.cmake)

# Platform detection
include(CheckCXXSourceCompiles)
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
endif()

# Find required packages
find_package(Vulkan REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat swscale)
find_package(PkgConfig REQUIRED)

# Platform-specific packages
if(PLATFORM_WINDOWS)
    find_package(DirectX12 REQUIRED)
elseif(PLATFORM_LINUX)
    pkg_check_modules(VULKAN REQUIRED vulkan)
    find_package(X11 REQUIRED)
elseif(PLATFORM_MACOS)
    find_package(Metal REQUIRED)
    find_package(Cocoa REQUIRED)
endif()

# Subdirectories
add_subdirectory(src)
add_subdirectory(external)
add_subdirectory(tests)

# Installation configuration
include(GNUInstallDirs)
install(DIRECTORY resources/ DESTINATION ${CMAKE_INSTALL_DATADIR})
install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
```

### 4.1.3 Compiler Hardening Configuration

**cmake/CompilerHardening.cmake:**
```cmake
# Security hardening flags based on compiler
function(apply_compiler_hardening target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Stack protection
        target_compile_options(${target} PRIVATE
            -fstack-protector-strong
            -fstack-clash-protection
        )
        
        # Control Flow Integrity
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE -fcf-protection=full)
        endif()
        
        # Position Independent Code
        target_compile_options(${target} PRIVATE -fPIE -fPIC)
        
        # Fortify Source
        target_compile_definitions(${target} PRIVATE
            -D_FORTIFY_SOURCE=3
        )
        
        # Format security
        target_compile_options(${target} PRIVATE
            -Wformat -Wformat-security
            -Werror=format-security
        )
        
        # Linker hardening
        target_link_options(${target} PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -pie
        )
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Microsoft security features
        target_compile_options(${target} PRIVATE
            /GS                    # Buffer security check
            /guard:cf              # Control flow guard
            /sdl                   # Security development lifecycle
            /DYNAMICBASE            # Address space layout randomization
            /HIGHENTROPYVA         # 64-bit ASLR
            /SAFESEH                # Safe exception handlers
            /NXCOMPAT               # Data execution prevention
        )
    endif()
    
    # Sanitizers for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE /fsanitize=address)
            target_link_options(${target} PRIVATE /fsanitize=address)
        endif()
    endif()
endfunction()

# LTO optimization for release builds
function(enable_lto target)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckIPOSupported)
        cmake_ipo_supported(RESULT ipo_supported)
        if(ipo_supported)
            set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
    endif()
endfunction()
```

### 4.1.4 Security Flags Configuration

**cmake/SecurityFlags.cmake:**
```cmake
# Additional security configurations
function(configure_security target)
    # Runtime security checks
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(${target} PRIVATE
            -D_GLIBCXX_ASSERTIONS=1
            -D_LIBCPP_DEBUG=0  # Disable libcpp debug in release
        )
    endif()
    
    # Disable insecure functions
    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            -D_CRT_SECURE_NO_WARNINGS
            -D_CRT_SECURE_NO_DEPRECATE
        )
    endif()
    
    # Stack smashing protection
    if(UNIX AND NOT APPLE)
        target_link_options(${target} PRIVATE -Wl,-z,noexecstack)
    endif()
    
    # RELRO (Read-Only Relocation)
    if(UNIX)
        target_link_options(${target} PRIVATE -Wl,-z,relro -Wl,-z,now)
    endif()
endfunction()
```

---

## 4.2 VULKAN RENDERING CONTEXT IMPLEMENTATION

### 4.2.1 Core Vulkan Infrastructure

**src/rendering/vulkan_context.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <functional>

class VulkanContext {
public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        std::optional<uint32_t> compute_family;
        std::optional<uint32_t> transfer_family;
        
        bool is_complete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };
    
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };
    
private:
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    VkQueue compute_queue_;
    VkQueue transfer_queue_;
    
    QueueFamilyIndices queue_families_;
    SwapChainSupportDetails swap_chain_support_;
    
    bool enable_validation_layers_;
    std::vector<const char*> validation_layers_;
    std::vector<const char*> device_extensions_;
    
public:
    VulkanContext(bool enable_validation = true);
    ~VulkanContext();
    
    // Initialization
    bool initialize(GLFWwindow* window);
    void cleanup();
    
    // Accessors
    VkInstance instance() const { return instance_; }
    VkDevice device() const { return device_; }
    VkPhysicalDevice physical_device() const { return physical_device_; }
    VkSurfaceKHR surface() const { return surface_; }
    
    VkQueue graphics_queue() const { return graphics_queue_; }
    VkQueue present_queue() const { return present_queue_; }
    VkQueue compute_queue() const { return compute_queue_; }
    VkQueue transfer_queue() const { return transfer_queue_; }
    
    const QueueFamilyIndices& queue_families() const { return queue_families_; }
    const SwapChainSupportDetails& swap_chain_support() const { return swap_chain_support_; }
    
    // Utility functions
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, 
                               VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat find_depth_format();
    
    bool is_device_suitable(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    
private:
    // Instance setup
    bool create_instance();
    bool setup_debug_messenger();
    bool create_surface(GLFWwindow* window);
    
    // Device setup
    bool pick_physical_device();
    bool create_logical_device();
    bool is_extension_supported(const std::string& extension_name);
    
    // Validation layers
    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    
    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data
    );
    
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    VkResult create_debug_utils_messenger_ext(
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        VkDebugUtilsMessengerEXT* debug_messenger
    );
    void destroy_debug_utils_messenger_ext(VkDebugUtilsMessengerEXT debug_messenger);
};
```

**src/rendering/vulkan_context.cpp:**
```cpp
#include "vulkan_context.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <set>
#include <algorithm>

const std::vector<const char*> validation_layers_ = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions_ = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanContext::VulkanContext(bool enable_validation) 
    : instance_(VK_NULL_HANDLE), debug_messenger_(VK_NULL_HANDLE),
      surface_(VK_NULL_HANDLE), physical_device_(VK_NULL_HANDLE),
      device_(VK_NULL_HANDLE), graphics_queue_(VK_NULL_HANDLE),
      present_queue_(VK_NULL_HANDLE), compute_queue_(VK_NULL_HANDLE),
      transfer_queue_(VK_NULL_HANDLE), enable_validation_layers_(enable_validation) {
    
    if (enable_validation && !check_validation_layer_support()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }
    
    validation_layers_ = validation_layers_;
    device_extensions_ = device_extensions_;
}

VulkanContext::~VulkanContext() {
    cleanup();
}

bool VulkanContext::initialize(GLFWwindow* window) {
    try {
        return create_instance() &&
               setup_debug_messenger() &&
               create_surface(window) &&
               pick_physical_device() &&
               create_logical_device();
    } catch (const std::exception& e) {
        std::cerr << "Vulkan initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanContext::cleanup() {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }
    
    if (debug_messenger_ != VK_NULL_HANDLE) {
        destroy_debug_utils_messenger_ext(debug_messenger_);
    }
    
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
    
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

bool VulkanContext::create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Shotcut CPP";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Shotcut Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;
    
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    auto extensions = get_required_extensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    
    if (enable_validation_layers_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanContext::setup_debug_messenger() {
    if (!enable_validation_layers_) return true;
    
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populate_debug_messenger_create_info(create_info);
    
    return create_debug_utils_messenger_ext(&create_info, &debug_messenger_) == VK_SUCCESS;
}

bool VulkanContext::create_surface(GLFWwindow* window) {
    VkResult result = glfwCreateWindowSurface(instance_, window, nullptr, &surface_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create window surface: " << result << std::endl;
        return false;
    }
    return true;
}

bool VulkanContext::pick_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    
    if (device_count == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    
    // Score devices and pick the best one
    int best_score = -1;
    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            int score = rate_device_suitability(device);
            if (score > best_score) {
                physical_device_ = device;
                best_score = score;
            }
        }
    }
    
    if (physical_device_ == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        return false;
    }
    
    queue_families_ = find_queue_families(physical_device_);
    swap_chain_support_ = query_swap_chain_support(physical_device_);
    
    return true;
}

bool VulkanContext::create_logical_device() {
    VkDeviceQueueCreateInfo queue_create_infos[4];
    uint32_t queue_create_count = 0;
    std::set<uint32_t> unique_queue_families;
    
    // Graphics queue
    if (queue_families_.graphics_family.has_value()) {
        VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = *queue_families_.graphics_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &(float){1.0f};
        unique_queue_families.insert(*queue_families_.graphics_family);
    }
    
    // Present queue
    if (queue_families_.present_family.has_value() && 
        unique_queue_families.find(*queue_families_.present_family) == unique_queue_families.end()) {
        VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = *queue_families_.present_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &(float){1.0f};
        unique_queue_families.insert(*queue_families_.present_family);
    }
    
    // Compute queue
    if (queue_families_.compute_family.has_value() && 
        unique_queue_families.find(*queue_families_.compute_family) == unique_queue_families.end()) {
        VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = *queue_families_.compute_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &(float){1.0f};
        unique_queue_families.insert(*queue_families_.compute_family);
    }
    
    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.fillModeNonSolid = VK_TRUE;
    device_features.independentBlend = VK_TRUE;
    device_features.sampleRateShading = VK_TRUE;
    
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();
    
    if (enable_validation_layers_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    
    VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create logical device: " << result << std::endl;
        return false;
    }
    
    // Get queue handles
    vkGetDeviceQueue(device_, *queue_families_.graphics_family, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, *queue_families_.present_family, 0, &present_queue_);
    
    if (queue_families_.compute_family.has_value()) {
        vkGetDeviceQueue(device_, *queue_families_.compute_family, 0, &compute_queue_);
    }
    
    return true;
}

// Additional implementation methods would continue here...
// Including validation layer support, device rating, memory type finding, etc.
```
