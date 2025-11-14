#pragma once

// Audio Processing System - Professional Audio Pipeline
// Real-time audio processing, mixing, and effects

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>
#include <atomic>

namespace Shotcut::Audio {

// ============================================================================
// AUDIO TYPES AND CONSTANTS
// ============================================================================

/// Audio sample formats
enum class AudioFormat {
    UNKNOWN = 0,
    PCM_S16 = 1,        // 16-bit signed integer
    PCM_S24 = 2,        // 24-bit signed integer
    PCM_S32 = 3,        // 32-bit signed integer
    PCM_F32 = 4,        // 32-bit float
    PCM_F64 = 5,        // 64-bit double
};

/// Audio channel layouts
enum class ChannelLayout {
    UNKNOWN = 0,
    MONO = 1,
    STEREO = 2,
    STEREO_CENTER = 3,      // Left, Right, Center
    STEREO_REAR = 4,        // Left, Right, Rear Left, Rear Right
    SURROUND_5_1 = 6,       // 5.1 surround
    SURROUND_7_1 = 8,        // 7.1 surround
    CUSTOM = 255,
};

/// Audio processing quality levels
enum class AudioQuality {
    LOW = 0,           // 22.05 kHz, 8-bit (for performance)
    MEDIUM = 1,        // 44.1 kHz, 16-bit (standard)
    HIGH = 2,          // 48 kHz, 24-bit (professional)
    ULTRA = 3,         // 96 kHz, 32-bit float (studio)
    CUSTOM = 4,
};

/// Sample rates for different quality levels
constexpr uint32_t SAMPLE_RATES[] = {
    22050,   // LOW
    44100,   // MEDIUM
    48000,   // HIGH
    96000    // ULTRA
};

/// Bit depths for different quality levels
constexpr uint8_t BIT_DEPTHS[] = {
    16,      // LOW
    16,      // MEDIUM
    24,      // HIGH
    32       // ULTRA
};

// ============================================================================
// AUDIO BUFFER MANAGEMENT
// ============================================================================

/// Audio buffer description
struct AudioBufferDesc {
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    AudioFormat format = AudioFormat::PCM_S16;
    ChannelLayout layout = ChannelLayout::STEREO;
    uint32_t frames = 0;  // Number of sample frames
    bool interleaved = true;  // Interleaved vs planar layout
};

/// Audio buffer for holding audio data
class AudioBuffer {
public:
    AudioBuffer();
    AudioBuffer(const AudioBufferDesc& desc);
    ~AudioBuffer();
    
    // Buffer management
    bool allocate(const AudioBufferDesc& desc);
    void deallocate();
    bool is_allocated() const { return data_ != nullptr; }
    
    // Accessors
    const AudioBufferDesc& get_desc() const { return desc_; }
    float* get_data() { return data_.get(); }
    const float* get_data() const { return data_.get(); }
    size_t get_size() const { return data_size_; }
    uint32_t get_frame_count() const { return desc_.frames; }
    uint32_t get_channel_count() const { return desc_.channels; }
    
    // Format helpers
    size_t get_sample_size() const;
    size_t get_frame_size() const;
    size_t get_channel_size() const;
    
    // Data manipulation
    void clear();
    void silence();
    bool copy_from(const AudioBuffer& other);
    bool mix_from(const AudioBuffer& other, float gain = 1.0f);
    
    // Format conversion
    bool convert_to(AudioFormat target_format);
    bool convert_to_interleaved(bool interleaved);
    bool resample_to(uint32_t new_sample_rate);
    
private:
    AudioBufferDesc desc_;
    std::unique_ptr<float[]> data_;
    size_t data_size_ = 0;
    bool owns_data_ = false;
    
    // Helper methods
    size_t calculate_buffer_size(const AudioBufferDesc& desc) const;
    bool allocate_data(size_t size);
    void deallocate_data();
};

// ============================================================================
// AUDIO EFFECTS
// ============================================================================

/// Base audio effect interface
class AudioEffect {
public:
    AudioEffect(const std::string& name);
    virtual ~AudioEffect() = default;
    
