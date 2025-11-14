// Phase 5 Effects & Audio Processing Demo
// Comprehensive demonstration of GPU effects and audio processing capabilities

#include "../include/effects/effect_system.h"
#include "../include/audio/audio_processor.h"
#include "../include/timeline/entity.h"
#include "../include/timeline/components.h"
#include <iostream>
#include <chrono>
#include <random>

using namespace Shotcut;
using namespace std::chrono;

// Demo configuration
struct DemoConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t audio_sample_rate = 48000;
    uint32_t audio_channels = 2;
    float demo_duration = 10.0f;
    bool enable_gpu_effects = true;
    bool enable_audio_processing = true;
};

// Demo state tracking
struct DemoState {
    float current_time = 0.0f;
    uint32_t frame_count = 0;
    bool running = true;
    
    // Performance tracking
    float avg_fps = 0.0f;
    float avg_audio_latency = 0.0f;
    size_t total_frames_processed = 0;
    
    // System instances
    std::unique_ptr<Effects::EffectManager> effect_manager;
    std::unique_ptr<Audio::AudioProcessor> audio_processor;
    std::unique_ptr<Timeline::World> timeline_world;
    
    // Demo entities and effects
    Timeline::Entity demo_entity;
    Effects::EffectChain video_effects_chain;
    std::vector<std::shared_ptr<Audio::AudioChannel>> audio_channels;
};

// Mock Vulkan context for demo (without actual Vulkan)
class MockVulkanContext {
public:
    VkDevice get_device() const { return reinterpret_cast<VkDevice>(0x12345678); }
    bool is_valid() const { return true; }
};

// Generate demo audio data
void generate_demo_audio_data(Audio::AudioBuffer& buffer, float frequency, float duration) {
    const Audio::AudioBufferDesc& desc = buffer.get_desc();
    float* samples = static_cast<float*>(buffer.get_data());
    
    float phase = 0.0f;
    float phase_increment = (2.0f * 3.14159265f * frequency) / static_cast<float>(desc.sample_rate);
    
    for (uint32_t frame = 0; frame < desc.frames; ++frame) {
        float sample_value = std::sin(phase) * 0.5f; // 50% volume
        
        // Fill both channels
        for (uint32_t ch = 0; ch < desc.channels; ++ch) {
            samples[frame * desc.channels + ch] = sample_value;
        }
        
        phase += phase_increment;
        if (phase > 2.0f * 3.14159265f) {
            phase -= 2.0f * 3.14159265f;
        }
    }
}

// Initialize demo systems
bool initialize_demo(const DemoConfig& config, DemoState& state) {
    std::cout << "=== Phase 5 Effects & Audio Processing Demo ===" << std::endl;
    std::cout << "Initializing systems..." << std::endl;
    
    // Initialize effect manager
    state.effect_manager = std::make_unique<Effects::EffectManager>();
    
    // Initialize audio processor
    state.audio_processor = std::make_unique<Audio::AudioProcessor>();
    
    Audio::AudioBufferDesc audio_desc;
    audio_desc.sample_rate = config.audio_sample_rate;
    audio_desc.channels = config.audio_channels;
    audio_desc.format = Audio::AudioFormat::PCM_F32;
    audio_desc.frames = 1024; // Standard buffer size
    
    if (!state.audio_processor->initialize(audio_desc)) {
        std::cerr << "Failed to initialize audio processor" << std::endl;
        return false;
    }
    
    // Initialize timeline world
    state.timeline_world = std::make_unique<Timeline::World>();
    
    std::cout << "✓ Effect manager initialized" << std::endl;
    std::cout << "✓ Audio processor initialized" << std::endl;
    std::cout << "✓ Timeline world initialized" << std::endl;
    
    return true;
}

// Setup demo effects chain
void setup_video_effects(DemoState& state) {
    std::cout << "\n--- Setting up Video Effects ---" << std::endl;
    
    // Create demo effect chain
    MockVulkanContext mock_context;
    
    if (!state.video_effects_chain.initialize(nullptr)) {
        std::cout << "⚠ GPU effects disabled (Vulkan not available)" << std::endl;
    }
    
    std::cout << "✓ Video effects chain created" << std::endl;
    
    // List available effects
    auto effect_ids = state.effect_manager->get_effect_ids();
    std::cout << "Available effects:" << std::endl;
    for (const auto& id : effect_ids) {
        std::cout << "  - " << id << std::endl;
    }
}

