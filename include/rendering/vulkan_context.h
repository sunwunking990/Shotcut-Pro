#pragma once

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <functional>

// Forward declaration for GLFW
#ifdef HAVE_GLFW
struct GLFWwindow;
#else
struct GLFWwindow; // Forward declaration for when GLFW is not available
#endif

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
    
    // Device rating for selection
    int rate_device_suitability(VkPhysicalDevice device);
};
