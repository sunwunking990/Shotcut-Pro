// Entity Component System - Implementation
// Professional timeline editing system with high-performance ECS architecture

#include "timeline/entity.h"
#include "timeline/systems.h"
#include <algorithm>
#include <stdexcept>

namespace Shotcut::Timeline {

// ============================================================================
// COMPONENT TYPE REGISTRY IMPLEMENTATION
// ============================================================================

ComponentTypeID ComponentTypeRegistry::next_type_id_ = 1;

// ============================================================================
// ENTITY MANAGER IMPLEMENTATION
// ============================================================================

Entity EntityManager::create_entity() {
    EntityID id = allocate_id();
    Generation generation = 1;
    
    // Ensure generation vector is large enough
    if (id >= generations_.size()) {
        generations_.resize(id + 1, 0);
    }
    
    generations_[id] = generation;
    
    Entity entity{id, generation};
    entities_.push_back(entity);
    
    return entity;
}

void EntityManager::destroy_entity(Entity entity) {
    if (!is_entity_alive(entity)) return;
    
    // Mark generation as invalid
    if (entity.id < generations_.size()) {
        generations_[entity.id] = 0;
    }
    
    // Add ID to free list
    deallocate_id(entity.id);
    
    // Remove from entities vector (will be cleaned up in cleanup_destroyed_entities)
    // This is delayed for performance reasons
}

bool EntityManager::is_entity_alive(Entity entity) const {
    if (entity.id == INVALID_ENTITY) return false;
    if (entity.id >= generations_.size()) return false;
    return generations_[entity.id] == entity.generation;
}

Entity EntityManager::get_entity(EntityID id) const {
    if (id >= generations_.size()) return {INVALID_ENTITY, 0};
    return {id, generations_[id]};
}

void EntityManager::cleanup_destroyed_entities() {
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [this](const Entity& entity) {
                return !is_entity_alive(entity);
            }),
        entities_.end()
    );
}

EntityID EntityManager::allocate_id() {
    if (!free_ids_.empty()) {
        EntityID id = free_ids_.back();
        free_ids_.pop_back();
        return id;
    }
    
    if (next_entity_id_ >= MAX_ENTITIES) {
        throw std::runtime_error("Maximum entity count reached");
    }
    
    return next_entity_id_++;
}

void EntityManager::deallocate_id(EntityID id) {
    if (id != INVALID_ENTITY) {
        free_ids_.push_back(id);
    }
}

// ============================================================================
// COMPONENT MANAGER IMPLEMENTATION
// ============================================================================

const std::vector<ComponentTypeID>& ComponentManager::get_entity_components(Entity entity) const {
    static const std::vector<ComponentTypeID> empty;
    auto it = entity_components_.find(entity);
    return it != entity_components_.end() ? it->second : empty;
}

std::vector<Entity> ComponentManager::get_entities_with_component(ComponentTypeID type_id) const {
    std::vector<Entity> entities;
    
    auto type_it = components_.find(type_id);
    if (type_it != components_.end()) {
        for (const auto& entity_component_pair : type_it->second) {
            entities.push_back(entity_component_pair.first);
        }
    }
    
    return entities;
}

void ComponentManager::remove_all_components(Entity entity) {
    auto entity_comps_it = entity_components_.find(entity);
    if (entity_comps_it == entity_components_.end()) return;
    
    const auto& component_types = entity_comps_it->second;
    for (ComponentTypeID type_id : component_types) {
        auto type_it = components_.find(type_id);
        if (type_it != components_.end()) {
            auto entity_it = type_it->second.find(entity);
            if (entity_it != type_it->second.end()) {
                entity_it->second->on_destroy();
                type_it->second.erase(entity_it);
            }
        }
    }
    
    entity_components_.erase(entity_comps_it);
}

