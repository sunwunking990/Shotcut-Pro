// Timeline ECS Demo - Professional Video Editing System Showcase
// Demonstrates Entity Component System for timeline operations

#include "include/timeline/entity.h"
#include "include/timeline/components.h"
#include "include/timeline/systems.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace Shotcut::Timeline;

// Demo helper functions
void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_entity_info(const World& world, Entity entity, const std::string& description) {
    std::cout << "  Entity " << entity.id << " (gen " << entity.generation << "): " << description << "\n";
    
    // Show components
    auto* clip = world.get_component<ClipComponent>(entity);
    if (clip) {
        std::cout << "    Clip: " << clip->name 
                  << " | Duration: " << clip->duration << "s"
                  << " | Track: " << clip->track_id 
                  << " | Time: " << clip->timeline_start_time << "s\n";
    }
    
    auto* video = world.get_component<VideoClipComponent>(entity);
    if (video) {
        std::cout << "    Video: " << video->width << "x" << video->height
                  << " | " << video->frame_rate << "fps"
                  << " | " << video->codec << "\n";
    }
    
    auto* audio = world.get_component<AudioClipComponent>(entity);
    if (audio) {
        std::cout << "    Audio: " << audio->sample_rate << "Hz"
                  << " | " << audio->channels << " channels"
                  << " | Volume: " << audio->volume << "\n";
    }
    
    auto* track = world.get_component<TrackComponent>(entity);
    if (track) {
        std::cout << "    Track: " << track->track_name
                  << " | Type: " << (track->is_video_track() ? "Video" : 
                                   track->is_audio_track() ? "Audio" : "Other")
                  << " | Height: " << track->height << "px\n";
    }
    
    auto* transform = world.get_component<TransformComponent>(entity);
    if (transform) {
        std::cout << "    Transform: Pos(" << transform->position.x << ", " << transform->position.y << ")"
                  << " | Scale(" << transform->scale.x << ", " << transform->scale.y << ")"
                  << " | Rotation: " << transform->rotation << "°\n";
    }
}

void demonstrate_basic_entity_creation(World& world) {
    print_separator("1. Basic Entity Creation");
    
    // Create some entities
    Entity video_clip = world.create_entity();
    Entity audio_clip = world.create_entity();
    Entity video_track = world.create_entity();
    Entity audio_track = world.create_entity();
    
    // Add components to video clip
    auto* clip_comp = world.add_component<ClipComponent>(video_clip);
    clip_comp->name = "Sample Video";
    clip_comp->duration = 10.5;
    clip_comp->timeline_start_time = 2.0;
    clip_comp->track_id = 1;
    clip_comp->is_selected = true;
    
    auto* video_comp = world.add_component<VideoClipComponent>(video_clip);
    video_comp->width = 1920;
    video_comp->height = 1080;
    video_comp->frame_rate = 30.0;
    video_comp->codec = "H.264";
    
    auto* transform_comp = world.add_component<TransformComponent>(video_clip);
    transform_comp->position = Vec2(100.0f, 50.0f);
    transform_comp->scale = Vec2(1.0f, 1.0f);
    transform_comp->rotation = 0.0f;
    
    // Add components to audio clip
    auto* audio_clip_comp = world.add_component<ClipComponent>(audio_clip);
    audio_clip_comp->name = "Background Music";
    audio_clip_comp->duration = 15.0;
    audio_clip_comp->timeline_start_time = 0.0;
    audio_clip_comp->track_id = 2;
    
    auto* audio_comp = world.add_component<AudioClipComponent>(audio_clip);
    audio_comp->sample_rate = 48000;
    audio_comp->channels = 2;
    audio_comp->volume = 0.8f;
    audio_comp->pan = 0.0f;
    
    // Add track components
    auto* video_track_comp = world.add_component<TrackComponent>(video_track, TrackComponent::Type::VIDEO);
    video_track_comp->track_name = "Video Track 1";
    video_track_comp->track_id = 1;
    video_track_comp->height = 120.0f;
    
    auto* audio_track_comp = world.add_component<TrackComponent>(audio_track, TrackComponent::Type::AUDIO);
    audio_track_comp->track_name = "Audio Track 1";
    audio_track_comp->track_id = 2;
    audio_track_comp->height = 80.0f;
    audio_track_comp->volume = 1.0f;
    
    // Print entity information
    print_entity_info(world, video_clip, "Video Clip Entity");
    print_entity_info(world, audio_clip, "Audio Clip Entity");
    print_entity_info(world, video_track, "Video Track Entity");
    print_entity_info(world, audio_track, "Audio Track Entity");
    
    std::cout << "\n  Total entities created: " << world.get_entity_manager()->get_entity_count() << "\n";
    std::cout << "  Total components: " << world.get_component_manager()->get_component_count() << "\n";
}

