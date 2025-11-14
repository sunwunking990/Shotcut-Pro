# Shotcut C++ Transformation - System Patterns

## Architectural Overview

### Core Design Philosophy

The ShotcutCPP system follows a **layered, component-based architecture** with these fundamental principles:

1. **GPU-First Design** - All major operations leverage GPU acceleration
2. **Unified Pipeline** - UI and video processing share GPU resources
3. **Memory Safety** - Modern C++ patterns prevent common vulnerabilities
4. **Component Modularity** - Clear separation of concerns with defined interfaces
5. **Performance by Design** - Sub-frame latency targets drive all architectural decisions

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                           │
├─────────────────────────────────────────────────────────────────┤
│  UI Framework  │  Timeline Engine  │  Effects System  │ Audio │
├─────────────────────────────────────────────────────────────────┤
│                    Core Services Layer                         │
├─────────────────────────────────────────────────────────────────┤
│ Video Pipeline │ Audio Pipeline │ Plugin System │ Project Mgmt │
├─────────────────────────────────────────────────────────────────┤
│                   Rendering Layer                             │
├─────────────────────────────────────────────────────────────────┤
│   Vulkan Context   │   NVIDIA SDK   │   DirectX 12 (Win)   │
├─────────────────────────────────────────────────────────────────┤
│                   Platform Layer                              │
├─────────────────────────────────────────────────────────────────┤
│   Windows API      │   Linux API     │   File System        │
└─────────────────────────────────────────────────────────────────┘
```

## Core Architectural Patterns

### 1. Entity Component System (ECS) for Timeline

#### Pattern Overview
The timeline uses an ECS architecture for optimal performance with complex projects containing thousands of clips.

```cpp
// ECS-based timeline architecture
class Timeline {
    entt::registry registry;  // Core ECS registry
    
    // Components - Data-only structures
    struct VideoClip {
        MediaFile file;
        TimeRange in_out;
        Transform2D transform;
        EffectsStack effects;
    };
    
    struct AudioClip {
        MediaFile file;
        TimeRange in_out;
        AudioParameters params;
        Volume volume;
    };
    
    struct Effect {
        GPUShader shader;
        ParameterMap params;
        TimeRange range;
        bool enabled;
    };
    
    struct Track {
        TrackType type;
        bool muted;
        bool locked;
        float height;
    };
    
    // Systems - Logic processors
    void process_video_clips(float delta_time);
    void process_audio_clips(float delta_time);
    void apply_effects(float delta_time);
    void render_timeline_ui();
    
public:
    // Public interface
    entity_t add_video_clip(const VideoClip& clip);
    entity_t add_audio_clip(const AudioClip& clip);
    void remove_clip(entity_t entity);
    void update_frame(TimeCode current_time);
};
```

#### Performance Benefits
- **Cache Locality:** Related data stored contiguously in memory
- **Parallel Processing:** Natural multi-threading for independent entities
- **Scalability:** Handles 1000+ clips without performance degradation
- **Flexibility:** Easy addition of new component types

#### Implementation Patterns
```cpp
// System processing pattern
void Timeline::process_video_clips(float delta_time) {
    // Get all entities with VideoClip and Transform components
    auto view = registry.view<VideoClip, Transform>();
    
    // Process in parallel using std::execution
    std::for_each(std::execution::par_unseq, view.begin(), view.end(),
        [&](auto entity) {
            auto& clip = view.get<VideoClip>(entity);
            auto& transform = view.get<Transform>(entity);
            
            if (is_clip_active(clip, current_time_)) {
                process_clip_gpu(clip, transform);
            }
        });
}

// Memory-efficient component iteration
void Timeline::render_timeline_ui() {
    // Batch render clips by track for optimal GPU usage
    auto tracks = registry.view<Track>();
    for (auto track_entity : tracks) {
        auto& track = tracks.get<Track>(track_entity);
        render_track_clips(track_entity, track);
    }
}
```

### 2. Unified GPU Rendering Pipeline

#### Pattern Overview
All rendering (UI, video, effects) goes through a single Vulkan pipeline for optimal performance.

```cpp
class UnifiedRenderingPipeline {
    VulkanContext vulkan_context_;
    
    // Rendering layers (bottom to top)
    struct RenderLayer {
        enum Type {
            VideoContent = 0,    // Bottom layer
            EffectsOverlay = 1,   // Middle layer
            UIInterface = 2       // Top layer
        };
    };
    
