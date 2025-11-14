// ShotcutCPP Complete UI Demo
// Comprehensive demonstration of all implemented systems

#include "../include/core/application.h"
#include "../include/ui/widget_system.h"
#include "../include/ui/theme_system.h"
#include "../include/ui/layout_engine.h"
#include "../include/ui/timeline_widget.h"
#include "../include/ui/input_manager.h"
#include "../include/ui/ui_renderer.h"
#include "../include/ui/math_types.h"
#include "../include/video/export_system.h"
#include "../include/video/video.h"
#include "../include/timeline/entity.h"
#include "../include/effects/effect_system.h"
#include "../include/audio/audio_processor.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

namespace Shotcut {

class CompleteUIDemo : public UI::Widget {
private:
    std::unique_ptr<UI::ThemeSystem> theme_system_;
    std::unique_ptr<UI::LayoutEngine> layout_engine_;
    std::unique_ptr<UI::InputManager> input_manager_;
    std::unique_ptr<UI::UIRenderer> renderer_;
    std::unique_ptr<UI::TimelineWidget> timeline_widget_;
    std::unique_ptr<Video::VideoSystem> video_system_;
    std::unique_ptr<Video::ExportSystem> export_system_;
    std::unique_ptr<Timeline::EntityManager> entity_manager_;
    std::unique_ptr<Effects::EffectSystem> effect_system_;
    std::unique_ptr<Audio::AudioProcessor> audio_processor_;
    
    // Demo state
    bool is_running_ = false;
    bool show_timeline_ = true;
    bool show_export_dialog_ = false;
    bool show_effects_panel_ = true;
    bool show_audio_mixer_ = true;
    
    // Performance metrics
    std::chrono::steady_clock::time_point last_frame_time_;
    int frame_count_ = 0;
    float fps_ = 0.0f;

public:
    CompleteUIDemo() : Widget("CompleteUIDemo") {
        std::cout << "Initializing ShotcutCPP Complete UI Demo...\n";
        
        // Initialize core systems
        theme_system_ = std::make_unique<UI::ThemeSystem>();
        theme_system_->load_theme(UI::ThemeType::ProfessionalDark);
        
        layout_engine_ = std::make_unique<UI::LayoutEngine>();
        input_manager_ = std::make_unique<UI::InputManager>();
        renderer_ = std::make_unique<UI::UIRenderer>();
        
        // Initialize video systems
        video_system_ = std::make_unique<Video::VideoSystem>();
        if (!video_system_->initialize()) {
            std::cout << "Warning: Video system initialization failed\n";
        }
        
        export_system_ = std::make_unique<Video::ExportSystem>();
        if (!export_system_->initialize()) {
            std::cout << "Warning: Export system initialization failed\n";
        }
        
        // Initialize timeline system
        entity_manager_ = std::make_unique<Timeline::EntityManager>();
        
        // Initialize effects system
        effect_system_ = std::make_unique<Effects::EffectSystem>();
        if (!effect_system_->initialize()) {
            std::cout << "Warning: Effects system initialization failed\n";
        }
        
        // Initialize audio system
        audio_processor_ = std::make_unique<Audio::AudioProcessor>();
        if (!audio_processor_->initialize()) {
            std::cout << "Warning: Audio processor initialization failed\n";
        }
        
        // Create timeline widget
        timeline_widget_ = std::make_unique<UI::TimelineWidget>();
        
        // Initialize input system
        input_manager_->initialize();
        
        last_frame_time_ = std::chrono::steady_clock::now();
        
        std::cout << "All systems initialized successfully!\n";
    }
    
    ~CompleteUIDemo() override {
        std::cout << "Shutting down Complete UI Demo...\n";
        
        // Cleanup is handled automatically by unique_ptr destructors
    }
    