    // Effect identification
    const std::string& get_name() const { return name_; }
    const std::string& get_type() const { return type_; }
    
    // Processing
    virtual bool process(AudioBuffer& buffer, float time = 0.0f) = 0;
    
    // Parameters
    virtual bool set_parameter(const std::string& name, float value) = 0;
    virtual float get_parameter(const std::string& name) const = 0;
    virtual std::vector<std::string> get_parameter_names() const = 0;
    
    // State
    virtual void reset() = 0;
    virtual bool is_enabled() const { return enabled_; }
    virtual void set_enabled(bool enabled_param) { enabled_ = enabled_param; }

protected:
    std::string name_;
    std::string type_;
    bool enabled_ = true;
};

/// Volume/gain control effect
class VolumeEffect : public AudioEffect {
public:
    VolumeEffect();
    
    bool process(AudioBuffer& buffer, float time = 0.0f) override;
    bool set_parameter(const std::string& name, float value) override;
    float get_parameter(const std::string& name) const override;
    std::vector<std::string> get_parameter_names() const override;
    void reset() override;
    
private:
    float gain_ = 1.0f;
    float mute_gain_ = 0.0f;
    bool muted_ = false;
    
    void apply_gain(AudioBuffer& buffer, float gain);
};

/// Equalizer effect with multiple frequency bands
class EqualizerEffect : public AudioEffect {
public:
    EqualizerEffect(uint32_t bands = 10);
    
    bool process(AudioBuffer& buffer, float time = 0.0f) override;
    bool set_parameter(const std::string& name, float value) override;
    float get_parameter(const std::string& name) const override;
    std::vector<std::string> get_parameter_names() const override;
    void reset() override;
    
    struct Band {
        float frequency;      // Center frequency
        float gain;          // Gain in dB
        float q;             // Q factor
        bool enabled;
    };
    
    const std::vector<Band>& get_bands() const { return bands_; }
    void set_band(uint32_t index, const Band& band);

private:
    std::vector<Band> bands_;
    std::vector<float> filter_coeffs_;
    std::vector<std::vector<float>> filter_state_;
    uint32_t sample_rate_;
    
    void update_filter_coeffs();
    void apply_biquad_filter(float* samples, size_t count, 
                           const float* coeffs, std::vector<float>& state);
};

/// Compressor/Limiter effect for dynamic range control
class CompressorEffect : public AudioEffect {
public:
    CompressorEffect();
    
    bool process(AudioBuffer& buffer, float time = 0.0f) override;
    bool set_parameter(const std::string& name, float value) override;
    float get_parameter(const std::string& name) const override;
    std::vector<std::string> get_parameter_names() const override;
    void reset() override;
    
private:
    float threshold_ = -20.0f;    // Threshold in dB
    float ratio_ = 4.0f;          // Compression ratio
    float attack_time_ = 0.003f;  // Attack time in seconds
    float release_time_ = 0.1f;   // Release time in seconds
    float makeup_gain_ = 0.0f;    // Makeup gain in dB
    float knee_width_ = 2.0f;     // Knee width in dB
    
    // Envelope follower state
    std::vector<float> envelope_;
    float attack_coeff_;
    float release_coeff_;
    uint32_t sample_rate_;
    
    void update_coefficients(uint32_t sample_rate);
    float apply_compression(float input_sample, float& envelope);
};

/// Reverb effect using convolution or algorithmic approach
class ReverbEffect : public AudioEffect {
public:
    enum class ReverbType {
        HALL,
        ROOM,
        PLATE,
        SPRING,
        CONVOLUTION
    };
    
    ReverbEffect(ReverbType type = ReverbType::HALL);
    
    bool process(AudioBuffer& buffer, float time = 0.0f) override;
    bool set_parameter(const std::string& name, float value) override;
    float get_parameter(const std::string& name) const override;
    std::vector<std::string> get_parameter_names() const override;
    void reset() override;
    