    // Command buffer management
    std::vector<VkCommandBuffer> frame_command_buffers_;
    std::array<RenderPass, 3> render_passes_;
    
    // GPU resource management
    GPUBufferPool buffer_pool_;
    GPUTexturePool texture_pool_;
    DescriptorSetManager descriptor_manager_;
    
public:
    void render_frame();
    void begin_video_layer();
    void begin_effects_layer();
    void begin_ui_layer();
    void end_frame();
};
```

#### Layer Compositing Pattern
```cpp
void UnifiedRenderingPipeline::render_frame() {
    auto cmd_buffer = begin_frame();
    
    // 1. Video content layer (bottom)
    begin_video_layer();
    video_processor_->render_to_surface(cmd_buffer);
    
    // 2. Effects overlay layer
    begin_effects_layer();
    effects_processor_->render_effects(cmd_buffer);
    
    // 3. UI interface layer (top)
    begin_ui_layer();
    ui_renderer_->render_interface(cmd_buffer);
    
    // 4. Single present call
    end_frame(cmd_buffer);
}
```

#### Memory Management Pattern
```cpp
class GPUMemoryManager {
    // Pooled allocation for performance
    struct MemoryPool {
        std::vector<VkBuffer> buffers_;
        std::vector<VkDeviceMemory> memories_;
        std::queue<size_t> free_indices_;
        size_t buffer_size_;
        VkBufferUsageFlags usage_;
    };
    
    std::map<VkBufferUsageFlags, MemoryPool> pools_;
    
public:
    template<typename T>
    GPUBuffer allocate_buffer(const std::vector<T>& data, VkBufferUsageFlags usage) {
        auto& pool = pools_[usage];
        size_t index = pool.allocate(data.size() * sizeof(T));
        
        // Upload data to GPU
        upload_to_gpu(pool.buffers_[index], data.data(), data.size() * sizeof(T));
        
        return GPUBuffer{pool.buffers_[index], index, data.size() * sizeof(T)};
    }
    
    void deallocate_buffer(const GPUBuffer& buffer);
};
```

### 3. Hardware Acceleration Abstraction

#### Pattern Overview
Hardware acceleration is abstracted through a common interface with platform-specific implementations.

```cpp
// Hardware codec interface
class HardwareCodec {
public:
    virtual ~HardwareCodec() = default;
    
    virtual Result initialize(const CodecConfig& config) = 0;
    virtual Result decode_frame(const EncodedData& input, GPUFrame& output) = 0;
    virtual Result encode_frame(const GPUFrame& input, EncodedData& output) = 0;
    virtual Result get_capabilities(CodecCapabilities& caps) = 0;
};

// NVIDIA implementation
class NVIDIACodec : public HardwareCodec {
    NVDECContext decoder_context_;
    NVENCContext encoder_context_;
    CUDAInterop cuda_interop_;
    
public:
    Result initialize(const CodecConfig& config) override {
        if (config.type == CodecType::H264) {
            return initialize_h264_decoder(config);
        } else if (config.type == CodecType::H265) {
            return initialize_h265_decoder(config);
        }
        return Result::UnsupportedCodec;
    }
    
    Result decode_frame(const EncodedData& input, GPUFrame& output) override {
        // Use NVDEC for hardware decoding
        CUVIDPICPARAMS pic_params;
        parse_h264_bitstream(input.data, input.size, pic_params);
        
        // Decode to GPU memory directly
        CUVIDDECODER decoder = decoder_context_.get_decoder();
        cuvidDecodePicture(decoder, &pic_params);
        
        // Map to CUDA surface
        CUDA_SURFACE surface;
        cuda_interop_.map_decoder_output(surface);
        
        // Convert to our GPU frame format
        output = cuda_interop_.convert_to_gpu_frame(surface);
        
        return Result::Success;
    }
};

