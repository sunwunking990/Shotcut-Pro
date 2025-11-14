#pragma once

#include "widget_system.h"
#include "theme_system.h"
#include "math_types.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class VulkanSwapchain;
class VulkanGraphicsPipeline;
class VulkanCommandBuffer;

namespace Shotcut::UI {

/**
 * @class VulkanUIRenderer
 * @brief Modern Vulkan-based UI rendering system
 * 
 * Provides GPU-accelerated rendering for UI components with:
 * - Direct Vulkan rendering pipeline
 * - Efficient vertex buffer management
 * - Theme-aware color rendering
 * - Professional UI appearance
 */
class VulkanUIRenderer {
public:
    struct Vertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 uv;
    };

    VulkanUIRenderer();
    ~VulkanUIRenderer();

    // Initialization
    bool initialize(VulkanContext* context, VulkanSwapchain* swapchain, 
                   VulkanGraphicsPipeline* pipeline, VulkanCommandBuffer* command_buffer);
    void cleanup();

    // Theme management
    void set_theme(std::shared_ptr<Theme> theme);
    const Theme& get_theme() const { return *current_theme_; }

    // Frame management
    bool begin_frame(uint32_t& image_index);
    bool end_frame();

    // Drawing primitives
    void draw_rectangle(const glm::vec2& position, const glm::vec2& size, 
                       const glm::vec4& color, float border_radius = 0.0f);
    void draw_button(const glm::vec2& position, const glm::vec2& size,
                    const std::string& text, const glm::vec4& bg_color,
                    const glm::vec4& text_color, float border_radius = 4.0f);
    void draw_panel(const glm::vec2& position, const glm::vec2& size,
                   const glm::vec4& bg_color, float border_radius = 0.0f);
    void draw_text(const glm::vec2& position, const std::string& text,
                  const glm::vec4& color, float font_size = 16.0f);

    // Widget rendering
    void render_widget(const std::shared_ptr<Widget>& widget);

private:
    VulkanContext* context_;
    VulkanSwapchain* swapchain_;
    VulkanGraphicsPipeline* pipeline_;
    VulkanCommandBuffer* command_buffer_;

    std::shared_ptr<Theme> current_theme_;
    std::vector<Vertex> vertex_buffer_;
    bool initialized_;

    // Vertex buffer management
    VkBuffer vertex_buffer_handle_;
    VkDeviceMemory vertex_buffer_memory_;
    size_t vertex_buffer_capacity_;

    bool create_vertex_buffer();
    void destroy_vertex_buffer();
    void add_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void flush_vertices();
};

} // namespace Shotcut::UI