    bool load_impulse_response(const std::string& file_path);
    void set_impulse_response(const AudioBuffer& ir_buffer);

private:
    ReverbType reverb_type_;
    uint32_t sample_rate_;
    float room_size_ = 0.5f;
    float damping_ = 0.5f;
    float wet_level_ = 0.3f;
    float dry_level_ = 0.7f;
    float width_ = 1.0f;
    
    // Algorithmic reverb state
    std::vector<std::vector<float>> delay_lines_;
    std::vector<uint32_t> delay_indices_;
    std::vector<uint32_t> delay_lengths_;
    std::vector<std::vector<float>> filter_states_;
    
    // Convolution reverb state
    AudioBuffer impulse_response_;
    std::vector<std::vector<float>> convolution_buffer_;
    
    void initialize_algorithmic_reverb(uint32_t sample_rate, uint32_t channels);
    void process_algorithmic_reverb(AudioBuffer& buffer);
    void process_convolution_reverb(AudioBuffer& buffer);
};

// ============================================================================
// AUDIO MIXER
// ============================================================================

/// Audio channel/bus for mixing
class AudioChannel {
public:
    AudioChannel(const std::string& name, uint32_t channels = 2);
    ~AudioChannel() = default;
    
    // Channel properties
    const std::string& get_name() const { return name_; }
    uint32_t get_channel_count() const { return channels_; }
    float get_volume() const { return volume_; }
    bool is_muted() const { return muted_; }
    bool is_soloed() const { return soloed_; }
    
    // Channel control
    void set_volume(float volume) { volume_ = volume; }
    void set_muted(bool muted) { muted_ = muted; }
    void set_soloed(bool soloed) { soloed_ = soloed; }
    void set_pan(float pan) { pan_ = pan; }  // -1.0 (left) to 1.0 (right)
    
    // Effect processing
    void add_effect(std::shared_ptr<AudioEffect> effect);
    void remove_effect(size_t index);
    void remove_effect(const std::string& name);
    AudioEffect* get_effect(size_t index) const;
    AudioEffect* get_effect(const std::string& name) const;
    size_t get_effect_count() const { return effects_.size(); }
    
    // Audio processing
    bool process_audio(AudioBuffer& input_buffer, AudioBuffer& output_buffer, float time);
    void reset();
    
    // Input/Output routing
    void set_input_bus(uint32_t bus) { input_bus_ = bus; }
    void set_output_bus(uint32_t bus) { output_bus_ = bus; }
    uint32_t get_input_bus() const { return input_bus_; }
    uint32_t get_output_bus() const { return output_bus_; }

private:
    std::string name_;
    uint32_t channels_;
    float volume_ = 1.0f;
    bool muted_ = false;
    bool soloed_ = false;
    float pan_ = 0.0f;  // Center
    
    std::vector<std::shared_ptr<AudioEffect>> effects_;
    uint32_t input_bus_ = 0;
    uint32_t output_bus_ = 0;
    
    bool apply_volume_and_pan(AudioBuffer& buffer);
};

/// Audio mixer for mixing multiple channels
class AudioMixer {
public:
    AudioMixer(uint32_t max_channels = 64, uint32_t sample_rate = 44100);
    ~AudioMixer() = default;
    
    // Mixer properties
    uint32_t get_sample_rate() const { return sample_rate_; }
    uint32_t get_channel_count() const { return output_channels_; }
    float get_master_volume() const { return master_volume_; }
    
    // Mixer control
    void set_sample_rate(uint32_t sample_rate);
    void set_master_volume(float volume) { master_volume_ = volume; }
    void set_output_channels(uint32_t channels) { output_channels_ = channels; }
    
    // Channel management
    std::shared_ptr<AudioChannel> create_channel(const std::string& name, uint32_t channels = 2);
    void remove_channel(const std::string& name);
    std::shared_ptr<AudioChannel> get_channel(const std::string& name) const;
    std::shared_ptr<AudioChannel> get_channel(size_t index) const;
    size_t get_channel_count() const { return channels_.size(); }
    std::vector<std::string> get_channel_names() const;
    