// Software fallback implementation
class SoftwareCodec : public HardwareCodec {
    FFmpegContext ffmpeg_context_;
    
public:
    Result decode_frame(const EncodedData& input, GPUFrame& output) override {
        // Use FFmpeg for software decoding
        AVFrame* av_frame = nullptr;
        int ret = avcodec_send_packet(ffmpeg_context_.codec_ctx, &input.av_packet);
        ret = avcodec_receive_frame(ffmpeg_context_.codec_ctx, &av_frame);
        
        if (ret == 0) {
            // Upload to GPU
            upload_cpu_frame_to_gpu(av_frame, output);
            av_frame_free(&av_frame);
            return Result::Success;
        }
        
        return Result::DecodeError;
    }
};
```

#### Factory Pattern for Codec Selection
```cpp
class CodecFactory {
public:
    static std::unique_ptr<HardwareCodec> create_codec(const CodecConfig& config) {
        // Try NVIDIA hardware first
        if (has_nvidia_gpu() && is_nvidia_codec_supported(config.type)) {
            auto codec = std::make_unique<NVIDIACodec>();
            if (codec->initialize(config) == Result::Success) {
                return codec;
            }
        }
        
        // Try Intel Quick Sync
        if (has_intel_gpu() && is_intel_codec_supported(config.type)) {
            auto codec = std::make_unique<IntelCodec>();
            if (codec->initialize(config) == Result::Success) {
                return codec;
            }
        }
        
        // Fallback to software
        return std::make_unique<SoftwareCodec>();
    }
};
```

### 4. Async Operation Patterns with Coroutines

#### Pattern Overview
Modern C++20 coroutines handle asynchronous operations cleanly without callback hell.

```cpp
// Async media loading using coroutines
class AsyncMediaLoader {
public:
    // Coroutine-based media loading
    task<std::shared_ptr<MediaFile>> load_media_file_async(const std::string& path) {
        // 1. Start file loading on thread pool
        auto file_data = co_await load_file_on_thread_pool(path);
        if (!file_data.success) {
            co_return nullptr;
        }
        
        // 2. Detect format on main thread
        auto format = co_await detect_format_on_main_thread(file_data.data);
        if (!format.is_supported()) {
            co_return nullptr;
        }
        
        // 3. Initialize decoder on GPU thread
        auto decoder = co_await initialize_decoder_on_gpu_thread(format);
        if (!decoder.success) {
            co_return nullptr;
        }
        
        // 4. Create media file object
        auto media_file = std::make_shared<MediaFile>();
        media_file->path = path;
        media_file->format = format;
        media_file->decoder = std::move(decoder.decoder);
        media_file->duration = decoder.duration;
        
        co_return media_file;
    }
    
private:
    thread_pool executor_;
    gpu_thread_pool gpu_executor_;
    main_thread_scheduler main_scheduler_;
};

// Usage in UI
void TimelineWidget::on_media_dropped(const std::string& path) {
    // Start async loading
    auto load_task = media_loader_.load_media_file_async(path);
    
    // Handle completion with lambda
    load_task.then([this, path](auto media_file) {
        if (media_file) {
            add_clip_to_timeline(media_file);
        } else {
            show_error_dialog("Failed to load: " + path);
        }
    });
}
```

### 5. Security-First File Processing

#### Pattern Overview
All file processing happens in sandboxed environments with validation.

```cpp
class SecureFileProcessor {
public:
    Result process_media_file(const std::string& file_path) {
        // 1. Validate file path and permissions
        if (!validate_file_path(file_path)) {
            return Result::InvalidPath;
        }
        
        // 2. Process in sandboxed process
        Sandbox sandbox;
        sandbox.set_memory_limit(1024 * 1024 * 1024); // 1GB limit
        sandbox.set_time_limit(30); // 30 seconds
        sandbox.set_network_access(false);
        
        auto result = sandbox.execute([&file_path]() {
            return parse_media_file_safely(file_path);
        });
        
        if (result.status != SandboxStatus::Success) {
            log_security_event("Sandbox violation", file_path);
            return Result::SecurityViolation;
        }
        
        // 3. Validate parsed content
        if (!validate_parsed_content(result.data)) {
            return Result::InvalidContent;
        }
        
        // 4. Process with memory-safe operations
        return process_validated_content(result.data);
    }
    
private:
    bool validate_file_path(const std::string& path) {
        // Check for directory traversal
        if (path.find("..") != std::string::npos) return false;
        
        // Check for null bytes
        if (path.find('\0') != std::string::npos) return false;
        
        // Validate against allowed directories
        return is_path_in_allowed_directories(path);
    }
    
