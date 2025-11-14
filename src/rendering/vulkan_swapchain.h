#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class VulkanContext;

/**
 * @class VulkanSwapchain
 * @brief Manages Vulkan swap chain for presenting rendered frames to screen
 * 
 * Handles:
 * - Swap chain creation and recreation
 * - Image views for swap chain images
 * - Framebuffers for rendering targets
 * - Presentation synchronization
 */
class VulkanSwapchain {
public:
    struct SwapchainImage {
        VkImage image;
        VkImageView view;
        VkFramebuffer framebuffer;
    };

    VulkanSwapchain(VulkanContext* context);
    ~VulkanSwapchain();

    // Initialization
    bool initialize(VkSurfaceKHR surface, uint32_t width, uint32_t height);
    void cleanup();

    // Accessors
    VkSwapchainKHR swapchain() const { return swapchain_; }
    VkFormat image_format() const { return image_format_; }
    VkExtent2D extent() const { return extent_; }
    uint32_t image_count() const { return static_cast<uint32_t>(images_.size()); }
    const std::vector<SwapchainImage>& images() const { return images_; }
    VkRenderPass render_pass() const { return render_pass_; }

    // Frame management
    VkResult acquire_next_image(uint32_t& image_index, VkSemaphore semaphore);
    VkResult present_image(uint32_t image_index, VkSemaphore wait_semaphore);

    // Utilities
    void recreate(uint32_t width, uint32_t height);

private:
    VulkanContext* context_;
    VkSwapchainKHR swapchain_;
    VkFormat image_format_;
    VkExtent2D extent_;
    std::vector<SwapchainImage> images_;
    VkRenderPass render_pass_;
    VkImage depth_image_;
    VkDeviceMemory depth_image_memory_;
    VkImageView depth_image_view_;

    bool create_swapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height);
    bool create_image_views();
    bool create_render_pass();
    bool create_framebuffers();
    bool create_depth_resources();
    void cleanup_swapchain();
};
