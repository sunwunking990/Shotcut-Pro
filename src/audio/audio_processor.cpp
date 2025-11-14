// Audio Processing System Implementation
// Professional audio processing, mixing, and effects

#include "../include/audio/audio_processor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

namespace Shotcut::Audio {

// ============================================================================
// AUDIO BUFFER IMPLEMENTATION
// ============================================================================

AudioBuffer::AudioBuffer() = default;

AudioBuffer::AudioBuffer(const AudioBufferDesc& desc) {
    allocate(desc);
}

AudioBuffer::~AudioBuffer() {
    deallocate();
}

bool AudioBuffer::allocate(const AudioBufferDesc& desc) {
    deallocate();
    
    desc_ = desc;
    data_size_ = calculate_buffer_size(desc);
    
    if (!allocate_data(data_size_)) {
        return false;
    }
    
    owns_data_ = true;
    return true;
}

void AudioBuffer::deallocate() {
    if (data_ && owns_data_) {
        deallocate_data();
    }
    
    data_ = nullptr;
    data_size_ = 0;
    owns_data_ = false;
}

size_t AudioBuffer::get_sample_size() const {
    switch (desc_.format) {
        case AudioFormat::PCM_S16: return 2;
        case AudioFormat::PCM_S24: return 3;
        case AudioFormat::PCM_S32: return 4;
        case AudioFormat::PCM_F32: return 4;
        case AudioFormat::PCM_F64: return 8;
        default: return 0;
    }
}

size_t AudioBuffer::get_frame_size() const {
    return get_sample_size() * desc_.channels;
}

size_t AudioBuffer::get_channel_size() const {
    return get_sample_size() * desc_.frames;
}

void AudioBuffer::clear() {
    if (data_ && data_size_ > 0) {
        std::memset(data_.get(), 0, data_size_);
    }
}

void AudioBuffer::silence() {
    clear();
}

bool AudioBuffer::copy_from(const AudioBuffer& other) {
    if (desc_.format != other.desc_.format ||
        desc_.channels != other.desc_.channels ||
        desc_.frames != other.desc_.frames) {
        return false;
    }
    
    if (data_size_ != other.data_size_) {
        return false;
    }
    
    if (data_ && other.data_) {
        std::memcpy(data_.get(), other.data_.get(), data_size_);
        return true;
    }
    
    return false;
}

bool AudioBuffer::mix_from(const AudioBuffer& other, float gain) {
    if (desc_.format != AudioFormat::PCM_F32 || 
        other.desc_.format != AudioFormat::PCM_F32 ||
        desc_.channels != other.desc_.channels ||
        desc_.frames != other.desc_.frames) {
        return false;
    }
    
    float* dest = data_.get();
    const float* src = other.data_.get();
    size_t samples = desc_.frames * desc_.channels;
    
    for (size_t i = 0; i < samples; ++i) {
        dest[i] += src[i] * gain;
    }
    
    return true;
}

bool AudioBuffer::convert_to(AudioFormat target_format) {
    if (desc_.format == target_format) {
        return true;
    }
    
    // Simplified format conversion - full implementation would handle all formats
    if (desc_.format == AudioFormat::PCM_S16 && target_format == AudioFormat::PCM_F32) {
        const int16_t* src = reinterpret_cast<const int16_t*>(data_.get());
        
        // Calculate new size and allocate
        size_t sample_count = desc_.frames * desc_.channels;
        size_t new_size = sample_count * sizeof(float);
        
        try {
            auto new_data = std::make_unique<float[]>(sample_count);
            
            // Convert samples
            for (size_t i = 0; i < sample_count; ++i) {
                new_data[i] = static_cast<float>(src[i]) / 32768.0f;
            }
            
            // Update buffer
            data_ = std::move(new_data);
            data_size_ = new_size;
            desc_.format = target_format;
            
            return true;
        } catch (const std::bad_alloc&) {
            return false;
        }
    }
    
    return false;
}

size_t AudioBuffer::calculate_buffer_size(const AudioBufferDesc& desc) const {
    return static_cast<size_t>(desc.frames) * desc.channels * get_sample_size();
}

bool AudioBuffer::allocate_data(size_t size) {
    deallocate_data();
    
    // Calculate number of float samples needed
    size_t sample_count = size / sizeof(float);
    
    try {
        data_ = std::make_unique<float[]>(sample_count);
        return true;
    } catch (const std::bad_alloc&) {
        data_.reset();
        return false;
    }
}

void AudioBuffer::deallocate_data() {
    data_.reset();
}

// ============================================================================
// EQUALIZER EFFECT IMPLEMENTATION
// ============================================================================

EqualizerEffect::EqualizerEffect(uint32_t bands) : AudioEffect("Equalizer") {
    type_ = "equalizer";
    sample_rate_ = 44100;
    
    // Initialize bands with default frequencies (geometric spacing)
    bands_.resize(bands);
    filter_coeffs_.resize(bands * 5); // 5 coefficients per biquad
    filter_state_.resize(bands * 2);  // 2 state variables per biquad
    
    float min_freq = 20.0f;    // 20 Hz minimum
    float max_freq = 20000.0f;  // 20 kHz maximum
    
    for (uint32_t i = 0; i < bands; ++i) {
        float freq_ratio = static_cast<float>(i) / (bands - 1);
        float log_freq = std::log10(min_freq) + freq_ratio * (std::log10(max_freq) - std::log10(min_freq));
        
        bands_[i] = {
            std::pow(10.0f, log_freq),  // frequency
            0.0f,                         // gain (dB)
            1.0f,                         // Q factor
            true                            // enabled
        };
    }
    
    update_filter_coeffs();
}

bool EqualizerEffect::process(AudioBuffer& buffer, float time) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32) {
        return false;
    }
    
    float* samples = static_cast<float*>(buffer.get_data());
    uint32_t frame_count = buffer.get_frame_count();
    uint32_t channels = buffer.get_channel_count();
    
    // Process each channel
    for (uint32_t ch = 0; ch < channels; ++ch) {
        for (uint32_t band = 0; band < bands_.size(); ++band) {
            if (!bands_[band].enabled) continue;
            
            const float* coeffs = &filter_coeffs_[band * 5];
            std::vector<float>& state = filter_state_[band];
            
            apply_biquad_filter(samples + ch, frame_count * channels, coeffs, state);
        }
    }
    
    return true;
}

