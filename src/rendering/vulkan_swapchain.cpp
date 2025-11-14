#include "vulkan_swapchain.h"
#include "vulkan_context.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

VulkanSwapchain::VulkanSwapchain(VulkanContext* context)
    : context_(context), swapchain_(VK_NULL_HANDLE), 
      render_pass_(VK_NULL_HANDLE), depth_image_(VK_NULL_HANDLE),
      depth_image_memory_(VK_NULL_HANDLE), depth_image_view_(VK_NULL_HANDLE) {
}

VulkanSwapchain::~VulkanSwapchain() {
    cleanup();
}

bool VulkanSwapchain::initialize(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    try {
        if (!create_swapchain(surface, width, height)) {
            return false;
        }
        if (!create_image_views()) {
            return false;
        }
        if (!create_render_pass()) {
            return false;
        }
        if (!create_depth_resources()) {
            return false;
        }
        if (!create_framebuffers()) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Swapchain initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanSwapchain::cleanup() {
    cleanup_swapchain();
    
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(context_->device(), render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
}

bool VulkanSwapchain::create_swapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    const auto& swap_chain_support = context_->swap_chain_support();
    
    VkSurfaceFormatKHR surface_format = swap_chain_support.formats[0];
    for (const auto& available_format : swap_chain_support.formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = available_format;
            break;
        }
    }
    
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& available_present_mode : swap_chain_support.present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = available_present_mode;
            break;
        }
    }
    
    VkExtent2D actual_extent = {width, height};
    actual_extent.width = std::clamp(actual_extent.width,
        swap_chain_support.capabilities.minImageExtent.width,
        swap_chain_support.capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height,
        swap_chain_support.capabilities.minImageExtent.height,
        swap_chain_support.capabilities.maxImageExtent.height);
    
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = actual_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    const auto& queue_families = context_->queue_families();
    uint32_t queue_family_indices[] = {
        *queue_families.graphics_family,
        *queue_families.present_family
    };
    
    if (queue_families.graphics_family != queue_families.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult result = vkCreateSwapchainKHR(context_->device(), &create_info, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create swap chain: " << result << std::endl;
        return false;
    }
    
    image_format_ = surface_format.format;
    extent_ = actual_extent;
    
    return true;
}

bool VulkanSwapchain::create_image_views() {
    uint32_t image_count;
    vkGetSwapchainImagesKHR(context_->device(), swapchain_, &image_count, nullptr);
    
    std::vector<VkImage> swap_chain_images(image_count);
    vkGetSwapchainImagesKHR(context_->device(), swapchain_, &image_count, swap_chain_images.data());
    
    images_.resize(image_count);
    
    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        images_[i].image = swap_chain_images[i];
        
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = image_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        
        VkResult result = vkCreateImageView(context_->device(), &create_info, nullptr, &images_[i].view);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create image view: " << result << std::endl;
            return false;
        }
    }
    
    return true;
}

bool VulkanSwapchain::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = context_->find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};
    
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    
    VkResult result = vkCreateRenderPass(context_->device(), &render_pass_info, nullptr, &render_pass_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create render pass: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanSwapchain::create_depth_resources() {
    VkFormat depth_format = context_->find_depth_format();
    
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = extent_.width;
    image_info.extent.height = extent_.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = depth_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateImage(context_->device(), &image_info, nullptr, &depth_image_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create depth image: " << result << std::endl;
        return false;
    }
    
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context_->device(), depth_image_, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = context_->find_memory_type(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    result = vkAllocateMemory(context_->device(), &alloc_info, nullptr, &depth_image_memory_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate depth image memory: " << result << std::endl;
        return false;
    }
    
    vkBindImageMemory(context_->device(), depth_image_, depth_image_memory_, 0);
    
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = depth_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = depth_format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    result = vkCreateImageView(context_->device(), &view_info, nullptr, &depth_image_view_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create depth image view: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanSwapchain::create_framebuffers() {
    for (size_t i = 0; i < images_.size(); i++) {
        VkImageView attachments[] = {
            images_[i].view,
            depth_image_view_
        };
        
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass_;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = extent_.width;
        framebuffer_info.height = extent_.height;
        framebuffer_info.layers = 1;
        
        VkResult result = vkCreateFramebuffer(context_->device(), &framebuffer_info, nullptr, &images_[i].framebuffer);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer: " << result << std::endl;
            return false;
        }
    }
    
    return true;
}

void VulkanSwapchain::cleanup_swapchain() {
    if (depth_image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(context_->device(), depth_image_view_, nullptr);
        depth_image_view_ = VK_NULL_HANDLE;
    }
    
    if (depth_image_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->device(), depth_image_memory_, nullptr);
        depth_image_memory_ = VK_NULL_HANDLE;
    }
    
    if (depth_image_ != VK_NULL_HANDLE) {
        vkDestroyImage(context_->device(), depth_image_, nullptr);
        depth_image_ = VK_NULL_HANDLE;
    }
    
    for (auto& image : images_) {
        if (image.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(context_->device(), image.framebuffer, nullptr);
            image.framebuffer = VK_NULL_HANDLE;
        }
        
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->device(), image.view, nullptr);
            image.view = VK_NULL_HANDLE;
        }
    }
    
    images_.clear();
    
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(context_->device(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

VkResult VulkanSwapchain::acquire_next_image(uint32_t& image_index, VkSemaphore semaphore) {
    return vkAcquireNextImageKHR(context_->device(), swapchain_, UINT64_MAX, semaphore, VK_NULL_HANDLE, &image_index);
}

VkResult VulkanSwapchain::present_image(uint32_t image_index, VkSemaphore wait_semaphore) {
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &image_index;
    
    return vkQueuePresentKHR(context_->present_queue(), &present_info);
}

void VulkanSwapchain::recreate(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(context_->device());
    cleanup_swapchain();
    create_swapchain(context_->surface(), width, height);
    create_image_views();
    create_depth_resources();
    create_framebuffers();
}
