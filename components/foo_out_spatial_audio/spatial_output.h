#pragma once

#include "stdafx.h"
#include "component_config.h"

namespace spatial_audio {

class spatial_audio_output : public output_impl {
public:
    spatial_audio_output(const GUID& device, double bufferLength, bool dither, t_uint32 bitDepth);
    ~spatial_audio_output();

    static GUID g_get_guid();
    static const char* g_get_name();
    static void g_enum_devices(output_device_enum_callback& callback);
    static bool g_advanced_settings_query();
    static bool g_needs_bitdepth_config();
    static bool g_needs_dither_config();
    static bool g_needs_device_list_prefixes();
    static bool g_supports_multiple_streams();
    static bool g_is_high_latency();

    unsigned get_forced_sample_rate() override;
    unsigned get_forced_channel_mask() override;

    void pause(bool state) override;
    void volume_set(double value) override;
    bool is_progressing() override;
    pfc::eventHandle_t get_trigger_event() override;

private:
    struct ChannelState {
        std::string key;
        AudioObjectType type = AudioObjectType_None;
        Microsoft::WRL::ComPtr<ISpatialAudioObject> object;
    };

    void on_update() override;
    void write(const audio_chunk& data) override;
    t_size can_write_samples() override;
    t_size get_latency_samples() override;
    void on_flush() override;
    void on_force_play() override;
    void open(audio_chunk::spec_t const& spec) override;

    void start_stream(uint32_t sampleRate);
    void stop_stream();
    void render_loop(uint32_t sampleRate);
    void clear_queue();

    double test_signal_value(double& phase) const;
    double test_frequency_hz() const;
    static bool is_5point1_mask(unsigned mask);
    static bool is_7point1_mask(unsigned mask);
    static uint32_t fixed_sample_rate(SampleRateMode mode);
    static uint32_t highest_supported_sample_rate();
    static bool spatial_sample_rate_supported(uint32_t sampleRate);
    static uint32_t forced_sample_rate(const OutputConfig& config);
    static int target_from_key(const std::string& key);
    static int channel_index(const std::string& key);
    static AudioObjectType requested_static_mask(const OutputConfig& config, AudioObjectType nativeMask);
    static bool target_coordinates(int target, float& x, float& y, float& z);
    static float clamp_sample(double value);
    static double db_to_linear(double db);
    static WAVEFORMATEX make_object_format(uint32_t sampleRate);
    static std::string hresult_text(HRESULT hr);
    static void throw_if_failed(HRESULT hr, const char* action);

    OutputConfig config_;
    GUID device_ = {};
    double bufferLength_ = 1.0;
    uint32_t sampleRate_ = 48000;

    mutable std::mutex mutex_;
    std::deque<std::array<float, 12>> queue_;
    size_t capacityFrames_ = 48000;
    bool stopping_ = false;
    bool paused_   = false;
    bool started_  = false;

    std::atomic<double> volumeDb_        = 0.0;
    std::atomic<size_t> queuedFrames_    = 0;
    std::atomic<size_t> lastRenderFrames_= 0;

    HANDLE wakeEvent_    = nullptr;
    HANDLE spatialEvent_ = nullptr;
    std::thread renderThread_;
};

}  // namespace spatial_audio