bool EqualizerEffect::set_parameter(const std::string& name, float value) {
    if (name == "band_gain") {
        // Set gain for all bands (value is dB)
        for (auto& band : bands_) {
            band.gain = value;
        }
        update_filter_coeffs();
        return true;
    } else if (name == "q_factor") {
        // Set Q factor for all bands
        for (auto& band : bands_) {
            band.q = value;
        }
        update_filter_coeffs();
        return true;
    }
    
    return false;
}

float EqualizerEffect::get_parameter(const std::string& name) const {
    if (name == "band_count") {
        return static_cast<float>(bands_.size());
    } else if (name == "band_gain") {
        return bands_.empty() ? 0.0f : bands_[0].gain;
    } else if (name == "q_factor") {
        return bands_.empty() ? 1.0f : bands_[0].q;
    }
    
    return 0.0f;
}

std::vector<std::string> EqualizerEffect::get_parameter_names() const {
    return {"band_count", "band_gain", "q_factor"};
}

void EqualizerEffect::reset() {
    for (auto& state : filter_state_) {
        state.clear();
    }
}

void EqualizerEffect::set_band(uint32_t index, const Band& band) {
    if (index < bands_.size()) {
        bands_[index] = band;
        update_filter_coeffs();
    }
}

void EqualizerEffect::update_filter_coeffs() {
    const float pi = 3.14159265358979323846f;
    
    for (uint32_t i = 0; i < bands_.size(); ++i) {
        const auto& band = bands_[i];
        float* coeffs = &filter_coeffs_[i * 5];
        
        // Convert gain from dB to linear
        float A = std::pow(10.0f, band.gain / 40.0f);
        
        // Normalize frequency
        float omega = 2.0f * pi * band.frequency / sample_rate_;
        float sinw = std::sin(omega);
        float cosw = std::cos(omega);
        float alpha = sinw / (2.0f * band.q);
        
        // Peaking EQ filter coefficients
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw;
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw;
        float a2 = 1.0f - alpha / A;
        
        // Normalize coefficients
        coeffs[0] = b0 / a0;
        coeffs[1] = b1 / a0;
        coeffs[2] = b2 / a0;
        coeffs[3] = a1 / a0;
        coeffs[4] = a2 / a0;
    }
}

