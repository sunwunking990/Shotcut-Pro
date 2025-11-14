#pragma once

// Timeline Systems - Entity Component System Processing Pipeline
// Professional video editing systems for processing timeline entities

#include "entity.h"
#include "components.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace Shotcut::Timeline {

// Forward declarations
class EntityManager;
class ComponentManager;
class SystemManager;

// ============================================================================
// BASE SYSTEM INTERFACE
// ============================================================================

// Base system class - all systems inherit from this
class System {
public:
    virtual ~System() = default;
    
    // System lifecycle
    virtual void initialize() {}
    virtual void update(double delta_time) = 0;
    virtual void cleanup() {}
    
    // System information
    virtual const char* get_name() const = 0;
    virtual bool is_enabled() const { return is_enabled_; }
    virtual void set_enabled(bool enabled) { is_enabled_ = enabled; }
    
    // System execution order
    virtual int get_priority() const { return 0; } // Higher = later execution
    
protected:
    bool is_enabled_ = true;
    double delta_time_ = 0.0;
};

// System factory function type
using SystemFactory = std::function<std::unique_ptr<System>()>;

// ============================================================================
// ENTITY MANAGER
// ============================================================================

// Manages entity creation, destruction, and lifecycle
class EntityManager {
private:
    std::vector<Entity> entities_;
    std::vector<Generation> generations_;
    std::vector<EntityID> free_ids_;
    EntityID next_entity_id_ = 1;
    
public:
    Entity create_entity();
    void destroy_entity(Entity entity);
    bool is_entity_alive(Entity entity) const;
    
    const std::vector<Entity>& get_entities() const { return entities_; }
    size_t get_entity_count() const { return entities_.size(); }
    
    // Entity lookup by ID
    Entity get_entity(EntityID id) const;
    
    // Cleanup destroyed entities
    void cleanup_destroyed_entities();
    
private:
    EntityID allocate_id();
    void deallocate_id(EntityID id);
};

// ============================================================================
// COMPONENT MANAGER
// ============================================================================

// Manages component storage and access
class ComponentManager {
private:
    // Component storage by type
    std::unordered_map<ComponentTypeID, std::unordered_map<Entity, std::unique_ptr<Component>, EntityHash>> components_;
    
    // Entity component mapping for quick lookups
    std::unordered_map<Entity, std::vector<ComponentTypeID>, EntityHash> entity_components_;
    
public:
    // Component management
    template<typename T, typename... Args>
    T* add_component(Entity entity, Args&&... args);
    
    template<typename T>
    bool remove_component(Entity entity);
    
    template<typename T>
    T* get_component(Entity entity);
    
    template<typename T>
    const T* get_component(Entity entity) const;
    
    template<typename T>
    bool has_component(Entity entity) const;
    
    // Entity component queries
    const std::vector<ComponentTypeID>& get_entity_components(Entity entity) const;
    std::vector<Entity> get_entities_with_component(ComponentTypeID type_id) const;
    
    // Component cleanup
    void remove_all_components(Entity entity);
    void cleanup_components();
    
    // Statistics
    size_t get_component_count() const;
    size_t get_component_count(ComponentTypeID type_id) const;
};

// ============================================================================
// SYSTEM MANAGER
// ============================================================================

// Manages system execution and dependencies
class SystemManager {
private:
    std::vector<std::unique_ptr<System>> systems_;
    std::unordered_map<const char*, System*> system_map_;
    bool is_initialized_ = false;
    
public:
    // System registration
    template<typename T, typename... Args>
    T* register_system(Args&&... args);
    
    System* get_system(const char* name);
    const System* get_system(const char* name) const;
    
    // System execution
    void initialize_all();
    void update_all(double delta_time);
    void cleanup_all();
    
    // System management
    void enable_system(const char* name);
    void disable_system(const char* name);
    void set_system_priority(const char* name, int priority);
    