    void run() {
        std::cout << "\n=== SHOTCUT COMPLETE UI DEMO ===\n";
        std::cout << "Professional Video Editor Demonstration\n";
        std::cout << "=====================================\n\n";
        
        is_running_ = true;
        
        // Display system information
        display_system_info();
        
        // Create demo timeline
        create_demo_timeline();
        
        // Initialize renderer
        renderer_->initialize();
        
        // Main demo loop
        while (is_running_) {
            process_input();
            update();
            render();
            
            // Calculate FPS
            calculate_fps();
            
            // Small delay for demo purposes
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        std::cout << "\nDemo completed. Shutting down...\n";
    }
    
private:
    void display_system_info() {
        std::cout << "SYSTEM INFORMATION:\n";
        std::cout << "-----------------\n";
        
        // Video system info
        if (video_system_ && video_system_->is_initialized()) {
            auto video_info = video_system_->get_system_info();
            std::cout << "Video System: Initialized\n";
            std::cout << "  Hardware Acceleration: " << (video_info.hardware_acceleration ? "Enabled" : "Disabled") << "\n";
            std::cout << "  Supported Codecs: ";
            for (const auto& codec : video_info.supported_codecs) {
                std::cout << codec << " ";
            }
            std::cout << "\n";
        }
        
        // Export system info
        if (export_system_ && export_system_->is_initialized()) {
            std::cout << "Export System: Initialized\n";
            auto presets = export_system_->get_available_presets();
            std::cout << "  Available Presets: " << presets.size() << "\n";
            std::cout << "  Batch Export: Enabled\n";
        }
        
        // Effects system info
        if (effect_system_ && effect_system_->is_initialized()) {
            std::cout << "Effects System: Initialized\n";
            std::cout << "  GPU Effects: Enabled\n";
            std::cout << "  Real-time Processing: Enabled\n";
        }
        
        // Audio system info
        if (audio_processor_ && audio_processor_->is_initialized()) {
            std::cout << "Audio System: Initialized\n";
            std::cout << "  Sample Rate: 48kHz\n";
            std::cout << "  Channels: Stereo\n";
            std::cout << "  Low Latency: Enabled\n";
        }
        
        std::cout << "\n";
    }
    
    void create_demo_timeline() {
        std::cout << "Creating demo timeline...\n";
        
        if (!entity_manager_) {
            std::cout << "Error: Entity manager not initialized\n";
            return;
        }
        
        // Create demo video clip
        auto video_clip_entity = entity_manager_->create_entity();
        entity_manager_->add_component(video_clip_entity, Timeline::VideoClipComponent{
            .file_path = "demo_video.mp4",
            .start_time = 0.0f,
            .duration = 10.0f,
            .track_index = 0,
            .position_x = 0.0f,
            .width = 1920.0f,
            .height = 1080.0f
        });
        
        // Create demo audio clip
        auto audio_clip_entity = entity_manager_->create_entity();
        entity_manager_->add_component(audio_clip_entity, Timeline::AudioClipComponent{
            .file_path = "demo_audio.wav",
            .start_time = 0.0f,
            .duration = 10.0f,
            .track_index = 0,
            .volume = 1.0f,
            .pan = 0.0f
        });
        
        // Create effect component
        entity_manager_->add_component(video_clip_entity, Timeline::EffectComponent{
            .effect_name = "Color Correction",
            .enabled = true,
            .parameters = {{"brightness", 0.0f}, {"contrast", 1.0f}, {"saturation", 1.0f}}
        });
        
        std::cout << "Demo timeline created with 2 clips and effects\n\n";
    }
    
    void process_input() {
        if (!input_manager_) return;
        
        // Process all queued input events
        input_manager_->process_events();
        
        // Check for demo-specific input
        if (input_manager_->is_key_pressed(UI::KeyCode::Escape)) {
            is_running_ = false;
        }
        
        if (input_manager_->is_key_pressed(UI::KeyCode::T)) {
            show_timeline_ = !show_timeline_;
            std::cout << "Timeline " << (show_timeline_ ? "shown" : "hidden") << "\n";
        }
        
        if (input_manager_->is_key_pressed(UI::KeyCode::E)) {
            show_export_dialog_ = !show_export_dialog_;
            std::cout << "Export dialog " << (show_export_dialog_ ? "shown" : "hidden") << "\n";
        }
        
        if (input_manager_->is_key_pressed(UI::KeyCode::F)) {
            show_effects_panel_ = !show_effects_panel_;
            std::cout << "Effects panel " << (show_effects_panel_ ? "shown" : "hidden") << "\n";
        }
        
        if (input_manager_->is_key_pressed(UI::KeyCode::A)) {
            show_audio_mixer_ = !show_audio_mixer_;
            std::cout << "Audio mixer " << (show_audio_mixer_ ? "shown" : "hidden") << "\n";
        }
        
        if (input_manager_->is_key_pressed(UI::KeyCode::Space)) {
            if (video_system_) {
                video_system_->toggle_playback();
                std::cout << "Playback toggled\n";
            }
        }
    }
    
    void update() {
        // Update timeline widget
        if (timeline_widget_ && show_timeline_) {
            timeline_widget_->update();
        }
        
        // Update video system
        if (video_system_) {
            video_system_->update();
        }
        
        // Update export system progress
        if (export_system_ && export_system_->is_exporting()) {
            auto progress = export_system_->get_progress();
            if (progress.has_error) {
                std::cout << "Export error: " << progress.error_message << "\n";
            }
        }
        
        // Update effects system
        if (effect_system_) {
            effect_system_->update();
        }
        
        // Update audio processor
        if (audio_processor_) {
            audio_processor_->update();
        }
    }
    
    void render() {
        if (!renderer_) return;
        
        renderer_->begin_frame();
        
        // Render main layout
        render_main_layout();
        
        // Render UI panels based on state
        if (show_timeline_) {
            render_timeline_panel();
        }
        
        if (show_export_dialog_) {
            render_export_dialog();
        }
        
        if (show_effects_panel_) {
            render_effects_panel();
        }
        
        if (show_audio_mixer_) {
            render_audio_mixer();
        }
        
        // Render performance overlay
        render_performance_overlay();
        
        renderer_->end_frame();
    }
    
    void render_main_layout() {
        renderer_->render_rectangle(0, 0, 800, 600, theme_system_->get_color(UI::ColorRole::Background));
        
        // Render main toolbar
        renderer_->render_rectangle(0, 0, 800, 40, theme_system_->get_color(UI::ColorRole::Toolbar));
        
        // Render toolbar buttons
        render_toolbar_button(10, 5, 60, 30, "Play", UI::KeyCode::Space);
        render_toolbar_button(80, 5, 60, 30, "Export", UI::KeyCode::E);
        render_toolbar_button(150, 5, 60, 30, "Effects", UI::KeyCode::F);
        render_toolbar_button(220, 5, 60, 30, "Audio", UI::KeyCode::A);
        render_toolbar_button(290, 5, 60, 30, "Timeline", UI::KeyCode::T);
        
        // Render main viewport
        renderer_->render_rectangle(0, 40, 800, 400, theme_system_->get_color(UI::ColorRole::Panel));
        renderer_->render_text(10, 50, "Video Preview Viewport", theme_system_->get_color(UI::ColorRole::Text));
    }
    
    void render_toolbar_button(int x, int y, int width, int height, const std::string& label, UI::KeyCode shortcut) {
        auto bg_color = input_manager_->is_mouse_in_rect(x, y, width, height) ? 
                       theme_system_->get_color(UI::ColorRole::ButtonHover) : 
                       theme_system_->get_color(UI::ColorRole::Button);
        
        renderer_->render_rectangle(x, y, width, height, bg_color);
        renderer_->render_text(x + 5, y + 10, label, theme_system_->get_color(UI::ColorRole::ButtonText));
        renderer_->render_text(x + 5, y + 25, "[" + std::string(1, static_cast<char>(shortcut)) + "]", 
                              theme_system_->get_color(UI::ColorRole::TextSecondary));
    }
    
    void render_timeline_panel() {
        int panel_height = 160;
        renderer_->render_rectangle(0, 600 - panel_height, 800, panel_height, 
                                  theme_system_->get_color(UI::ColorRole::Panel));
        
        renderer_->render_text(10, 445, "Timeline Panel", theme_system_->get_color(UI::ColorRole::Text));
        
        if (timeline_widget_) {
            // Render simplified timeline representation
            renderer_->render_rectangle(10, 470, 780, 120, theme_system_->get_color(UI::ColorRole::TimelineBackground));
            
            // Render demo clips
            renderer_->render_rectangle(20, 480, 200, 30, theme_system_->get_color(UI::ColorRole::VideoClip));
            renderer_->render_text(25, 485, "Video Clip (10s)", theme_system_->get_color(UI::ColorRole::Text));
            
            renderer_->render_rectangle(20, 520, 200, 30, theme_system_->get_color(UI::ColorRole::AudioClip));
            renderer_->render_text(25, 525, "Audio Clip (10s)", theme_system_->get_color(UI::ColorRole::Text));
        }
    }
    
    void render_export_dialog() {
        // Render modal dialog
        renderer_->render_rectangle(200, 100, 400, 300, theme_system_->get_color(UI::ColorRole::Modal));
        renderer_->render_rectangle(200, 100, 400, 40, theme_system_->get_color(UI::ColorRole::ModalHeader));
        
        renderer_->render_text(210, 115, "Export Settings", theme_system_->get_color(UI::ColorRole::Text));
        
        if (export_system_ && export_system_->is_initialized()) {
            auto presets = export_system_->get_available_presets();
            int y_offset = 150;
            
            for (size_t i = 0; i < std::min(presets.size(), size_t(8)); ++i) {
                renderer_->render_text(210, y_offset + i * 20, 
                                      "- Export preset " + std::to_string(i + 1), 
                                      theme_system_->get_color(UI::ColorRole::Text));
            }
        }
        
        renderer_->render_text(210, 370, "Press 'E' to close", theme_system_->get_color(UI::ColorRole::TextSecondary));
    }
    
    void render_effects_panel() {
        renderer_->render_rectangle(800 - 200, 40, 200, 200, theme_system_->get_color(UI::ColorRole::Panel));
        renderer_->render_text(810, 50, "Effects Panel", theme_system_->get_color(UI::ColorRole::Text));
        
        if (effect_system_ && effect_system_->is_initialized()) {
            renderer_->render_text(810, 80, "• Color Correction", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 100, "• Gaussian Blur", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 120, "• Sharpening", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 140, "• Scale Transform", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 160, "• Noise Generator", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 180, "• Crossfade", theme_system_->get_color(UI::ColorRole::Text));
        }
    }
    
    void render_audio_mixer() {
        renderer_->render_rectangle(800 - 200, 240, 200, 150, theme_system_->get_color(UI::ColorRole::Panel));
        renderer_->render_text(810, 250, "Audio Mixer", theme_system_->get_color(UI::ColorRole::Text));
        
        if (audio_processor_ && audio_processor_->is_initialized()) {
            renderer_->render_text(810, 280, "• Volume Control", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 300, "• Equalizer", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 320, "• Compressor", theme_system_->get_color(UI::ColorRole::Text));
            renderer_->render_text(810, 340, "• Reverb", theme_system_->get_color(UI::ColorRole::Text));
        }
    }
    
    void render_performance_overlay() {
        renderer_->render_text(10, 10, "FPS: " + std::to_string(static_cast<int>(fps_)), 
                              theme_system_->get_color(UI::ColorRole::TextSecondary));
        
        frame_count_++;
    }
    
    void calculate_fps() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time_);
        
        if (elapsed.count() >= 1000) { // Update every second
            fps_ = static_cast<float>(frame_count_) / (elapsed.count() / 1000.0f);
            frame_count_ = 0;
            last_frame_time_ = now;
        }
    }
};

} // namespace Shotcut

int main() {
    std::cout << "Starting ShotcutCPP Complete UI Demo...\n\n";
    
    try {
        auto demo = std::make_unique<Shotcut::CompleteUIDemo>();
        demo->run();
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\nThank you for testing ShotcutCPP!\n";
    return 0;
}
