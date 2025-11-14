#include "layout_engine.h"
#include <algorithm>
#include <numeric>

namespace Shotcut::UI {

LayoutConstraints Container::calculate_constraints() const {
    LayoutConstraints result;
    
    if (children_.empty()) {
        result.min_width = result.max_width = 0.0f;
        result.min_height = result.max_height = 0.0f;
        return result;
    }
    
    // Calculate combined constraints based on flex direction
    float total_min_width = 0.0f, total_max_width = 0.0f;
    float total_min_height = 0.0f, total_max_height = 0.0f;
    float max_child_min_width = 0.0f, max_child_min_height = 0.0f;
    
    for (const auto& child : children_) {
        auto child_constraints = child->calculate_constraints();
        
        // For row layout
        if (flex_props_.direction == FlexProperties::Direction::Row ||
            flex_props_.direction == FlexProperties::Direction::RowReverse) {
            
            total_min_width += child_constraints.min_width.value_or(0.0f);
            total_max_width += child_constraints.max_width.value_or(std::numeric_limits<float>::max());
            max_child_min_height = std::max(max_child_min_height, 
                                        child_constraints.min_height.value_or(0.0f));
        } 
        // For column layout
        else {
            total_min_height += child_constraints.min_height.value_or(0.0f);
            total_max_height += child_constraints.max_height.value_or(std::numeric_limits<float>::max());
            max_child_min_width = std::max(max_child_min_width, 
                                       child_constraints.min_width.value_or(0.0f));
        }
    }
    
    // Apply flex properties
    if (flex_props_.direction == FlexProperties::Direction::Row ||
        flex_props_.direction == FlexProperties::Direction::RowReverse) {
        
        result.min_width = total_min_width + (children_.size() - 1) * flex_props_.gap;
        result.max_width = total_max_width + (children_.size() - 1) * flex_props_.gap;
        result.min_height = max_child_min_height;
        result.max_height = std::nullopt; // Flexible height by default
    } else {
        result.min_height = total_min_height + (children_.size() - 1) * flex_props_.gap;
        result.max_height = total_max_height + (children_.size() - 1) * flex_props_.gap;
        result.min_width = max_child_min_width;
        result.max_width = std::nullopt; // Flexible width by default
    }
    
    return result;
}

void Container::apply_constraints(const LayoutConstraints& constraints) {
    // Apply container-level constraints
    if (constraints.min_width) {
        bounds_.width = std::max(bounds_.width, *constraints.min_width);
    }
    if (constraints.max_width) {
        bounds_.width = std::min(bounds_.width, *constraints.max_width);
    }
    if (constraints.min_height) {
        bounds_.height = std::max(bounds_.height, *constraints.min_height);
    }
    if (constraints.max_height) {
        bounds_.height = std::min(bounds_.height, *constraints.max_height);
    }
}

void Container::add_child(std::shared_ptr<LayoutElement> child) {
    children_.push_back(child);
    invalidate_layout();
}

void Container::remove_child(std::shared_ptr<LayoutElement> child) {
    children_.erase(
        std::remove(children_.begin(), children_.end(), child),
        children_.end()
    );
    invalidate_layout();
}



void Container::compute_layout(float available_width, float available_height) {
    if (layout_valid_ && children_.empty()) return;
    
    // Prepare flex items
    auto flex_items = prepare_flex_items();
    if (flex_items.empty()) return;
    
    // Compute flex layout
    compute_flex_layout(flex_items, available_width, available_height);
    
    layout_valid_ = true;
}



std::vector<Container::FlexItem> Container::prepare_flex_items() const {
    std::vector<FlexItem> items;
    items.reserve(children_.size());
    
    for (const auto& child : children_) {
        FlexItem item;
        item.element = child;
        item.constraints = child->calculate_constraints();
        item.flex_basis = item.constraints.preferred_width.value_or(0.0f);
        
        items.push_back(std::move(item));
    }
    
    return items;
}

void Container::compute_flex_layout(const std::vector<FlexItem>& items, 
                                float available_width, float available_height) {
    
    const bool is_row = (flex_props_.direction == FlexProperties::Direction::Row ||
                        flex_props_.direction == FlexProperties::Direction::RowReverse);
    
    const float main_axis_size = is_row ? available_width : available_height;
    const float cross_axis_size = is_row ? available_height : available_width;
    
    // Calculate total flexible space
    float total_flexible_space = 0.0f;
    for (const auto& item : items) {
        if (is_row && item.constraints.has_flexible_width()) {
            total_flexible_space += item.flex_grow;
        } else if (!is_row && item.constraints.has_flexible_height()) {
            total_flexible_space += item.flex_grow;
        }
    }
    
    // Distribute space and position items
    float current_pos = 0.0f;
    const float gap = flex_props_.gap;
    
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        Rect child_bounds;
        
        if (is_row) {
            // Calculate width
            float child_width = item.flex_basis;
            if (item.constraints.has_flexible_width() && total_flexible_space > 0.0f) {
                float extra_space = main_axis_size - child_width;
                child_width += extra_space * (item.flex_grow / total_flexible_space);
            }
            
            // Calculate height
            float child_height = cross_axis_size;
            if (item.constraints.min_height) {
                child_height = std::max(child_height, *item.constraints.min_height);
            }
            if (item.constraints.max_height) {
                child_height = std::min(child_height, *item.constraints.max_height);
            }
            
            child_bounds = {current_pos, 0.0f, child_width, child_height};
            current_pos += child_width + gap;
        } else {
            // Calculate height
            float child_height = item.flex_basis;
            if (item.constraints.has_flexible_height() && total_flexible_space > 0.0f) {
                float extra_space = main_axis_size - child_height;
                child_height += extra_space * (item.flex_grow / total_flexible_space);
            }
            
            // Calculate width
            float child_width = cross_axis_size;
            if (item.constraints.min_width) {
                child_width = std::max(child_width, *item.constraints.min_width);
            }
            if (item.constraints.max_width) {
                child_width = std::min(child_width, *item.constraints.max_width);
            }
            
            child_bounds = {0.0f, current_pos, child_width, child_height};
            current_pos += child_height + gap;
        }
        
        item.element->set_bounds(child_bounds);
        item.element->apply_constraints(item.constraints);
    }
}