void ComponentManager::cleanup_components() {
    // Remove components for destroyed entities
    std::vector<Entity> destroyed_entities;
    
    for (const auto& entity_components_pair : entity_components_) {
        // Check if entity is marked as destroyed (generation = 0)
        if (entity_components_pair.first.generation == 0) {
            destroyed_entities.push_back(entity_components_pair.first);
        }
    }
    
    for (Entity entity : destroyed_entities) {
        remove_all_components(entity);
    }
}

size_t ComponentManager::get_component_count() const {
    size_t total = 0;
    for (const auto& type_components_pair : components_) {
        total += type_components_pair.second.size();
    }
    return total;
}

size_t ComponentManager::get_component_count(ComponentTypeID type_id) const {
    auto it = components_.find(type_id);
    return it != components_.end() ? it->second.size() : 0;
}

// ============================================================================
// SYSTEM MANAGER IMPLEMENTATION
// ============================================================================

System* SystemManager::get_system(const char* name) {
    auto it = system_map_.find(name);
    return it != system_map_.end() ? it->second : nullptr;
}

const System* SystemManager::get_system(const char* name) const {
    auto it = system_map_.find(name);
    return it != system_map_.end() ? it->second : nullptr;
}

void SystemManager::initialize_all() {
    if (is_initialized_) return;
    
    // Sort systems by priority before initialization
    sort_systems_by_priority();
    
    for (auto& system : systems_) {
        if (system && system->is_enabled()) {
            system->initialize();
        }
    }
    
    is_initialized_ = true;
}

void SystemManager::update_all(double delta_time) {
    for (auto& system : systems_) {
        if (system && system->is_enabled()) {
            system->update(delta_time);
        }
    }
}

void SystemManager::cleanup_all() {
    for (auto& system : systems_) {
        if (system) {
            system->cleanup();
        }
    }
    systems_.clear();
    system_map_.clear();
    is_initialized_ = false;
}

void SystemManager::enable_system(const char* name) {
    System* system = get_system(name);
    if (system) {
        system->set_enabled(true);
    }
}

void SystemManager::disable_system(const char* name) {
    System* system = get_system(name);
    if (system) {
        system->set_enabled(false);
    }
}

void SystemManager::set_system_priority(const char* name, int priority) {
    System* system = get_system(name);
    if (system) {
        // Note: This requires re-sorting systems
        // For simplicity, we'll implement this as a no-op for now
        // In a full implementation, this would update priority and re-sort
    }
}

void SystemManager::sort_systems_by_priority() {
    std::sort(systems_.begin(), systems_.end(),
        [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
            return a->get_priority() < b->get_priority();
        });
}

void SystemManager::add_system_to_map(System* system) {
    if (system) {
        system_map_[system->get_name()] = system;
    }
}

// ============================================================================
// WORLD IMPLEMENTATION
// ============================================================================

World::World()
    : entity_manager_(std::make_unique<EntityManager>())
    , component_manager_(std::make_unique<ComponentManager>())
    , system_manager_(std::make_unique<SystemManager>()) {
}

World::~World() {
    cleanup();
}

void World::initialize() {
    if (is_initialized_) return;
    
    // Initialize all systems
    system_manager_->initialize_all();
    
    is_initialized_ = true;
}

void World::update(double delta_time) {
    if (!is_initialized_) {
        initialize();
    }
    
    // Apply time scale
    double scaled_delta_time = delta_time * time_scale_;
    
    // Update all systems
    system_manager_->update_all(scaled_delta_time);
    
    // Cleanup destroyed entities and components
    entity_manager_->cleanup_destroyed_entities();
    component_manager_->cleanup_components();
}

void World::cleanup() {
    if (!is_initialized_) return;
    
    system_manager_->cleanup_all();
    component_manager_->cleanup_components();
    
    is_initialized_ = false;
}

Entity World::create_entity() {
    return entity_manager_->create_entity();
}

void World::destroy_entity(Entity entity) {
    component_manager_->remove_all_components(entity);
    entity_manager_->destroy_entity(entity);
}

bool World::is_entity_alive(Entity entity) const {
    return entity_manager_->is_entity_alive(entity);
}

} // namespace Shotcut::Timeline
