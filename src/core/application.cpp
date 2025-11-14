#include "application.h"
#include "../rendering/vulkan_context.h"
#include "../rendering/vulkan_swapchain.h"
#include "../rendering/vulkan_graphics_pipeline.h"
#include "../rendering/vulkan_command_buffer.h"
#include "../ui/vulkan_ui_renderer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace shotcut {

class Application::Impl {
private:
    GLFWwindow* window_;
    std::unique_ptr<VulkanContext> vulkan_context_;
    std::unique_ptr<VulkanSwapchain> swapchain_;
    std::unique_ptr<VulkanGraphicsPipeline> graphics_pipeline_;
    std::unique_ptr<VulkanCommandBuffer> command_buffer_;
    std::unique_ptr<Shotcut::UI::VulkanUIRenderer> ui_renderer_;
    
    uint32_t window_width_;
    uint32_t window_height_;
    bool should_close_;
    
public:
    Impl() : window_(nullptr), window_width_(1920), window_height_(1080), should_close_(false) {}
    ~Impl() = default;
    
    bool initialize() {
        std::cout << "Initializing ShotcutCPP Application..." << std::endl;
        
#ifdef HAVE_GLFW
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window_ = glfwCreateWindow(window_width_, window_height_, "ShotcutCPP - Professional Video Editor", nullptr, nullptr);
        
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Set window user pointer for callbacks
        glfwSetWindowUserPointer(window_, this);
        
        // Set close callback
        glfwSetWindowCloseCallback(window_, [](GLFWwindow* w) {
            auto* impl = static_cast<Application::Impl*>(glfwGetWindowUserPointer(w));
            impl->should_close_ = true;
        });
#else
        std::cout << "GLFW not available, running in headless mode for demo" << std::endl;
        window_ = nullptr;
#endif
        
        // Initialize Vulkan
        vulkan_context_ = std::make_unique<VulkanContext>(true);
        if (!vulkan_context_->initialize(window_)) {
            std::cerr << "Failed to initialize Vulkan" << std::endl;
            return false;
        }
        
        // Initialize swapchain
        swapchain_ = std::make_unique<VulkanSwapchain>(vulkan_context_.get());
        if (!swapchain_->initialize(vulkan_context_->surface(), window_width_, window_height_)) {
            std::cerr << "Failed to initialize swapchain" << std::endl;
            return false;
        }
        
        // Initialize graphics pipeline
        graphics_pipeline_ = std::make_unique<VulkanGraphicsPipeline>(vulkan_context_.get(), swapchain_.get());
        if (!graphics_pipeline_->initialize()) {
            std::cerr << "Failed to initialize graphics pipeline" << std::endl;
            return false;
        }
        
        // Initialize command buffer
        command_buffer_ = std::make_unique<VulkanCommandBuffer>(vulkan_context_.get(), swapchain_.get(), graphics_pipeline_.get());
        if (!command_buffer_->initialize()) {
            std::cerr << "Failed to initialize command buffer" << std::endl;
            return false;
        }
        
        // Initialize UI renderer
        ui_renderer_ = std::make_unique<Shotcut::UI::VulkanUIRenderer>();
        if (!ui_renderer_->initialize(vulkan_context_.get(), swapchain_.get(), graphics_pipeline_.get(), command_buffer_.get())) {
            std::cerr << "Failed to initialize UI renderer" << std::endl;
            return false;
        }
        
        std::cout << "Application initialized successfully!" << std::endl;
        std::cout << "  - Vulkan Context: OK" << std::endl;
        std::cout << "  - Swapchain: OK (" << swapchain_->image_count() << " images)" << std::endl;
        std::cout << "  - Graphics Pipeline: OK" << std::endl;
        std::cout << "  - Command Buffers: OK" << std::endl;
        std::cout << "  - UI Renderer: OK" << std::endl;
        
        return true;
    }
    