// Setup audio channels and effects
void setup_audio_effects(DemoState& state) {
    std::cout << "\n--- Setting up Audio Effects ---" << std::endl;
    
    auto& mixer = state.audio_processor->get_mixer();
    
    // Create audio channels
    auto channel1 = mixer.create_channel("Music", 2);
    auto channel2 = mixer.create_channel("Voice", 2);
    auto channel3 = mixer.create_channel("Effects", 2);
    
    if (!channel1 || !channel2 || !channel3) {
        std::cerr << "Failed to create audio channels" << std::endl;
        return;
    }
    
    state.audio_channels = {channel1, channel2, channel3};
    
    // Add volume effects to channels
    auto volume1 = Audio::AudioEffectFactory::create_volume_effect();
    auto volume2 = Audio::AudioEffectFactory::create_volume_effect();
    auto volume3 = Audio::AudioEffectFactory::create_volume_effect();
    
    if (volume1 && volume2 && volume3) {
        volume1->set_parameter("gain", 0.8f);  // Music at 80%
        volume2->set_parameter("gain", 1.0f);  // Voice at 100%
        volume3->set_parameter("gain", 0.6f);  // Effects at 60%
        
        channel1->add_effect(std::move(volume1));
        channel2->add_effect(std::move(volume2));
        channel3->add_effect(std::move(volume3));
    }
    
    // Configure channel settings
    channel1->set_volume(0.8f);
    channel1->set_pan(-0.3f);  // Slightly left
    
    channel2->set_volume(1.0f);
    channel2->set_pan(0.0f);   // Center
    
    channel3->set_volume(0.6f);
    channel3->set_pan(0.2f);   // Slightly right
    
    std::cout << "✓ Audio channels created with effects" << std::endl;
    std::cout << "✓ Channel mix configured" << std::endl;
    
    // List available audio effects
    auto audio_effects = Audio::AudioEffectFactory::get_available_effects();
    std::cout << "Available audio effects:" << std::endl;
    for (const auto& effect : audio_effects) {
        auto info = Audio::AudioEffectFactory::get_effect_info(effect);
        std::cout << "  - " << info.name << ": " << info.description << std::endl;
    }
}

// Setup timeline entities
void setup_timeline_demo(DemoState& state) {
    std::cout << "\n--- Setting up Timeline Demo ---" << std::endl;
    
    // Create demo entities
    state.demo_entity = state.timeline_world->create_entity();
    
    // Add components to demonstrate timeline system
    auto clip_component = state.timeline_world->add_component<Timeline::ClipComponent>(
        state.demo_entity);
    
    if (clip_component) {
        clip_component->start_time = 0.0f;
        clip_component->duration = 5.0f;
        clip_component->track_id = 0;
        std::cout << "✓ Timeline entity created with clip component" << std::endl;
    }
    
    // Add effect component
    auto effect_component = state.timeline_world->add_component<Timeline::EffectComponent>(
        state.demo_entity);
    
    if (effect_component) {
        effect_component->effect_id = "color_correction";
        effect_component->enabled = true;
        std::cout << "✓ Effect component added to timeline entity" << std::endl;
    }
    
    // Add transform component
    auto transform_component = state.timeline_world->add_component<Timeline::TransformComponent>(
        state.demo_entity);
    
    if (transform_component) {
        transform_component->position = {0.0f, 0.0f};
        transform_component->scale = {1.0f, 1.0f};
        transform_component->rotation = 0.0f;
        std::cout << "✓ Transform component added to timeline entity" << std::endl;
    }
}

// Process single frame
void process_frame(const DemoConfig& config, DemoState& state, float delta_time) {
    state.current_time += delta_time;
    state.frame_count++;
    
    // Process audio
    if (config.enable_audio_processing && state.audio_processor) {
        // Generate demo audio buffers
        std::vector<Audio::AudioBuffer> audio_buffers;
        
        for (size_t i = 0; i < state.audio_channels.size(); ++i) {
            Audio::AudioBuffer buffer;
            Audio::AudioBufferDesc desc;
            desc.sample_rate = config.audio_sample_rate;
            desc.channels = config.audio_channels;
            desc.format = Audio::AudioFormat::PCM_F32;
            desc.frames = 512;
            
            if (buffer.allocate(desc)) {
                // Generate different tones for each channel
                float frequency = 440.0f * (i + 1); // A4, A5, A6
                generate_demo_audio_data(buffer, frequency, delta_time);
                audio_buffers.push_back(std::move(buffer));
            }
        }
        
        // Process audio through mixer
        Audio::AudioBuffer output_buffer;
        if (state.audio_processor->process_buffers(audio_buffers, output_buffer, state.current_time)) {
            // Get audio levels for monitoring
            auto levels = state.audio_processor->get_current_levels();
            
            if (state.frame_count % 60 == 0) { // Print every second
                std::cout << "\rAudio Levels - L: " << levels.left_peak 
                          << " R: " << levels.right_peak 
                          << " RMS L: " << levels.left_rms 
                          << " R: " << levels.right_rms << std::flush;
            }
        }
    }
    
    // Process timeline systems
    if (state.timeline_world) {
        // Update timeline systems (in a real implementation, this would process
        // all entities and their components)
        state.timeline_world->update(delta_time);
    }
    
    // Process video effects (mock implementation)
    if (config.enable_gpu_effects && state.frame_count % 60 == 0) {
        std::cout << "\rFrame: " << state.frame_count 
                  << " Time: " << state.current_time 
                  << "s Processing video effects..." << std::flush;
    }
}