void EqualizerEffect::apply_biquad_filter(float* samples, size_t count, 
                                   const float* coeffs, std::vector<float>& state) {
    // Direct Form II biquad implementation
    float x1 = state.size() > 0 ? state[0] : 0.0f;
    float x2 = state.size() > 1 ? state[1] : 0.0f;
    float y1 = state.size() > 2 ? state[2] : 0.0f;
    float y2 = state.size() > 3 ? state[3] : 0.0f;
    
    for (size_t n = 0; n < count; ++n) {
        float x0 = samples[n];
        
        // Apply biquad difference equation
        float y0 = coeffs[0] * x0 + coeffs[1] * x1 + coeffs[2] * x2 
                  - coeffs[3] * y1 - coeffs[4] * y2;
        
        samples[n] = y0;
        
        // Update delay lines
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;
    }
    
    // Store state for next call
    if (state.size() < 4) {
        state.resize(4);
    }
    state[0] = x1;
    state[1] = x2;
    state[2] = y1;
    state[3] = y2;
}

// ============================================================================
// VOLUME EFFECT IMPLEMENTATION
// ============================================================================

VolumeEffect::VolumeEffect() : AudioEffect("Volume") {
    type_ = "volume";
}

bool VolumeEffect::process(AudioBuffer& buffer, float time) {
    if (!buffer.is_allocated()) {
        return false;
    }
    
    float effective_gain = muted_ ? mute_gain_ : gain_;
    apply_gain(buffer, effective_gain);
    
    return true;
}

bool VolumeEffect::set_parameter(const std::string& name, float value) {
    if (name == "gain") {
        gain_ = value;
        return true;
    } else if (name == "muted") {
        muted_ = (value > 0.5f);
        return true;
    }
    return false;
}

float VolumeEffect::get_parameter(const std::string& name) const {
    if (name == "gain") {
        return gain_;
    } else if (name == "muted") {
        return muted_ ? 1.0f : 0.0f;
    }
    return 0.0f;
}

std::vector<std::string> VolumeEffect::get_parameter_names() const {
    return {"gain", "muted"};
}

void VolumeEffect::reset() {
    gain_ = 1.0f;
    muted_ = false;
    mute_gain_ = 0.0f;
}

void VolumeEffect::apply_gain(AudioBuffer& buffer, float gain) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32) {
        return;
    }
    
    float* samples = static_cast<float*>(buffer.get_data());
    size_t sample_count = buffer.get_frame_count() * buffer.get_channel_count();
    
    for (size_t i = 0; i < sample_count; ++i) {
        samples[i] *= gain;
    }
}

// ============================================================================
// AUDIO CHANNEL IMPLEMENTATION
// ============================================================================

AudioChannel::AudioChannel(const std::string& name, uint32_t channels)
    : name_(name)
    , channels_(channels) {
}

void AudioChannel::add_effect(std::shared_ptr<AudioEffect> effect) {
    effects_.push_back(effect);
}

void AudioChannel::remove_effect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void AudioChannel::remove_effect(const std::string& name) {
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
                      [&name](const std::shared_ptr<AudioEffect>& effect) {
                          return effect && effect->get_name() == name;
                      }),
        effects_.end());
}

AudioEffect* AudioChannel::get_effect(size_t index) const {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

AudioEffect* AudioChannel::get_effect(const std::string& name) const {
    auto it = std::find_if(effects_.begin(), effects_.end(),
                           [&name](const std::shared_ptr<AudioEffect>& effect) {
                               return effect && effect->get_name() == name;
                           });
    return it != effects_.end() ? it->get() : nullptr;
}

bool AudioChannel::process_audio(AudioBuffer& input_buffer, AudioBuffer& output_buffer, float time) {
    if (!input_buffer.is_allocated()) {
        return false;
    }
    
    // Copy input to output if different buffers
    if (&input_buffer != &output_buffer) {
        if (!output_buffer.is_allocated()) {
            output_buffer.allocate(input_buffer.get_desc());
        }
        output_buffer.copy_from(input_buffer);
    }
    
    // Apply effects in order
    for (auto& effect : effects_) {
        if (effect && effect->is_enabled()) {
            effect->process(output_buffer, time);
        }
    }
    
    // Apply volume and pan
    apply_volume_and_pan(output_buffer);
    
    return true;
}

void AudioChannel::reset() {
    for (auto& effect : effects_) {
        if (effect) {
            effect->reset();
        }
    }
}

bool AudioChannel::apply_volume_and_pan(AudioBuffer& buffer) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32 ||
        buffer.get_channel_count() < 2) {
        return false;
    }
    
    float* samples = static_cast<float*>(buffer.get_data());
    uint32_t frame_count = buffer.get_frame_count();
    
    float left_gain = volume_ * (pan_ <= 0.0f ? 1.0f : 1.0f - pan_);
    float right_gain = volume_ * (pan_ >= 0.0f ? 1.0f : 1.0f + pan_);
    
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        uint32_t sample_idx = frame * buffer.get_channel_count();
        
        // Apply stereo balance
        samples[sample_idx] *= left_gain;     // Left channel
        samples[sample_idx + 1] *= right_gain; // Right channel
        
        // Apply to additional channels if present
        for (uint32_t ch = 2; ch < buffer.get_channel_count(); ++ch) {
            samples[sample_idx + ch] *= volume_;
        }
    }
    
    return true;
}