    bool validate_parsed_content(const ParsedMedia& content) {
        // Validate frame dimensions
        if (content.width > 8192 || content.height > 8192) return false;
        
        // Validate frame rate
        if (content.frame_rate > 240.0f) return false;
        
        // Validate bit depth
        if (content.bit_depth > 16) return false;
        
        // Validate duration
        if (content.duration > 24 * 3600) return false; // 24 hours max
        
        return true;
    }
};
```

## Component Interaction Patterns

### 1. Observer Pattern for Timeline Events

```cpp
// Timeline event system
class TimelineEventDispatcher {
public:
    enum class EventType {
        ClipAdded,
        ClipRemoved,
        ClipModified,
        TrackAdded,
        TrackRemoved,
        SelectionChanged
    };
    
    using EventCallback = std::function<void(const TimelineEvent&)>;
    
    void subscribe(EventType type, EventCallback callback) {
        subscribers_[type].push_back(callback);
    }
    
    void dispatch(const TimelineEvent& event) {
        auto& callbacks = subscribers_[event.type];
        for (auto& callback : callbacks) {
            callback(event);
        }
    }
    
private:
    std::map<EventType, std::vector<EventCallback>> subscribers_;
};

// Usage in UI components
class TimelineWidget {
    TimelineEventDispatcher* event_dispatcher_;
    
public:
    TimelineWidget(TimelineEventDispatcher* dispatcher) 
        : event_dispatcher_(dispatcher) {
        
        // Subscribe to timeline events
        event_dispatcher_->subscribe(
            TimelineEventDispatcher::EventType::ClipAdded,
            [this](const TimelineEvent& event) { on_clip_added(event); });
            
        event_dispatcher_->subscribe(
            TimelineEventDispatcher::EventType::SelectionChanged,
            [this](const TimelineEvent& event) { on_selection_changed(event); });
    }
};
```

### 2. Command Pattern for Undo/Redo

```cpp
// Command interface for undo/redo
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string get_description() const = 0;
};

// Concrete command example
class AddClipCommand : public Command {
    Timeline* timeline_;
    VideoClip clip_data_;
    entity_t clip_entity_;
    
public:
    AddClipCommand(Timeline* timeline, const VideoClip& clip)
        : timeline_(timeline), clip_data_(clip), clip_entity_(entt::null) {}
    
    void execute() override {
        clip_entity_ = timeline_->add_video_clip(clip_data_);
    }
    
    void undo() override {
        if (clip_entity_ != entt::null) {
            timeline_->remove_clip(clip_entity_);
        }
    }
    
    std::string get_description() const override {
        return "Add clip: " + clip_data_.file.path;
    }
};

// Command manager
class CommandManager {
    std::vector<std::unique_ptr<Command>> command_history_;
    size_t current_position_ = 0;
    
public:
    void execute_command(std::unique_ptr<Command> command) {
        // Clear redo history
        command_history_.erase(command_history_.begin() + current_position_, 
                           command_history_.end());
        
        // Execute command
        command->execute();
        
        // Add to history
        command_history_.push_back(std::move(command));
        current_position_++;
        
        // Limit history size
        if (command_history_.size() > MAX_UNDO_STACK) {
            command_history_.erase(command_history_.begin());
            current_position_--;
        }
    }
    
    void undo() {
        if (can_undo()) {
            current_position_--;
            command_history_[current_position_]->undo();
        }
    }
    
    void redo() {
        if (can_redo()) {
            command_history_[current_position_]->execute();
            current_position_++;
        }
    }
};
```

### 3. Factory Pattern for Plugin System

```cpp
// Plugin interface
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual Result initialize(const PluginContext& context) = 0;
    virtual std::vector<Effect*> get_effects() = 0;
    virtual std::string get_name() const = 0;
    virtual std::string get_version() const = 0;
};

// Plugin factory
class PluginFactory {
public:
    static std::unique_ptr<Plugin> load_plugin(const std::string& plugin_path) {
        // Load library in sandbox
        SandboxedLibrary library(plugin_path);
        
        // Get plugin entry point
        auto create_plugin = library.get_function<Plugin*()>("create_plugin");
        if (!create_plugin) {
            return nullptr;
        }
        
        // Create plugin instance
        auto plugin = std::unique_ptr<Plugin>(create_plugin());
        
        // Validate plugin
        if (!validate_plugin(plugin.get())) {
            return nullptr;
        }
        
        return plugin;
    }
    
private:
    static bool validate_plugin(Plugin* plugin) {
        // Check plugin interface
        if (!plugin->get_name().empty() && 
            !plugin->get_version().empty()) {
            return true;
        }
        return false;
    }
};
```

## Performance Optimization Patterns

### 1. Memory Pool Allocation

```cpp
// Memory pool for frequent allocations
template<typename T, size_t PoolSize = 1024>
class MemoryPool {
    alignas(T) std::array<std::byte, sizeof(T) * PoolSize> pool_;
    std::bitset<PoolSize> used_blocks_;
    
public:
    T* allocate() {
        size_t index = find_free_block();
        if (index == PoolSize) {
            return new T(); // Fallback to heap
        }
        
        used_blocks_[index] = true;
        return reinterpret_cast<T*>(&pool_[index * sizeof(T)]);
    }
    
