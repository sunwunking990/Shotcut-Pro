#pragma once

// Entity Component System - Core Entity Definitions
// Professional timeline editing system with high-performance ECS architecture

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

namespace Shotcut::Timeline {

// Forward declarations
class EntityManager;
class ComponentManager;
class SystemManager;

// Entity ID type - 32-bit for performance and scalability
using EntityID = uint32_t;

// Entity constants
constexpr EntityID INVALID_ENTITY = 0;
constexpr EntityID MAX_ENTITIES = 1000000; // 1M entities max

// Entity generation counter for handle validation
using Generation = uint32_t;

// Entity handle with generation validation
struct Entity {
    EntityID id;
    Generation generation;
    
    bool is_valid() const {
        return id != INVALID_ENTITY && generation != 0;
    }
    
    bool operator==(const Entity& other) const {
        return id == other.id && generation == other.generation;
    }
    
    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }
    
    bool operator<(const Entity& other) const {
        return id < other.id || (id == other.id && generation < other.generation);
    }
};

// Entity hash for unordered containers
struct EntityHash {
    std::size_t operator()(const Entity& entity) const {
        return static_cast<std::size_t>(entity.id) ^ 
               (static_cast<std::size_t>(entity.generation) << 32);
    }
};

// Component type ID system
using ComponentTypeID = uint32_t;

// Component type registry for unique IDs
class ComponentTypeRegistry {
private:
    static ComponentTypeID next_type_id_;
    
public:
    template<typename T>
    static ComponentTypeID get_type_id() {
        static ComponentTypeID type_id = next_type_id_++;
        return type_id;
    }
    
    static ComponentTypeID get_next_type_id() {
        return next_type_id_;
    }
};

// Base component class - all components inherit from this
struct Component {
    virtual ~Component() = default;
    
    // Component metadata
    virtual const char* get_name() const = 0;
    virtual ComponentTypeID get_type_id() const = 0;
    virtual bool is_serializable() const { return true; }
    
    // Lifecycle callbacks
    virtual void on_create() {}
    virtual void on_destroy() {}
    virtual void on_activate() {}
    virtual void on_deactivate() {}
};

// Component wrapper for type safety
template<typename T>
struct ComponentWrapper : public Component {
    static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
    
    T data;
    
    ComponentWrapper() = default;
    ComponentWrapper(const T& value) : data(value) {}
    ComponentWrapper(T&& value) : data(std::move(value)) {}
    
    const char* get_name() const override {
        return typeid(T).name();
    }
    
    ComponentTypeID get_type_id() const override {
        return ComponentTypeRegistry::get_type_id<T>();
    }
    
    // Operator overloads for easy access
    T& operator*() { return data; }
    const T& operator*() const { return data; }
    T* operator->() { return &data; }
    const T* operator->() const { return &data; }
};

// Component creation helper
template<typename T, typename... Args>
std::unique_ptr<ComponentWrapper<T>> make_component(Args&&... args) {
    return std::make_unique<ComponentWrapper<T>>(T{std::forward<Args>(args)...});
}

// Entity query builder for efficient component access
class EntityQuery {
private:
    std::vector<ComponentTypeID> required_components_;
    std::vector<ComponentTypeID> excluded_components_;
    std::vector<ComponentTypeID> optional_components_;
    
public:
    EntityQuery& require_component(ComponentTypeID type_id) {
        required_components_.push_back(type_id);
        return *this;
    }
    
    template<typename T>
    EntityQuery& require_component() {
        return require_component(ComponentTypeRegistry::get_type_id<T>());
    }
    
    EntityQuery& exclude_component(ComponentTypeID type_id) {
        excluded_components_.push_back(type_id);
        return *this;
    }
    
    template<typename T>
    EntityQuery& exclude_component() {
        return exclude_component(ComponentTypeRegistry::get_type_id<T>());
    }
    
    EntityQuery& optional_component(ComponentTypeID type_id) {
        optional_components_.push_back(type_id);
        return *this;
    }
    
    template<typename T>
    EntityQuery& optional_component() {
        return optional_component(ComponentTypeRegistry::get_type_id<T>());
    }
    
    // Accessors for system manager
    const std::vector<ComponentTypeID>& get_required() const { return required_components_; }
    const std::vector<ComponentTypeID>& get_excluded() const { return excluded_components_; }
    const std::vector<ComponentTypeID>& get_optional() const { return optional_components_; }
    
    bool matches(const std::vector<ComponentTypeID>& entity_components) const {
        // Check required components
        for (ComponentTypeID required : required_components_) {
            bool found = false;
            for (ComponentTypeID entity_comp : entity_components) {
                if (entity_comp == required) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        
        // Check excluded components
        for (ComponentTypeID excluded : excluded_components_) {
            for (ComponentTypeID entity_comp : entity_components) {
                if (entity_comp == excluded) {
                    return false;
                }
            }
        }
        
        return true;
    }
};

// Entity handle with automatic validation
class EntityHandle {
private:
    Entity entity_;
    EntityManager* manager_;
    
public:
    EntityHandle(Entity entity, EntityManager* manager) 
        : entity_(entity), manager_(manager) {}
    
    // Check if entity is still valid
    bool is_valid() const;
    
    // Get entity ID
    EntityID get_id() const { return entity_.id; }
    Generation get_generation() const { return entity_.generation; }
    Entity get_entity() const { return entity_; }
    
    // Component access
    template<typename T, typename... Args>
    bool add_component(Args&&... args);
    
    template<typename T>
    bool remove_component();
    
    template<typename T>
    bool has_component() const;
    
    template<typename T>
    T* get_component();
    
    template<typename T>
    const T* get_component() const;
    
    // Entity lifecycle
    bool destroy();
    bool is_alive() const;
    
    // Convenience operators
    explicit operator bool() const { return is_valid(); }
    bool operator!() const { return !is_valid(); }
};

// Entity iterator for efficient traversal
class EntityIterator {
private:
    const std::vector<Entity>* entities_;
    size_t index_;
    EntityQuery query_;
    EntityManager* manager_;
    
public:
    EntityIterator(const std::vector<Entity>* entities, 
                  const EntityQuery& query, 
                  EntityManager* manager)
        : entities_(entities), index_(0), query_(query), manager_(manager) {
        advance_to_next_valid();
    }
    
    EntityIterator& operator++() {
        ++index_;
        advance_to_next_valid();
        return *this;
    }
    
    EntityHandle operator*() const;
    bool operator!=(const EntityIterator& other) const {
        return index_ != other.index_;
    }
    
    bool is_end() const {
        return index_ >= entities_->size();
    }
    
private:
    void advance_to_next_valid();
};

// Entity view for range-based for loops
class EntityView {
private:
    const std::vector<Entity>* entities_;
    EntityQuery query_;
    EntityManager* manager_;
    
public:
    EntityView(const std::vector<Entity>* entities, 
               const EntityQuery& query, 
               EntityManager* manager)
        : entities_(entities), query_(query), manager_(manager) {}
    
    EntityIterator begin() const {
        return EntityIterator(entities_, query_, manager_);
    }
    
    EntityIterator end() const {
        return EntityIterator(entities_, query_, manager_);
    }
};

} // namespace Shotcut::Timeline