// ============================================================================
// AUDIO MIXER IMPLEMENTATION
// ============================================================================

AudioMixer::AudioMixer(uint32_t max_channels, uint32_t sample_rate)
    : max_channels_(max_channels)
    , sample_rate_(sample_rate)
    , output_channels_(2) {
    
    channels_.reserve(max_channels_);
    update_buffer_sizes();
}

void AudioMixer::set_sample_rate(uint32_t sample_rate) {
    if (sample_rate_ != sample_rate) {
        sample_rate_ = sample_rate;
        update_buffer_sizes();
        
        // Update all channels
        for (auto& channel : channels_) {
            channel->reset();
        }
    }
}

std::shared_ptr<AudioChannel> AudioMixer::create_channel(const std::string& name, uint32_t channels) {
    if (channels_.size() >= max_channels_) {
        return nullptr;
    }
    
    auto channel = std::make_shared<AudioChannel>(name, channels);
    channels_.push_back(channel);
    channel_map_[name] = channel;
    
    return channel;
}

void AudioMixer::remove_channel(const std::string& name) {
    auto it = channel_map_.find(name);
    if (it != channel_map_.end()) {
        auto channel = it->second;
        
        // Remove from map
        channel_map_.erase(it);
        
        // Remove from vector
        channels_.erase(
            std::remove_if(channels_.begin(), channels_.end(),
                          [&channel](const std::weak_ptr<AudioChannel>& weak_channel) {
                              return weak_channel.lock() == channel;
                          }),
            channels_.end());
    }
}

std::shared_ptr<AudioChannel> AudioMixer::get_channel(const std::string& name) const {
    auto it = channel_map_.find(name);
    return it != channel_map_.end() ? it->second : nullptr;
}

std::shared_ptr<AudioChannel> AudioMixer::get_channel(size_t index) const {
    return index < channels_.size() ? channels_[index] : nullptr;
}

std::vector<std::string> AudioMixer::get_channel_names() const {
    std::vector<std::string> names;
    names.reserve(channels_.size());
    
    for (const auto& channel : channels_) {
        if (channel) {
            names.push_back(channel->get_name());
        }
    }
    
    return names;
}

bool AudioMixer::process_buffers(const std::vector<AudioBuffer>& input_buffers,
                                 AudioBuffer& output_buffer, float time) {
    if (bypass_) {
        if (!input_buffers.empty()) {
            output_buffer.copy_from(input_buffers[0]);
        }
        return true;
    }
    
    // Initialize output buffer
    if (!output_buffer.is_allocated()) {
        AudioBufferDesc desc;
        desc.sample_rate = sample_rate_;
        desc.channels = output_channels_;
        desc.format = AudioFormat::PCM_F32;
        desc.frames = input_buffers.empty() ? 0 : input_buffers[0].get_frame_count();
        
        if (!output_buffer.allocate(desc)) {
            return false;
        }
    }
    
    // Clear output buffer
    output_buffer.clear();
    
    // Mix all input buffers
    for (size_t i = 0; i < input_buffers.size() && i < channels_.size(); ++i) {
        if (channels_[i]) {
            AudioBuffer temp_buffer;
            if (channels_[i]->process_audio(input_buffers[i], temp_buffer, time)) {
                output_buffer.mix_from(temp_buffer, 1.0f);
            }
        }
    }
    
    // Apply master volume
    if (output_buffer.get_desc().format == AudioFormat::PCM_F32) {
        float* samples = static_cast<float*>(output_buffer.get_data());
        size_t sample_count = output_buffer.get_frame_count() * output_buffer.get_channel_count();
        
        for (size_t i = 0; i < sample_count; ++i) {
            samples[i] *= master_volume_;
        }
    }
    
    return true;
}