void demonstrate_component_queries(World& world) {
    print_separator("2. Component Queries");
    
    // Query for all entities with ClipComponent
    auto clip_query = EntityQuery().require_component<ClipComponent>();
    auto clip_entities = world.query_entities(clip_query);
    
    std::cout << "  Entities with ClipComponent:\n";
    for (auto it = clip_entities.begin(); it != clip_entities.end(); ++it) {
        auto handle = *it;
        if (handle.is_valid()) {
            auto* clip = handle.get_component<ClipComponent>();
            if (clip) {
                std::cout << "    - " << clip->name << " (Entity " << handle.get_id() << ")\n";
            }
        }
    }
    
    // Query for all entities with VideoClipComponent
    auto video_query = EntityQuery().require_component<VideoClipComponent>();
    auto video_entities = world.query_entities(video_query);
    
    std::cout << "\n  Entities with VideoClipComponent:\n";
    for (auto it = video_entities.begin(); it != video_entities.end(); ++it) {
        auto handle = *it;
        if (handle.is_valid()) {
            auto* video = handle.get_component<VideoClipComponent>();
            if (video) {
                std::cout << "    - " << video->width << "x" << video->height 
                          << " video (Entity " << handle.get_id() << ")\n";
            }
        }
    }
    
    // Query for tracks only
    auto track_query = EntityQuery().require_component<TrackComponent>();
    auto track_entities = world.query_entities(track_query);
    
    std::cout << "\n  Track entities:\n";
    for (auto it = track_entities.begin(); it != track_entities.end(); ++it) {
        auto handle = *it;
        if (handle.is_valid()) {
            auto* track = handle.get_component<TrackComponent>();
            if (track) {
                std::cout << "    - " << track->track_name 
                          << " (" << (track->is_video_track() ? "Video" : "Audio") << ")\n";
            }
        }
    }
}

void demonstrate_component_modification(World& world) {
    print_separator("3. Component Modification");
    
    // Find a video clip entity
    auto video_query = EntityQuery().require_component<VideoClipComponent>();
    auto video_entities = world.query_entities(video_query);
    
    if (video_entities.begin() != video_entities.end()) {
        auto video_handle = *video_entities.begin();
        
        // Modify transform component
        auto* transform = video_handle.get_component<TransformComponent>();
        if (transform) {
            std::cout << "  Original transform:\n";
            std::cout << "    Position: (" << transform->position.x << ", " << transform->position.y << ")\n";
            std::cout << "    Scale: (" << transform->scale.x << ", " << transform->scale.y << ")\n";
            std::cout << "    Rotation: " << transform->rotation << "°\n";
            
            // Apply changes
            transform->position.x += 50.0f;
            transform->scale.x = 1.5f;
            transform->rotation = 15.0f;
            
            std::cout << "\n  Modified transform:\n";
            std::cout << "    Position: (" << transform->position.x << ", " << transform->position.y << ")\n";
            std::cout << "    Scale: (" << transform->scale.x << ", " << transform->scale.y << ")\n";
            std::cout << "    Rotation: " << transform->rotation << "°\n";
        }
        
        // Add an effect component dynamically
        auto* effect = world.add_component<EffectComponent>(video_handle.get_entity());
        effect->effect_name = "Color Correction";
        effect->effect_type = "video";
        effect->is_enabled = true;
        effect->start_time = 0.0;
        effect->duration = 10.5;
        effect->set_parameter("brightness", 0.2f);
        effect->set_parameter("contrast", 1.1f);
        
        std::cout << "\n  Added effect component: " << effect->effect_name << "\n";
        std::cout << "    Parameters: brightness=" << effect->get_parameter<float>("brightness", 0.0f)
                  << ", contrast=" << effect->get_parameter<float>("contrast", 1.0f) << "\n";
    }
}