    // System queries
    const std::vector<std::unique_ptr<System>>& get_systems() const { return systems_; }
    size_t get_system_count() const { return systems_.size(); }
    
    // Execution order optimization
    void sort_systems_by_priority();
    
private:
    void add_system_to_map(System* system);
};

// ============================================================================
// TIMELINE SYSTEMS
// ============================================================================

// Timeline management system - handles track and clip organization
class TimelineSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
    // Timeline properties
    double current_time_ = 0.0;
    double duration_ = 0.0;
    double zoom_level_ = 1.0;
    double playback_speed_ = 1.0;
    
    // Track organization
    std::vector<Entity> tracks_;
    std::unordered_map<int, std::vector<Entity>> track_clips_;
    
public:
    TimelineSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "TimelineSystem"; }
    
    // Timeline operations
    Entity create_track(TrackComponent::Type type, const std::string& name);
    void add_clip_to_track(Entity clip, Entity track);
    void remove_clip_from_track(Entity clip);
    
    // Timeline navigation
    void set_current_time(double time) { current_time_ = time; }
    double get_current_time() const { return current_time_; }
    void set_zoom_level(double zoom) { zoom_level_ = zoom; }
    double get_zoom_level() const { return zoom_level_; }
    
    // Timeline queries
    std::vector<Entity> get_clips_at_time(double time) const;
    std::vector<Entity> get_clips_in_range(double start, double end) const;
    Entity get_track_by_id(int track_id) const;
    
    // Timeline properties
    double calculate_duration() const;
    void update_duration();
    
private:
    void organize_clips_by_track();
    void update_track_indices();
};

// Clip processing system - handles clip operations and effects
class ClipProcessingSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
    // Processing state
    std::vector<Entity> active_clips_;
    std::vector<Entity> selected_clips_;
    
public:
    ClipProcessingSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "ClipProcessingSystem"; }
    int get_priority() const override { return 100; } // High priority
    
    // Clip operations
    void select_clip(Entity clip);
    void deselect_clip(Entity clip);
    void select_clips_in_range(double start, double end);
    void clear_selection();
    
    // Clip manipulation
    void move_clip(Entity clip, double new_time, int new_track = -1);
    void trim_clip(Entity clip, double new_start, double new_duration);
    void split_clip(Entity clip, double split_time);
    
    // Clip queries
    const std::vector<Entity>& get_active_clips() const { return active_clips_; }
    const std::vector<Entity>& get_selected_clips() const { return selected_clips_; }
    Entity get_clip_at_position(double time, int track_id) const;
    
    // Clip validation
    bool is_clip_placement_valid(Entity clip, double time, int track_id) const;
    std::vector<Entity> get_overlapping_clips(Entity clip) const;
    
private:
    void update_active_clips();
    void validate_clip_positions();
};

// Transform system - handles clip transformations
class TransformSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
public:
    TransformSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "TransformSystem"; }
    int get_priority() const override { return 200; } // After clip processing
    
    // Transform operations
    void set_clip_transform(Entity clip, const Vec2& position, const Vec2& scale, float rotation);
    void apply_transform_to_clip(Entity clip, const TransformComponent& transform);
    
    // Transform queries
    TransformComponent* get_clip_transform(Entity clip);
    bool has_clip_transform(Entity clip) const;
    
private:
    void update_transforms();
    void apply_motion_tracking(Entity clip, double current_time);
};

// Effect processing system - handles effects and filters
class EffectProcessingSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
    // Effect chains
    std::unordered_map<Entity, std::vector<Entity>> clip_effects_;
    