void AudioMixer::update_buffer_sizes() {
    AudioBufferDesc desc;
    desc.sample_rate = sample_rate_;
    desc.channels = output_channels_;
    desc.format = AudioFormat::PCM_F32;
    desc.frames = 1024; // Standard buffer size
    
    mix_buffer_.allocate(desc);
    temp_mix_buffer_.resize(1024 * output_channels_);
}

void AudioMixer::reset_all_channels() {
    for (auto& channel : channels_) {
        if (channel) {
            channel->reset();
        }
    }
}

// ============================================================================
// AUDIO PROCESSOR IMPLEMENTATION
// ============================================================================

AudioProcessor::AudioProcessor()
    : master_mixer_(64, 44100) {
}

AudioProcessor::~AudioProcessor() {
    shutdown();
}

bool AudioProcessor::initialize(const AudioBufferDesc& output_desc) {
    output_format_ = output_desc;
    output_format_.format = AudioFormat::PCM_F32; // Always work with float internally
    
    if (!output_buffer_.allocate(output_format_)) {
        return false;
    }
    
    // Configure mixer
    master_mixer_.set_sample_rate(output_format_.sample_rate);
    master_mixer_.set_output_channels(output_format_.channels);
    
    initialized_ = true;
    return true;
}

void AudioProcessor::shutdown() {
    initialized_ = false;
    output_buffer_.deallocate();
    reset_levels();
}

bool AudioProcessor::set_output_format(const AudioBufferDesc& desc) {
    if (!initialized_) {
        output_format_ = desc;
        output_format_.format = AudioFormat::PCM_F32;
        return output_buffer_.allocate(output_format_);
    }
    
    // Reinitialize with new format
    shutdown();
    return initialize(desc);
}

bool AudioProcessor::process_buffers(const std::vector<AudioBuffer>& input_buffers,
                                    AudioBuffer& output_buffer, float time) {
    if (!initialized_) {
        return false;
    }
    
    // Process through mixer
    if (!master_mixer_.process_buffers(input_buffers, output_buffer_, time)) {
        return false;
    }
    
    // Update audio levels
    update_levels(output_buffer_);
    
    // Copy to output buffer if different
    if (&output_buffer_ != &output_buffer) {
        if (!output_buffer.is_allocated()) {
            output_buffer.allocate(output_format_);
        }
        output_buffer.copy_from(output_buffer_);
    }
    
    current_time_ = time;
    return true;
}

void AudioProcessor::update_levels(AudioBuffer& buffer) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32 ||
        buffer.get_channel_count() < 2) {
        return;
    }
    
    const float* samples = static_cast<const float*>(buffer.get_data());
    uint32_t frame_count = buffer.get_frame_count();
    
    float left_peak = 0.0f;
    float right_peak = 0.0f;
    float left_sum = 0.0f;
    float right_sum = 0.0f;
    
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        uint32_t sample_idx = frame * buffer.get_channel_count();
        float left_sample = std::abs(samples[sample_idx]);
        float right_sample = std::abs(samples[sample_idx + 1]);
        
        left_peak = std::max(left_peak, left_sample);
        right_peak = std::max(right_peak, right_sample);
        
        left_sum += left_sample * left_sample;
        right_sum += right_sample * right_sample;
    }
    
    current_levels_.left_peak = left_peak;
    current_levels_.right_peak = right_peak;
    current_levels_.left_rms = std::sqrt(left_sum / frame_count);
    current_levels_.right_rms = std::sqrt(right_sum / frame_count);
    current_levels_.clipping = (left_peak > 1.0f || right_peak > 1.0f);
    
    // Store in history
    level_history_.push_back(left_peak + right_peak);
    if (level_history_.size() > level_history_size_) {
        level_history_.erase(level_history_.begin());
    }
}

void AudioProcessor::reset_levels() {
    current_levels_ = {};
    level_history_.clear();
}