    // Audio processing
    bool process_mix(float** input_buffers, uint32_t input_channels, 
                   uint32_t frames, float** output_buffers, 
                   uint32_t output_channels, float time);
    
    bool process_buffers(const std::vector<AudioBuffer>& input_buffers,
                        AudioBuffer& output_buffer, float time);
    
    // Bypass and reset
    void set_bypass(bool bypass) { bypass_ = bypass; }
    bool is_bypassed() const { return bypass_; }
    void reset_all_channels();
    
private:
    uint32_t max_channels_;
    uint32_t sample_rate_;
    uint32_t output_channels_;
    float master_volume_ = 1.0f;
    bool bypass_ = false;
    
    std::vector<std::shared_ptr<AudioChannel>> channels_;
    std::unordered_map<std::string, std::shared_ptr<AudioChannel>> channel_map_;
    
    AudioBuffer mix_buffer_;
    std::vector<float> temp_mix_buffer_;
    
    void update_buffer_sizes();
};

// ============================================================================
// AUDIO PROCESSOR
// ============================================================================

/// Main audio processor for the application
class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();
    
    // Initialization
    bool initialize(const AudioBufferDesc& output_desc);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Audio format
    const AudioBufferDesc& get_output_format() const { return output_format_; }
    bool set_output_format(const AudioBufferDesc& desc);
    
    // Audio processing
    bool process_frame(float** input_buffers, uint32_t input_channels,
                      uint32_t frames, float time = 0.0f);
    
    bool process_buffers(const std::vector<AudioBuffer>& input_buffers,
                        AudioBuffer& output_buffer, float time = 0.0f);
    
    // Master mixer access
    AudioMixer& get_mixer() { return master_mixer_; }
    const AudioMixer& get_mixer() const { return master_mixer_; }
    
    // Monitoring and metering
    struct AudioLevels {
        float left_peak = 0.0f;
        float right_peak = 0.0f;
        float left_rms = 0.0f;
        float right_rms = 0.0f;
        bool clipping = false;
    };
    
    const AudioLevels& get_current_levels() const { return current_levels_; }
    
    // Configuration
    void set_quality(AudioQuality quality);
    AudioQuality get_quality() const { return quality_; }
    
    // Real-time parameters
    void set_latency_compensation(float latency_seconds) { latency_compensation_ = latency_seconds; }
    float get_latency_compensation() const { return latency_compensation_; }

private:
    bool initialized_ = false;
    AudioBufferDesc output_format_;
    AudioQuality quality_ = AudioQuality::HIGH;
    AudioMixer master_mixer_;
    AudioBuffer output_buffer_;
    
    // Monitoring
    AudioLevels current_levels_;
    std::vector<float> level_history_;
    size_t level_history_size_ = 100;
    
    // Real-time processing
    float latency_compensation_ = 0.0f;
    float current_time_ = 0.0f;
    
    // Internal methods
    void update_levels(AudioBuffer& buffer);
    void reset_levels();
};

// ============================================================================
// AUDIO PROCESSOR FACTORY
// ============================================================================

/// Factory for creating audio effects
class AudioEffectFactory {
public:
    // Effect creation
    static std::unique_ptr<AudioEffect> create_volume_effect();
    static std::unique_ptr<AudioEffect> create_equalizer_effect(uint32_t bands = 10);
    static std::unique_ptr<AudioEffect> create_compressor_effect();
    static std::unique_ptr<AudioEffect> create_reverb_effect(ReverbEffect::ReverbType type);
    
    // Built-in effects
    static std::vector<std::string> get_available_effects();
    static std::unique_ptr<AudioEffect> create_effect(const std::string& type);
    
    // Parameter information
    struct EffectInfo {
        std::string name;
        std::string type;
        std::string description;
        std::vector<std::string> parameters;
    };
    
    static std::vector<EffectInfo> get_effect_info();
    static EffectInfo get_effect_info(const std::string& type);
};

} // namespace Shotcut::Audio