// Performance monitoring
void update_performance_metrics(DemoState& state, float delta_time) {
    static float fps_accumulator = 0.0f;
    static uint32_t fps_count = 0;
    
    if (delta_time > 0.0f) {
        float fps = 1.0f / delta_time;
        fps_accumulator += fps;
        fps_count++;
        
        if (fps_count >= 60) { // Average over 60 frames
            state.avg_fps = fps_accumulator / fps_count;
            fps_accumulator = 0.0f;
            fps_count = 0;
        }
    }
    
    state.total_frames_processed++;
}

// Print system information
void print_system_info() {
    std::cout << "\n=== System Information ===" << std::endl;
    
    // Audio system info
    auto audio_effects = Audio::AudioEffectFactory::get_effect_info();
    std::cout << "Audio Effects System:" << std::endl;
    std::cout << "  Available effects: " << audio_effects.size() << std::endl;
    std::cout << "  Supported formats: PCM_S16, PCM_F32" << std::endl;
    std::cout << "  Sample rates: 22kHz, 44kHz, 48kHz, 96kHz" << std::endl;
    
    // Effects system info
    std::cout << "\nEffects System:" << std::endl;
    std::cout << "  GPU acceleration: " << (std::getenv("VULKAN_SDK") ? "Available" : "Not detected") << std::endl;
    std::cout << "  Effect chaining: Supported" << std::endl;
    std::cout << "  Real-time processing: Yes" << std::endl;
    
    // Timeline system info
    std::cout << "\nTimeline System:" << std::endl;
    std::cout << "  ECS architecture: Active" << std::endl;
    std::cout << "  Component types: 8+" << std::endl;
    std::cout << "  System types: 6" << std::endl;
}

// Run the main demo loop
void run_demo(const DemoConfig& config) {
    DemoState state;
    
    if (!initialize_demo(config, state)) {
        std::cerr << "Failed to initialize demo" << std::endl;
        return;
    }
    
    setup_video_effects(state);
    setup_audio_effects(state);
    setup_timeline_demo(state);
    print_system_info();
    
    std::cout << "\n=== Starting Demo Loop ===" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    auto start_time = high_resolution_clock::now();
    auto last_frame_time = start_time;
    
    // Demo loop
    while (state.running && state.current_time < config.demo_duration) {
        auto current_time = high_resolution_clock::now();
        auto delta_time = duration<float>(current_time - last_frame_time).count();
        
        process_frame(config, state, delta_time);
        update_performance_metrics(state, delta_time);
        
        last_frame_time = current_time;
        
        // Simple frame rate limiting (60 FPS)
        if (delta_time < 1.0f / 60.0f) {
            std::this_thread::sleep_for(
                milliseconds(static_cast<int>((1.0f / 60.0f - delta_time) * 1000)));
        }
        
        // Check for exit condition
        if (state.current_time >= config.demo_duration) {
            state.running = false;
        }
    }
    
    // Print final statistics
    auto total_time = duration<float>(high_resolution_clock::now() - start_time).count();
    
    std::cout << "\n\n=== Demo Complete ===" << std::endl;
    std::cout << "Total time: " << total_time << "s" << std::endl;
    std::cout << "Frames processed: " << state.total_frames_processed << std::endl;
    std::cout << "Average FPS: " << state.avg_fps << std::endl;
    std::cout << "Target duration: " << config.demo_duration << "s" << std::endl;
    std::cout << "Actual duration: " << state.current_time << "s" << std::endl;
}

int main(int argc, char* argv[]) {
    DemoConfig config;
    
    // Parse command line arguments (simplified)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--no-gpu") {
            config.enable_gpu_effects = false;
        } else if (arg == "--no-audio") {
            config.enable_audio_processing = false;
        } else if (arg == "--duration" && i + 1 < argc) {
            config.demo_duration = std::stof(argv[++i]);
        }
    }
    
    std::cout << "Phase 5 Effects & Audio Processing Demo" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "GPU Effects: " << (config.enable_gpu_effects ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Audio Processing: " << (config.enable_audio_processing ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Demo Duration: " << config.demo_duration << "s" << std::endl;
    
    try {
        run_demo(config);
    } catch (const std::exception& e) {
        std::cerr << "Demo error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