void AudioProcessor::set_quality(AudioQuality quality) {
    quality_ = quality;
    
    uint32_t sample_rate = SAMPLE_RATES[static_cast<int>(quality)];
    uint8_t bit_depth = BIT_DEPTHS[static_cast<int>(quality)];
    
    AudioBufferDesc new_desc = output_format_;
    new_desc.sample_rate = sample_rate;
    
    set_output_format(new_desc);
    master_mixer_.set_sample_rate(sample_rate);
}

// ============================================================================
// COMPRESSOR EFFECT IMPLEMENTATION
// ============================================================================

CompressorEffect::CompressorEffect() : AudioEffect("Compressor") {
    type_ = "compressor";
    sample_rate_ = 44100;
    update_coefficients(sample_rate_);
}

bool CompressorEffect::process(AudioBuffer& buffer, float time) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32) {
        return false;
    }
    
    float* samples = static_cast<float*>(buffer.get_data());
    uint32_t frame_count = buffer.get_frame_count();
    uint32_t channels = buffer.get_channel_count();
    
    // Process each channel
    for (uint32_t ch = 0; ch < channels; ++ch) {
        float& envelope = envelope_.size() > ch ? envelope_[ch] : 0.0f;
        
        for (uint32_t frame = 0; frame < frame_count; ++frame) {
            uint32_t sample_idx = frame * channels + ch;
            float input_sample = samples[sample_idx];
            
            // Apply compression
            float output_sample = apply_compression(input_sample, envelope);
            samples[sample_idx] = output_sample;
        }
    }
    
    return true;
}

bool CompressorEffect::set_parameter(const std::string& name, float value) {
    if (name == "threshold") {
        threshold_ = value;
        return true;
    } else if (name == "ratio") {
        ratio_ = value;
        return true;
    } else if (name == "attack") {
        attack_time_ = value;
        update_coefficients(sample_rate_);
        return true;
    } else if (name == "release") {
        release_time_ = value;
        update_coefficients(sample_rate_);
        return true;
    } else if (name == "makeup_gain") {
        makeup_gain_ = value;
        return true;
    } else if (name == "knee_width") {
        knee_width_ = value;
        return true;
    }
    
    return false;
}

float CompressorEffect::get_parameter(const std::string& name) const {
    if (name == "threshold") return threshold_;
    if (name == "ratio") return ratio_;
    if (name == "attack") return attack_time_;
    if (name == "release") return release_time_;
    if (name == "makeup_gain") return makeup_gain_;
    if (name == "knee_width") return knee_width_;
    
    return 0.0f;
}

std::vector<std::string> CompressorEffect::get_parameter_names() const {
    return {"threshold", "ratio", "attack", "release", "makeup_gain", "knee_width"};
}

void CompressorEffect::reset() {
    std::fill(envelope_.begin(), envelope_.end(), 0.0f);
}

void CompressorEffect::update_coefficients(uint32_t sample_rate) {
    sample_rate_ = sample_rate;
    
    // Convert time constants to filter coefficients
    attack_coeff_ = std::exp(-1.0f / (attack_time_ * sample_rate));
    release_coeff_ = std::exp(-1.0f / (release_time_ * sample_rate));
    
    // Ensure envelope vector has enough elements for channels
    envelope_.resize(8, 0.0f); // Support up to 8 channels
}

float CompressorEffect::apply_compression(float input_sample, float& envelope) {
    // Convert to dB
    float input_db = 20.0f * std::log10(std::abs(input_sample) + 1e-10f);
    
    // Update envelope follower
    float target = std::max(0.0f, input_db - threshold_);
    if (target > envelope) {
        envelope = attack_coeff_ * envelope + (1.0f - attack_coeff_) * target;
    } else {
        envelope = release_coeff_ * envelope + (1.0f - release_coeff_) * target;
    }
    
    // Calculate gain reduction
    float over_threshold = envelope;
    float gain_reduction = 0.0f;
    
    if (over_threshold > 0.0f) {
        // Soft knee implementation
        float knee_half = knee_width_ / 2.0f;
        if (over_threshold <= knee_half) {
            // Soft knee region
            float ratio_blend = over_threshold / knee_half;
            gain_reduction = over_threshold * ratio_blend * (1.0f - 1.0f/ratio_) / 2.0f;
        } else {
            // Full compression
            gain_reduction = knee_half * (1.0f - 1.0f/ratio_) / 2.0f 
                          + (over_threshold - knee_half) * (1.0f - 1.0f/ratio_);
        }
    }
    
    // Apply gain reduction and makeup gain
    float gain_linear = std::pow(10.0f, (makeup_gain_ - gain_reduction) / 20.0f);
    
    return input_sample * gain_linear;
}

