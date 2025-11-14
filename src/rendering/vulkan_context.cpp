#include "vulkan_context.h"

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <iostream>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <cstdint>
#include <limits>

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
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
    
    validation_layers_ = validation_layers;
    device_extensions_ = device_extensions;
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
#ifdef HAVE_GLFW
    VkResult result = glfwCreateWindowSurface(instance_, window, nullptr, &surface_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create window surface: " << result << std::endl;
        return false;
    }
    return true;
#else
    // For now, create a dummy surface - in real implementation this would be platform-specific
    surface_ = VK_NULL_HANDLE;
    std::cerr << "Warning: GLFW not available, creating dummy surface" << std::endl;
    return true;
#endif
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
        queue_info.pQueuePriorities = new float{1.0f};
        unique_queue_families.insert(*queue_families_.graphics_family);
    }
    
    // Present queue
    if (queue_families_.present_family.has_value() && 
        unique_queue_families.find(*queue_families_.present_family) == unique_queue_families.end()) {
        VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = *queue_families_.present_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = new float{1.0f};
        unique_queue_families.insert(*queue_families_.present_family);
    }
    
    // Compute queue
    if (queue_families_.compute_family.has_value() && 
        unique_queue_families.find(*queue_families_.compute_family) == unique_queue_families.end()) {
        VkDeviceQueueCreateInfo& queue_info = queue_create_infos[queue_create_count++];
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = *queue_families_.compute_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = new float{1.0f};
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

bool VulkanContext::check_validation_layer_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    
    for (const char* layer_name : validation_layers) {
        bool layer_found = false;
        
        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        
        if (!layer_found) {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> VulkanContext::get_required_extensions() {
    std::vector<const char*> extensions;
    
#ifdef HAVE_GLFW
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    extensions.insert(extensions.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
#else
    // Add platform-specific extensions when GLFW is not available
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef PLATFORM_WINDOWS
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#endif
    
    if (enable_validation_layers_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;
    
    return VK_FALSE;
}

void VulkanContext::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
}

VkResult VulkanContext::create_debug_utils_messenger_ext(
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    VkDebugUtilsMessengerEXT* debug_messenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) 
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance_, create_info, nullptr, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanContext::destroy_debug_utils_messenger_ext(VkDebugUtilsMessengerEXT debug_messenger) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) 
        vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance_, debug_messenger, nullptr);
    }
}

int VulkanContext::rate_device_suitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);
    
    int score = 0;
    
    // Discrete GPUs are usually a huge performance boost
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    
    // Maximum possible size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;
    
    // Check for required features
    if (!device_features.samplerAnisotropy) {
        return 0; // Cannot function without anisotropic filtering
    }
    
    return score;
}

bool VulkanContext::is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);
    
    bool extensions_supported = true;
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
    
    std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());
    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }
    
    extensions_supported = required_extensions.empty();
    
    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && 
                             !swap_chain_support.present_modes.empty();
    }
    
    return indices.is_complete() && extensions_supported && swap_chain_adequate;
}

VulkanContext::QueueFamilyIndices VulkanContext::find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    
    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        
        if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.compute_family = i;
        }
        
        if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transfer_family = i;
        }
        
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (present_support) {
            indices.present_family = i;
        }
        
        if (indices.is_complete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

VulkanContext::SwapChainSupportDetails VulkanContext::query_swap_chain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);
    
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }
    
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }
    
    return details;
}

uint32_t VulkanContext::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
    
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && 
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

VkFormat VulkanContext::find_supported_format(const std::vector<VkFormat>& candidates, 
                                         VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format!");
}

VkFormat VulkanContext::find_depth_format() {
    return find_supported_format(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