void demonstrate_entity_destruction(World& world) {
    print_separator("4. Entity Destruction");
    
    std::cout << "  Before destruction:\n";
    std::cout << "    Total entities: " << world.get_entity_manager()->get_entity_count() << "\n";
    std::cout << "    Total components: " << world.get_component_manager()->get_component_count() << "\n";
    
    // Find and destroy an audio clip
    auto audio_query = EntityQuery().require_component<AudioClipComponent>();
    auto audio_entities = world.query_entities(audio_query);
    
    if (audio_entities.begin() != audio_entities.end()) {
        auto audio_handle = *audio_entities.begin();
        std::cout << "\n  Destroying audio clip entity " << audio_handle.get_id() << "\n";
        
        world.destroy_entity(audio_handle.get_entity());
        
        std::cout << "\n  After destruction (before cleanup):\n";
        std::cout << "    Total entities: " << world.get_entity_manager()->get_entity_count() << "\n";
        std::cout << "    Total components: " << world.get_component_manager()->get_component_count() << "\n";
        
        // Update world to trigger cleanup
        world.update(0.016); // ~60fps
        
        std::cout << "\n  After cleanup:\n";
        std::cout << "    Total entities: " << world.get_entity_manager()->get_entity_count() << "\n";
        std::cout << "    Total components: " << world.get_component_manager()->get_component_count() << "\n";
    }
}

void demonstrate_performance_metrics(World& world) {
    print_separator("5. Performance Metrics");
    
    const int num_entities = 1000;
    const int iterations = 100;
    
    std::cout << "  Performance test with " << num_entities << " entities...\n";
    
    // Create many entities
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<Entity> test_entities;
    for (int i = 0; i < num_entities; ++i) {
        Entity entity = world.create_entity();
        world.add_component<ClipComponent>(entity);
        world.add_component<TransformComponent>(entity);
        
        if (i % 2 == 0) {
            world.add_component<VideoClipComponent>(entity);
        }
        
        test_entities.push_back(entity);
    }
    
    auto creation_end = std::chrono::high_resolution_clock::now();
    auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(creation_end - start);
    
    std::cout << "    Entity creation time: " << creation_time.count() << " μs\n";
    std::cout << "    Average per entity: " << (creation_time.count() / num_entities) << " μs\n";
    
    // Query performance test
    auto query_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto query = EntityQuery().require_component<ClipComponent>().require_component<TransformComponent>();
        auto results = world.query_entities(query);
        
        // Simulate processing
        int count = 0;
        for (auto it = results.begin(); it != results.end(); ++it) {
            count++;
        }
    }
    
    auto query_end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(query_end - query_start);
    
    std::cout << "    Query time (" << iterations << " iterations): " << query_time.count() << " μs\n";
    std::cout << "    Average per query: " << (query_time.count() / iterations) << " μs\n";
    
    // Component access performance test
    auto access_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        for (Entity entity : test_entities) {
            auto* clip = world.get_component<ClipComponent>(entity);
            auto* transform = world.get_component<TransformComponent>(entity);
            
            if (clip && transform) {
                // Simulate some work
                transform->position.x += 0.1f;
            }
        }
    }
    
    auto access_end = std::chrono::high_resolution_clock::now();
    auto access_time = std::chrono::duration_cast<std::chrono::microseconds>(access_end - access_start);
    
    std::cout << "    Component access time (" << iterations << " iterations): " << access_time.count() << " μs\n";
    std::cout << "    Average per iteration: " << (access_time.count() / iterations) << " μs\n";
    
    // Cleanup test entities
    for (Entity entity : test_entities) {
        world.destroy_entity(entity);
    }
    world.update(0.016); // Trigger cleanup
    
    std::cout << "\n  Final entity count: " << world.get_entity_manager()->get_entity_count() << "\n";
    std::cout << "  Final component count: " << world.get_component_manager()->get_component_count() << "\n";
}

int main() {
    std::cout << "===========================================================\n";
    std::cout << "  Shotcut C++ Timeline ECS Demo\n";
    std::cout << "  Professional Video Editing System Showcase\n";
    std::cout << "===========================================================\n";
    
    try {
        // Create ECS world
        World world;
        world.initialize();
        
        // Run demonstrations
        demonstrate_basic_entity_creation(world);
        demonstrate_component_queries(world);
        demonstrate_component_modification(world);
        demonstrate_entity_destruction(world);
        demonstrate_performance_metrics(world);
        
        print_separator("Demo Complete");
        std::cout << "  All ECS demonstrations completed successfully!\n";
        std::cout << "  The Entity Component System is ready for integration\n";
        std::cout << "  with the Shotcut video editing pipeline.\n";
        
        // Final cleanup
        world.cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