// ============================================================================
// REVERB EFFECT IMPLEMENTATION
// ============================================================================

ReverbEffect::ReverbEffect(ReverbType type) : AudioEffect("Reverb") {
    type_ = "reverb";
    reverb_type_ = type;
    
    // Initialize algorithmic reverb
    initialize_algorithmic_reverb(44100, 2);
}

bool ReverbEffect::process(AudioBuffer& buffer, float time) {
    if (buffer.get_desc().format != AudioFormat::PCM_F32) {
        return false;
    }
    
    switch (reverb_type_) {
        case ReverbType::HALL:
        case ReverbType::ROOM:
        case ReverbType::PLATE:
        case ReverbType::SPRING:
            process_algorithmic_reverb(buffer);
            break;
        case ReverbType::CONVOLUTION:
            if (impulse_response_.is_allocated()) {
                process_convolution_reverb(buffer);
            }
            break;
    }
    
    return true;
}

bool ReverbEffect::set_parameter(const std::string& name, float value) {
    if (name == "room_size") {
        room_size_ = value;
        return true;
    } else if (name == "damping") {
        damping_ = value;
        return true;
    } else if (name == "wet_level") {
        wet_level_ = value;
        return true;
    } else if (name == "dry_level") {
        dry_level_ = value;
        return true;
    } else if (name == "width") {
        width_ = value;
        return true;
    }
    
    return false;
}

float ReverbEffect::get_parameter(const std::string& name) const {
    if (name == "room_size") return room_size_;
    if (name == "damping") return damping_;
    if (name == "wet_level") return wet_level_;
    if (name == "dry_level") return dry_level_;
    if (name == "width") return width_;
    
    return 0.0f;
}

std::vector<std::string> ReverbEffect::get_parameter_names() const {
    return {"room_size", "damping", "wet_level", "dry_level", "width"};
}

void ReverbEffect::reset() {
    // Clear all delay lines and filter states
    for (auto& delay_line : delay_lines_) {
        std::fill(delay_line.begin(), delay_line.end(), 0.0f);
    }
    
    for (auto& delay_index : delay_indices_) {
        delay_index = 0;
    }
    
    for (auto& filter_state : filter_states_) {
        std::fill(filter_state.begin(), filter_state.end(), 0.0f);
    }
}

bool ReverbEffect::load_impulse_response(const std::string& file_path) {
    // Placeholder - would load from file
    return false;
}

void ReverbEffect::set_impulse_response(const AudioBuffer& ir_buffer) {
    // Create a new buffer with the same format
    AudioBufferDesc desc = ir_buffer.get_desc();
    if (!impulse_response_.allocate(desc)) {
        return;
    }
    
    // Copy the data
    impulse_response_.copy_from(ir_buffer);
    reverb_type_ = ReverbType::CONVOLUTION;
    
    // Initialize convolution buffer
    uint32_t ir_length = ir_buffer.get_frame_count();
    uint32_t channels = ir_buffer.get_channel_count();
    
    convolution_buffer_.resize(channels);
    for (uint32_t ch = 0; ch < channels; ++ch) {
        convolution_buffer_[ch].resize(ir_length, 0.0f);
    }
}

void ReverbEffect::initialize_algorithmic_reverb(uint32_t sample_rate, uint32_t channels) {
    sample_rate_ = sample_rate;
    
    // Initialize delay lines for Schroeder reverb
    delay_lines_.resize(channels);
    delay_indices_.resize(channels);
    delay_lengths_.resize(channels);
    filter_states_.resize(channels);
    
    // Delay lengths in samples (optimized for good diffusion)
    std::vector<uint32_t> base_delays = {1547, 1557, 1617, 1491, 1423, 1277, 1353, 1187};
    
    for (uint32_t ch = 0; ch < channels; ++ch) {
        uint32_t delay_length = base_delays[ch % base_delays.size()];
        delay_lines_[ch].resize(delay_length, 0.0f);
        delay_indices_[ch] = 0;
        delay_lengths_[ch] = delay_length;
        filter_states_[ch].resize(2, 0.0f); // Simple low-pass filter
    }
}

