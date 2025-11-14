#include "vulkan_ui_renderer.h"
#include "../rendering/vulkan_context.h"
#include "../rendering/vulkan_swapchain.h"
#include "../rendering/vulkan_graphics_pipeline.h"
#include "../rendering/vulkan_command_buffer.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace Shotcut::UI {

VulkanUIRenderer::VulkanUIRenderer()
    : context_(nullptr), swapchain_(nullptr), pipeline_(nullptr), command_buffer_(nullptr),
      initialized_(false), vertex_buffer_handle_(VK_NULL_HANDLE), 
      vertex_buffer_memory_(VK_NULL_HANDLE), vertex_buffer_capacity_(0) {
}

VulkanUIRenderer::~VulkanUIRenderer() {
    cleanup();
}

bool VulkanUIRenderer::initialize(VulkanContext* context, VulkanSwapchain* swapchain,
                                 VulkanGraphicsPipeline* pipeline, VulkanCommandBuffer* command_buffer) {
    try {
        context_ = context;
        swapchain_ = swapchain;
        pipeline_ = pipeline;
        command_buffer_ = command_buffer;

        // Create default theme
        current_theme_ = std::make_shared<Theme>();
        current_theme_->set_dark_theme();

        // Create vertex buffer
        if (!create_vertex_buffer()) {
            return false;
        }

        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Vulkan UI renderer initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanUIRenderer::cleanup() {
    if (context_) {
        vkDeviceWaitIdle(context_->device());
    }

    destroy_vertex_buffer();
    initialized_ = false;
}

bool VulkanUIRenderer::create_vertex_buffer() {
    vertex_buffer_capacity_ = 10000; // Initial capacity for 10000 vertices
    size_t buffer_size = vertex_buffer_capacity_ * sizeof(Vertex);

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = buffer_size;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(context_->device(), &buffer_info, nullptr, &vertex_buffer_handle_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create vertex buffer: " << result << std::endl;
        return false;
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context_->device(), vertex_buffer_handle_, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = context_->find_memory_type(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    result = vkAllocateMemory(context_->device(), &alloc_info, nullptr, &vertex_buffer_memory_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate vertex buffer memory: " << result << std::endl;
        return false;
    }

    vkBindBufferMemory(context_->device(), vertex_buffer_handle_, vertex_buffer_memory_, 0);

    return true;
}

void VulkanUIRenderer::destroy_vertex_buffer() {
    if (vertex_buffer_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->device(), vertex_buffer_memory_, nullptr);
        vertex_buffer_memory_ = VK_NULL_HANDLE;
    }

    if (vertex_buffer_handle_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(context_->device(), vertex_buffer_handle_, nullptr);
        vertex_buffer_handle_ = VK_NULL_HANDLE;
    }
}

void VulkanUIRenderer::set_theme(std::shared_ptr<Theme> theme) {
    current_theme_ = theme;
}

bool VulkanUIRenderer::begin_frame(uint32_t& image_index) {
    vertex_buffer_.clear();
    return command_buffer_->begin_frame(image_index);
}

bool VulkanUIRenderer::end_frame() {
    flush_vertices();
    return command_buffer_->end_frame();
}

void VulkanUIRenderer::add_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    // Create quad vertices (two triangles)
    Vertex v0 = {{position.x, position.y}, color, {0.0f, 0.0f}};
    Vertex v1 = {{position.x + size.x, position.y}, color, {1.0f, 0.0f}};
    Vertex v2 = {{position.x + size.x, position.y + size.y}, color, {1.0f, 1.0f}};
    Vertex v3 = {{position.x, position.y + size.y}, color, {0.0f, 1.0f}};

    // First triangle
    vertex_buffer_.push_back(v0);
    vertex_buffer_.push_back(v1);
    vertex_buffer_.push_back(v2);

    // Second triangle
    vertex_buffer_.push_back(v0);
    vertex_buffer_.push_back(v2);
    vertex_buffer_.push_back(v3);
}

void VulkanUIRenderer::flush_vertices() {
    if (vertex_buffer_.empty()) {
        return;
    }

    // Copy vertices to GPU memory
    void* data;
    vkMapMemory(context_->device(), vertex_buffer_memory_, 0, vertex_buffer_.size() * sizeof(Vertex), 0, &data);
    memcpy(data, vertex_buffer_.data(), vertex_buffer_.size() * sizeof(Vertex));
    vkUnmapMemory(context_->device(), vertex_buffer_memory_);

    // Bind vertex buffer
    VkBuffer buffers[] = {vertex_buffer_handle_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer_->current_command_buffer(), 0, 1, buffers, offsets);

    // Draw
    command_buffer_->draw_vertices(static_cast<uint32_t>(vertex_buffer_.size()));
}

void VulkanUIRenderer::draw_rectangle(const glm::vec2& position, const glm::vec2& size,
                                     const glm::vec4& color, float border_radius) {
    // For now, ignore border_radius and draw simple rectangle
    // Border radius support can be added with more complex shaders later
    add_quad(position, size, color);
}

void VulkanUIRenderer::draw_button(const glm::vec2& position, const glm::vec2& size,
                                  const std::string& text, const glm::vec4& bg_color,
                                  const glm::vec4& text_color, float border_radius) {
    // Draw button background
    draw_rectangle(position, size, bg_color, border_radius);
    
    // Text rendering would go here (requires font system)
    // For now, just draw the background
}

void VulkanUIRenderer::draw_panel(const glm::vec2& position, const glm::vec2& size,
                                 const glm::vec4& bg_color, float border_radius) {
    draw_rectangle(position, size, bg_color, border_radius);
}

void VulkanUIRenderer::draw_text(const glm::vec2& position, const std::string& text,
                                const glm::vec4& color, float font_size) {
    // Text rendering requires font system (FreeType + HarfBuzz)
    // This is a placeholder for future implementation
}

void VulkanUIRenderer::render_widget(const std::shared_ptr<Widget>& widget) {
    if (!widget || !widget->is_visible()) {
        return;
    }

    // Get widget properties
    auto bounds = widget->get_bounds();
    auto bg_color = current_theme_->get_color(ColorRole::Background);

    // Draw widget background
    draw_rectangle(glm::vec2(bounds.x, bounds.y), glm::vec2(bounds.width, bounds.height), 
                  glm::vec4(bg_color.r, bg_color.g, bg_color.b, bg_color.a));

    // Render children
    for (const auto& child : widget->get_children()) {
        if (auto child_widget = std::dynamic_pointer_cast<Widget>(child)) {
            render_widget(child_widget);
        }
    }
}

} // namespace Shotcut::UI