float Container::distribute_flexible_space(const std::vector<FlexItem>& items, 
                                       float available_space, bool is_width) const {
    float total_flex_grow = 0.0f;
    
    for (const auto& item : items) {
        if (is_width && item.constraints.has_flexible_width()) {
            total_flex_grow += item.flex_grow;
        } else if (!is_width && item.constraints.has_flexible_height()) {
            total_flex_grow += item.flex_grow;
        }
    }
    
    return total_flex_grow > 0.0f ? available_space / total_flex_grow : 0.0f;
}

// GPU Layout Engine Implementation
GPULayoutEngine::GPULayoutEngine() 
    : descriptor_set_layout_(VK_NULL_HANDLE), compute_pipeline_(VK_NULL_HANDLE),
      layout_buffer_(VK_NULL_HANDLE), layout_buffer_memory_(VK_NULL_HANDLE) {
}

GPULayoutEngine::~GPULayoutEngine() {
    // Cleanup should be called explicitly
}

bool GPULayoutEngine::initialize(VkDevice device, VkPhysicalDevice physical_device) {
    // Create layout buffers and compute pipeline
    if (!create_layout_buffers(device, physical_device)) {
        return false;
    }
    
    if (!create_compute_pipeline(device, physical_device)) {
        return false;
    }
    
    return true;
}

void GPULayoutEngine::cleanup(VkDevice device) {
    if (compute_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, compute_pipeline_, nullptr);
        compute_pipeline_ = VK_NULL_HANDLE;
    }
    
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    
    if (layout_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, layout_buffer_, nullptr);
        layout_buffer_ = VK_NULL_HANDLE;
    }
    
    if (layout_buffer_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, layout_buffer_memory_, nullptr);
        layout_buffer_memory_ = VK_NULL_HANDLE;
    }
}

void GPULayoutEngine::compute_layout_async(std::span<LayoutElement*> elements, 
                                         std::function<void()> completion_callback,
                                         VkDevice device, VkCommandPool command_pool, VkQueue queue) {
    // TODO: Implement GPU async layout computation
    // For now, fall back to CPU computation
    compute_layout_cpu(elements);
    if (completion_callback) {
        completion_callback();
    }
}

void GPULayoutEngine::compute_layout_cpu(std::span<LayoutElement*> elements) {
    // CPU fallback for layout computation
    for (auto* element : elements) {
        if (auto* container = dynamic_cast<Container*>(element)) {
            // Compute container layout with its bounds
            auto bounds = container->get_bounds();
            container->compute_layout(bounds.width, bounds.height);
        }
    }
}

bool GPULayoutEngine::create_compute_pipeline(VkDevice device, VkPhysicalDevice physical_device) {
    // TODO: Implement compute shader for layout
    // For now, return true as fallback to CPU
    return true;
}

bool GPULayoutEngine::create_layout_buffers(VkDevice device, VkPhysicalDevice physical_device) {
    // TODO: Implement Vulkan buffer creation
    // For now, return true as fallback to CPU
    return true;
}

void GPULayoutEngine::update_layout_buffer(const LayoutComputeData& data, VkDevice device) {
    // TODO: Implement buffer updates
}

} // namespace Shotcut::UI