public:
    EffectProcessingSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "EffectProcessingSystem"; }
    int get_priority() const override { return 300; } // After transforms
    
    // Effect management
    Entity add_effect_to_clip(Entity clip, const std::string& effect_name, const std::string& effect_type);
    void remove_effect_from_clip(Entity clip, Entity effect);
    void reorder_effects(Entity clip, const std::vector<Entity>& new_order);
    
    // Effect operations
    void enable_effect(Entity effect, bool enabled = true);
    void set_effect_parameter(Entity effect, const std::string& name, const std::string& value);
    
    // Effect queries
    std::vector<Entity> get_clip_effects(Entity clip) const;
    Entity get_effect_by_name(Entity clip, const std::string& name) const;
    
private:
    void update_effect_chains();
    void process_effect(Entity effect, double current_time);
};

// Audio processing system - handles audio mixing and processing
class AudioProcessingSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
    // Audio state
    std::vector<Entity> active_audio_clips_;
    float master_volume_ = 1.0f;
    
public:
    AudioProcessingSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "AudioProcessingSystem"; }
    int get_priority() const override { return 400; } // After effects
    
    // Audio operations
    void set_master_volume(float volume) { master_volume_ = volume; }
    float get_master_volume() const { return master_volume_; }
    
    void mute_track(int track_id, bool muted = true);
    void solo_track(int track_id, bool solo = true);
    
    // Audio queries
    std::vector<Entity> get_active_audio_clips() const;
    float calculate_mix_level(Entity clip) const;
    
private:
    void update_active_audio_clips();
    void mix_audio();
    void apply_audio_filters(Entity clip);
};

// Render system - handles final composition and output
class RenderSystem : public System {
private:
    EntityManager* entity_manager_;
    ComponentManager* component_manager_;
    
    // Render state
    double render_start_time_ = 0.0;
    double render_end_time_ = 0.0;
    bool is_rendering_ = false;
    int current_frame_ = 0;
    
public:
    RenderSystem(EntityManager* em, ComponentManager* cm);
    
    void initialize() override;
    void update(double delta_time) override;
    void cleanup() override;
    
    const char* get_name() const override { return "RenderSystem"; }
    int get_priority() const override { return 1000; } // Always last
    
    // Render operations
    void start_render(double start_time, double end_time);
    void stop_render();
    bool is_rendering() const { return is_rendering_; }
    
    // Render queries
    double get_render_progress() const;
    int get_current_frame() const { return current_frame_; }
    
private:
    void render_frame();
    void composite_clips();
    void apply_final_effects();
};

// ============================================================================
// WORLD - ECS COORDINATOR
// ============================================================================

// Main ECS coordinator - ties together all managers and systems
class World {
private:
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<ComponentManager> component_manager_;
    std::unique_ptr<SystemManager> system_manager_;
    
    bool is_initialized_ = false;
    double time_scale_ = 1.0;
    
public:
    World();
    ~World();
    
    // World lifecycle
    void initialize();
    void update(double delta_time);
    void cleanup();
    
    // Entity management
    Entity create_entity();
    void destroy_entity(Entity entity);
    bool is_entity_alive(Entity entity) const;
    
    // Component management
    template<typename T, typename... Args>
    T* add_component(Entity entity, Args&&... args);
    
    template<typename T>
    bool remove_component(Entity entity);
    
    template<typename T>
    T* get_component(Entity entity);
    
    template<typename T>
    const T* get_component(Entity entity) const;
    
    template<typename T>
    bool has_component(Entity entity) const;
    
    // System management
    template<typename T, typename... Args>
    T* register_system(Args&&... args);
    
    template<typename T>
    T* get_system();
    
    // Entity queries
    EntityView query_entities(const EntityQuery& query);
    
    // Time management
    void set_time_scale(double scale) { time_scale_ = scale; }
    double get_time_scale() const { return time_scale_; }
    
