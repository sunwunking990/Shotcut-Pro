#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <optional>
#include <concepts>
#include <ranges>
#include <functional>
#include <glm/glm.hpp>
#include <algorithm>

namespace Shotcut::UI {

// Basic geometry structures
struct Rect {
    float x = 0.0f, y = 0.0f;
    float width = 0.0f, height = 0.0f;
    
    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
    
    bool contains(const glm::vec2& point) const {
        return contains(point.x, point.y);
    }
    
    bool empty() const { return width <= 0.0f || height <= 0.0f; }
    
    Rect intersect(const Rect& other) const {
        float nx = std::max(x, other.x);
        float ny = std::max(y, other.y);
        float nw = std::min(x + width, other.x + other.width) - nx;
        float nh = std::min(y + height, other.y + other.height) - ny;
        
        if (nw <= 0.0f || nh <= 0.0f) return {};
        return {nx, ny, nw, nh};
    }
    
    bool operator!=(const Rect& other) const {
        return x != other.x || y != other.y || width != other.width || height != other.height;
    }
};

// Layout constraints system
struct LayoutConstraints {
    std::optional<float> min_width, max_width;
    std::optional<float> min_height, max_height;
    std::optional<float> preferred_width, preferred_height;
    
    bool has_flexible_width() const noexcept {
        return !min_width.has_value() || !max_width.has_value() || 
               (*max_width - *min_width) > 0.1f;
    }
    
    bool has_flexible_height() const noexcept {
        return !min_height.has_value() || !max_height.has_value() || 
               (*max_width - *min_width) > 0.1f;
    }
};

// Modern C++20 concepts for layout elements
template<typename T>
concept LayoutElement = requires(T t) {
    { t.get_bounds() } -> std::same_as<Rect>;
    { t.set_bounds(Rect{}) } -> std::same_as<void>;
    { t.calculate_constraints() } -> std::same_as<LayoutConstraints>;
    { t.apply_constraints(LayoutConstraints{}) } -> std::same_as<void>;
};

// Flex-like layout properties
struct FlexProperties {
    enum class Direction : uint8_t {
        Row, RowReverse, Column, ColumnReverse
    } direction = Direction::Row;
    
    enum class Wrap : uint8_t {
        NoWrap, Wrap, WrapReverse
    } wrap = Wrap::NoWrap;
    
    enum class JustifyContent : uint8_t {
        FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly
    } justify_content = JustifyContent::FlexStart;
    
    enum class AlignItems : uint8_t {
        FlexStart, FlexEnd, Center, Stretch, Baseline
    } align_items = AlignItems::Stretch;
    
    enum class AlignContent : uint8_t {
        FlexStart, FlexEnd, Center, Stretch, SpaceBetween, SpaceAround
    } align_content = AlignContent::Stretch;
    
    float gap = 0.0f;
};

// Forward declaration for Theme and StyleProperty
class Theme;
struct StyleProperty;

// Layout element base class
class LayoutElement {
public:
    virtual ~LayoutElement() = default;
    
    // Core layout interface
    virtual Rect get_bounds() const = 0;
    virtual void set_bounds(const Rect& bounds) = 0;
    virtual LayoutConstraints calculate_constraints() const = 0;
    virtual void apply_constraints(const LayoutConstraints& constraints) = 0;
    
    // Hierarchy management
    virtual void add_child(std::shared_ptr<LayoutElement> child) = 0;
    virtual void remove_child(std::shared_ptr<LayoutElement> child) = 0;
    virtual std::span<std::shared_ptr<LayoutElement>> get_children() = 0;
    
    // Layout properties
    virtual void set_flex_properties(const FlexProperties& props) = 0;
    virtual const FlexProperties& get_flex_properties() const = 0;
    
    // Layout computation
    virtual void compute_layout(float available_width, float available_height) = 0;
    virtual void invalidate_layout() = 0;
    virtual bool is_layout_valid() const = 0;
};

// Container element for children
class Container : public LayoutElement {
private:
    std::vector<std::shared_ptr<LayoutElement>> children_;
    FlexProperties flex_props_;
    Rect bounds_;
    bool layout_valid_ = false;
    
public:
    Container() = default;
    
    // LayoutElement interface
    Rect get_bounds() const override { return bounds_; }
    void set_bounds(const Rect& bounds) override { 
        bounds_ = bounds; 
        invalidate_layout();
    }
    
    LayoutConstraints calculate_constraints() const override;
    void apply_constraints(const LayoutConstraints& constraints) override;
    
    void add_child(std::shared_ptr<LayoutElement> child) override;
    void remove_child(std::shared_ptr<LayoutElement> child) override;
    std::span<std::shared_ptr<LayoutElement>> get_children() override { 
        return children_; 
    }
    
    void set_flex_properties(const FlexProperties& props) override {
        flex_props_ = props;
        invalidate_layout();
    }
    
    const FlexProperties& get_flex_properties() const override { 
        return flex_props_; 
    }
    
    void compute_layout(float available_width, float available_height) override;
    void invalidate_layout() override { layout_valid_ = false; }
    bool is_layout_valid() const override { return layout_valid_; }
    
private:
    struct FlexItem {
        std::shared_ptr<LayoutElement> element;
        float flex_grow = 0.0f;
        float flex_shrink = 1.0f;
        float flex_basis = 0.0f;
        LayoutConstraints constraints;
    };
    
    std::vector<FlexItem> prepare_flex_items() const;
    void compute_flex_layout(const std::vector<FlexItem>& items, 
                           float available_width, float available_height);
    float distribute_flexible_space(const std::vector<FlexItem>& items, 
                               float available_space, bool is_width) const;
};

// GPU-accelerated layout computation
class GPULayoutEngine {
private:
    VkDescriptorSetLayout descriptor_set_layout_;
    VkPipeline compute_pipeline_;
    VkBuffer layout_buffer_;
    VkDeviceMemory layout_buffer_memory_;
    
    struct LayoutComputeData {
        glm::vec4 bounds;           // x, y, width, height
        glm::vec4 constraints;      // min_w, max_w, min_h, max_h
        glm::vec4 flex_props;       // direction, wrap, justify, align
        uint32_t child_count;
        uint32_t padding[3];
    };
    
public:
    GPULayoutEngine();
    ~GPULayoutEngine();
    
    bool initialize(VkDevice device, VkPhysicalDevice physical_device);
    void cleanup(VkDevice device);
    
    // GPU-accelerated layout computation
    void compute_layout_async(std::span<LayoutElement*> elements, 
                          std::function<void()> completion_callback,
                          VkDevice device, VkCommandPool command_pool, VkQueue queue);
    
    // CPU fallback
    void compute_layout_cpu(std::span<LayoutElement*> elements);
    
private:
    bool create_compute_pipeline(VkDevice device, VkPhysicalDevice physical_device);
    bool create_layout_buffers(VkDevice device, VkPhysicalDevice physical_device);
    void update_layout_buffer(const LayoutComputeData& data, VkDevice device);
};

} // namespace Shotcut::UI