void ReverbEffect::process_algorithmic_reverb(AudioBuffer& buffer) {
    float* samples = static_cast<float*>(buffer.get_data());
    uint32_t frame_count = buffer.get_frame_count();
    uint32_t channels = buffer.get_channel_count();
    
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        std::vector<float> wet_samples(channels, 0.0f);
        
        // Process each channel's reverb
        for (uint32_t ch = 0; ch < channels; ++ch) {
            uint32_t sample_idx = frame * channels + ch;
            float input = samples[sample_idx];
            
            // Simple delay-based reverb
            auto& delay_line = delay_lines_[ch];
            auto& delay_idx = delay_indices_[ch];
            auto& delay_len = delay_lengths_[ch];
            auto& filter_state = filter_states_[ch];
            
            // Read from delay line
            float delayed = delay_line[delay_idx];
            
            // Apply damping filter
            auto& filter_state_vec = filter_states_[ch];
            float filtered = delayed * (1.0f - damping_) + filter_state_vec[0] * damping_;
            filter_state_vec[0] = filtered;
            
            // Mix wet signal
            wet_samples[ch] = filtered * wet_level_;
            
            // Write to delay line with feedback
            delay_line[delay_idx] = input + filtered * room_size_ * 0.8f;
            
            // Update delay index
            delay_idx = (delay_idx + 1) % delay_len;
        }
        
        // Apply width (stereo spread)
        if (channels >= 2) {
            float left = wet_samples[0];
            float right = wet_samples[1];
            float mid = (left + right) * 0.5f;
            float side = (left - right) * width_ * 0.5f;
            
            wet_samples[0] = mid + side;
            wet_samples[1] = mid - side;
        }
        
        // Mix wet and dry signals
        for (uint32_t ch = 0; ch < channels; ++ch) {
            uint32_t sample_idx = frame * channels + ch;
            samples[sample_idx] = samples[sample_idx] * dry_level_ + wet_samples[ch];
        }
    }
}

void ReverbEffect::process_convolution_reverb(AudioBuffer& buffer) {
    // Placeholder - would implement fast convolution using FFT
    // For now, just pass through
}

// ============================================================================
// AUDIO EFFECT FACTORY IMPLEMENTATION
// ============================================================================

std::unique_ptr<AudioEffect> AudioEffectFactory::create_volume_effect() {
    return std::make_unique<VolumeEffect>();
}

std::unique_ptr<AudioEffect> AudioEffectFactory::create_equalizer_effect(uint32_t bands) {
    return std::make_unique<EqualizerEffect>(bands);
}

std::unique_ptr<AudioEffect> AudioEffectFactory::create_compressor_effect() {
    return std::make_unique<CompressorEffect>();
}

std::unique_ptr<AudioEffect> AudioEffectFactory::create_reverb_effect(ReverbEffect::ReverbType type) {
    return std::make_unique<ReverbEffect>(type);
}

std::vector<std::string> AudioEffectFactory::get_available_effects() {
    return {"volume", "equalizer", "compressor", "reverb"};
}

std::unique_ptr<AudioEffect> AudioEffectFactory::create_effect(const std::string& type) {
    if (type == "volume") {
        return create_volume_effect();
    } else if (type == "equalizer") {
        return create_equalizer_effect();
    } else if (type == "compressor") {
        return create_compressor_effect();
    } else if (type == "reverb") {
        return create_reverb_effect(ReverbEffect::ReverbType::HALL);
    }
    
    return nullptr;
}

std::vector<AudioEffectFactory::EffectInfo> AudioEffectFactory::get_effect_info() {
    std::vector<EffectInfo> info;
    
    info.push_back({"Volume", "volume", "Basic volume/gain control", {"gain", "muted"}});
    info.push_back({"Equalizer", "equalizer", "Multi-band frequency equalizer", {"frequency", "gain", "q"}});
    info.push_back({"Compressor", "compressor", "Dynamic range compression", {"threshold", "ratio", "attack", "release"}});
    info.push_back({"Reverb", "reverb", "Reverberation effect", {"room_size", "damping", "wet_level", "dry_level"}});
    
    return info;
}

AudioEffectFactory::EffectInfo AudioEffectFactory::get_effect_info(const std::string& type) {
    auto all_info = get_effect_info();
    auto it = std::find_if(all_info.begin(), all_info.end(),
                           [&type](const EffectInfo& info) {
                               return info.type == type;
                           });
    
    return it != all_info.end() ? *it : EffectInfo{};
}

} // namespace Shotcut::Audio