    void deallocate(T* ptr) {
        if (is_from_pool(ptr)) {
            size_t index = (reinterpret_cast<std::byte*>(ptr) - pool_.data()) / sizeof(T);
            used_blocks_[index] = false;
        } else {
            delete ptr; // Heap allocation
        }
    }
    
private:
    size_t find_free_block() const {
        for (size_t i = 0; i < PoolSize; ++i) {
            if (!used_blocks_[i]) {
                return i;
            }
        }
        return PoolSize;
    }
    
    bool is_from_pool(T* ptr) const {
        auto ptr_bytes = reinterpret_cast<std::byte*>(ptr);
        return ptr_bytes >= pool_.data() && 
               ptr_bytes < pool_.data() + pool_.size();
    }
};
```

### 2. Lock-Free Data Structures

```cpp
// Lock-free queue for frame processing
template<typename T>
class LockFreeQueue {
    struct Node {
        std::atomic<T*> data;
        std::atomic<Node*> next;
        
        Node() : data(nullptr), next(nullptr) {}
    };
    
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    
public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }
    
    void enqueue(T* item) {
        Node* new_node = new Node;
        new_node->data.store(item);
        
        Node* old_tail;
        Node* old_tail_next;
        
        do {
            old_tail = tail_.load();
            old_tail_next = old_tail->next.load();
        } while (old_tail_next != nullptr);
        
        if (old_tail->next.compare_exchange_weak(old_tail_next, new_node)) {
            tail_.compare_exchange_weak(old_tail, new_node);
        }
    }
    
    T* dequeue() {
        Node* old_head;
        Node* new_head;
        T* data;
        
        do {
            old_head = head_.load();
            new_head = old_head->next.load();
            
            if (new_head == nullptr) {
                return nullptr; // Queue is empty
            }
            
            data = new_head->data.load();
        } while (!head_.compare_exchange_weak(old_head, new_head));
        
        delete old_head;
        return data;
    }
};
```

## Error Handling Patterns

### 1. Result Type for Error Propagation

```cpp
// Result type for error handling
template<typename T>
class Result {
public:
    enum class Status {
        Success,
        InvalidParameter,
        OutOfMemory,
        FileNotFound,
        CodecError,
        GPUError,
        NetworkError,
        UnknownError
    };
    
private:
    Status status_;
    std::optional<T> value_;
    std::string error_message_;
    
public:
    Result(T&& value) : status_(Status::Success), value_(std::move(value)) {}
    Result(Status status, const std::string& message = "") 
        : status_(status), error_message_(message) {}
    
    bool is_success() const { return status_ == Status::Success; }
    bool is_error() const { return !is_success(); }
    
    Status get_status() const { return status_; }
    const std::string& get_error_message() const { return error_message_; }
    
    T& get_value() {
        assert(is_success());
        return *value_;
    }
    
    const T& get_value() const {
        assert(is_success());
        return *value_;
    }
    
    // Chaining operations
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(get_value()))> {
        if (is_success()) {
            return Result<decltype(func(get_value()))>(func(get_value()));
        } else {
            return Result<decltype(func(get_value()))>(status_, error_message_);
        }
    }
};

// Usage
Result<GPUFrame> decode_video_frame(const EncodedData& data) {
    if (data.empty()) {
        return Result<GPUFrame>(Result<Status>::InvalidParameter, "Empty input data");
    }
    
    GPUFrame frame;
    if (!decoder_.decode_frame(data, frame)) {
        return Result<GPUFrame>(Result<Status>::CodecError, "Decoding failed");
    }
    
    return Result<GPUFrame>(std::move(frame));
}
```

---

**Document Status:** Active  
**Last Updated:** November 13, 2025  
**Next Review:** Architecture review (Monthly)  
**Stakeholders:** Development team, software architects, system designers