    int run() {
        std::cout << "\n=== SHOTCUT CPP - PROFESSIONAL VIDEO EDITOR ===" << std::endl;
        std::cout << "✓ Vulkan Rendering Backend (GPU-Accelerated)" << std::endl;
        std::cout << "✓ Modern C++23 Architecture" << std::endl;
        std::cout << "✓ Professional UI Framework" << std::endl;
        std::cout << "✓ Cross-Platform Support" << std::endl;
        std::cout << "================================================\n" << std::endl;
        
#ifdef HAVE_GLFW
        // Main rendering loop
        auto start_time = std::chrono::high_resolution_clock::now();
        uint32_t frame_count = 0;
        
        while (!glfwWindowShouldClose(window_) && !should_close_) {
            glfwPollEvents();
            
            // Render frame
            uint32_t image_index;
            if (!ui_renderer_->begin_frame(image_index)) {
                std::cerr << "Failed to begin frame" << std::endl;
                break;
            }
            
            // Begin render pass
            command_buffer_->begin_render_pass(image_index);
            command_buffer_->bind_pipeline();
            
            // Draw UI
            draw_ui();
            
            // End render pass
            command_buffer_->end_render_pass();
            
            // End frame and present
            if (!ui_renderer_->end_frame()) {
                std::cerr << "Failed to end frame" << std::endl;
                break;
            }
            
            frame_count++;
            
            // Print performance info every 60 frames
            if (frame_count % 60 == 0) {
                auto current_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
                double fps = (frame_count * 1000.0) / elapsed;
                std::cout << "Frame " << frame_count << " | FPS: " << fps << std::endl;
            }
        }
        
        std::cout << "\nApplication closed gracefully" << std::endl;
#else
        // Headless demo mode
        std::cout << "Running in headless demo mode..." << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::cout << "Demo step " << (i + 1) << "/10" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "Demo completed!" << std::endl;
#endif
        
        return 0;
    }
    
    void shutdown() {
        std::cout << "Shutting down ShotcutCPP Application..." << std::endl;
        
        // Cleanup UI renderer
        if (ui_renderer_) {
            ui_renderer_->cleanup();
            ui_renderer_.reset();
        }
        
        // Cleanup command buffer
        if (command_buffer_) {
            command_buffer_->cleanup();
            command_buffer_.reset();
        }
        
        // Cleanup graphics pipeline
        if (graphics_pipeline_) {
            graphics_pipeline_->cleanup();
            graphics_pipeline_.reset();
        }
        
        // Cleanup swapchain
        if (swapchain_) {
            swapchain_->cleanup();
            swapchain_.reset();
        }
        
        // Cleanup Vulkan
        if (vulkan_context_) {
            vulkan_context_->cleanup();
            vulkan_context_.reset();
        }
        
#ifdef HAVE_GLFW
        // Cleanup GLFW
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        
        glfwTerminate();
#endif
    }
    
private:
    void draw_ui() {
        // Draw professional UI layout
        
        // Main background
        ui_renderer_->draw_rectangle({0, 0}, {(float)window_width_, (float)window_height_}, {0.15f, 0.15f, 0.15f, 1.0f});
        
        // Top toolbar
        ui_renderer_->draw_panel({0, 0}, {(float)window_width_, 60}, {0.2f, 0.2f, 0.2f, 1.0f});
        
        // Left sidebar
        ui_renderer_->draw_panel({0, 60}, {300, (float)(window_height_ - 60)}, {0.18f, 0.18f, 0.18f, 1.0f});
        
        // Center timeline area
        ui_renderer_->draw_panel({300, 60}, {(float)(window_width_ - 600), (float)(window_height_ - 200)}, {0.16f, 0.16f, 0.16f, 1.0f});
        
        // Right properties panel
        ui_renderer_->draw_panel({(float)(window_width_ - 300), 60}, {300, (float)(window_height_ - 60)}, {0.18f, 0.18f, 0.18f, 1.0f});
        
        // Bottom status bar
        ui_renderer_->draw_panel({300, (float)(window_height_ - 140)}, {(float)(window_width_ - 600), 140}, {0.2f, 0.2f, 0.2f, 1.0f});
        
        // Draw some buttons in toolbar
        ui_renderer_->draw_button({10, 10}, {80, 40}, "File", {0.3f, 0.3f, 0.3f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        ui_renderer_->draw_button({100, 10}, {80, 40}, "Edit", {0.3f, 0.3f, 0.3f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        ui_renderer_->draw_button({190, 10}, {80, 40}, "View", {0.3f, 0.3f, 0.3f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    }
};

Application::Application() 
    : pimpl_(std::make_unique<Impl>()) {
}

Application::~Application() = default;

bool Application::initialize() {
    return pimpl_->initialize();
}

int Application::run() {
    return pimpl_->run();
}

void Application::shutdown() {
    pimpl_->shutdown();
}

} // namespace shotcut