    // Manager access
    EntityManager* get_entity_manager() const { return entity_manager_.get(); }
    ComponentManager* get_component_manager() const { return component_manager_.get(); }
    SystemManager* get_system_manager() const { return system_manager_.get(); }
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

// ComponentManager template implementations
template<typename T, typename... Args>
T* ComponentManager::add_component(Entity entity, Args&&... args) {
    ComponentTypeID type_id = ComponentTypeRegistry::get_type_id<T>();
    
    auto component = std::make_unique<ComponentWrapper<T>>(T{std::forward<Args>(args)...});
    component->on_create();
    
    T* ptr = &static_cast<ComponentWrapper<T>*>(component.get())->data;
    components_[type_id][entity] = std::move(component);
    
    // Update entity component mapping
    auto& entity_comps = entity_components_[entity];
    if (std::find(entity_comps.begin(), entity_comps.end(), type_id) == entity_comps.end()) {
        entity_comps.push_back(type_id);
    }
    
    return ptr;
}

template<typename T>
bool ComponentManager::remove_component(Entity entity) {
    ComponentTypeID type_id = ComponentTypeRegistry::get_type_id<T>();
    
    auto type_it = components_.find(type_id);
    if (type_it == components_.end()) return false;
    
    auto entity_it = type_it->second.find(entity);
    if (entity_it == type_it->second.end()) return false;
    
    entity_it->second->on_destroy();
    type_it->second.erase(entity_it);
    
    // Update entity component mapping
    auto entity_comps_it = entity_components_.find(entity);
    if (entity_comps_it != entity_components_.end()) {
        auto& comps = entity_comps_it->second;
        comps.erase(std::remove(comps.begin(), comps.end(), type_id), comps.end());
    }
    
    return true;
}

template<typename T>
T* ComponentManager::get_component(Entity entity) {
    ComponentTypeID type_id = ComponentTypeRegistry::get_type_id<T>();
    
    auto type_it = components_.find(type_id);
    if (type_it == components_.end()) return nullptr;
    
    auto entity_it = type_it->second.find(entity);
    if (entity_it == type_it->second.end()) return nullptr;
    
    return &static_cast<ComponentWrapper<T>*>(entity_it->second.get())->data;
}

template<typename T>
const T* ComponentManager::get_component(Entity entity) const {
    return const_cast<ComponentManager*>(this)->get_component<T>(entity);
}

template<typename T>
bool ComponentManager::has_component(Entity entity) const {
    ComponentTypeID type_id = ComponentTypeRegistry::get_type_id<T>();
    
    auto entity_comps_it = entity_components_.find(entity);
    if (entity_comps_it == entity_components_.end()) return false;
    
    const auto& comps = entity_comps_it->second;
    return std::find(comps.begin(), comps.end(), type_id) != comps.end();
}

// SystemManager template implementations
template<typename T, typename... Args>
T* SystemManager::register_system(Args&&... args) {
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    
    add_system_to_map(ptr);
    systems_.push_back(std::move(system));
    
    return ptr;
}

template<typename T>
T* SystemManager::get_system(const char* name) {
    auto it = system_map_.find(name);
    return it != system_map_.end() ? static_cast<T*>(it->second) : nullptr;
}

// World template implementations
template<typename T, typename... Args>
T* World::add_component(Entity entity, Args&&... args) {
    return component_manager_->add_component<T>(entity, std::forward<Args>(args)...);
}

template<typename T>
bool World::remove_component(Entity entity) {
    return component_manager_->remove_component<T>(entity);
}

template<typename T>
T* World::get_component(Entity entity) {
    return component_manager_->get_component<T>(entity);
}

template<typename T>
const T* World::get_component(Entity entity) const {
    return component_manager_->get_component<T>(entity);
}

template<typename T>
bool World::has_component(Entity entity) const {
    return component_manager_->has_component<T>(entity);
}

template<typename T, typename... Args>
T* World::register_system(Args&&... args) {
    return system_manager_->register_system<T>(std::forward<Args>(args)...);
}

template<typename T>
T* World::get_system() {
    T temp_instance;
    return static_cast<T*>(system_manager_->get_system(temp_instance.get_name()));
}

} // namespace Shotcut::Timeline
