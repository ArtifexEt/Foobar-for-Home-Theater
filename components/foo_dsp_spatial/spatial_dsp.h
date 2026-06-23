#pragma once
#include "stdafx.h"
#include "dsp_config.h"

namespace spatial_audio {

class spatial_upmix_dsp : public dsp_impl {
public:
    static GUID g_get_guid();
    static void g_get_name(pfc::string_base& out);

    bool on_chunk(audio_chunk* chunk, abort_callback&) override;
    void on_end_of_track(abort_callback&) override { reset_state(); }
    void on_flush() override { reset_state(); }

private:
    enum class InputLayout { Stereo, FivePointOne, SevenPointOne };

    struct InputFrame {
        float frontLeft   = 0.0f;
        float frontRight  = 0.0f;
        float frontCenter = 0.0f;
        float lfe         = 0.0f;
        float surroundLeft  = 0.0f;
        float surroundRight = 0.0f;
        float backLeft  = 0.0f;
        float backRight = 0.0f;
    };

    struct DelayLine {
        std::vector<float> buffer;
        size_t index = 0;
    };

    DspConfig config_;
    double lfeState_       = 0.0;
    uint32_t sampleRate_   = 48000;
    InputLayout inputLayout_ = InputLayout::Stereo;
    std::array<DelayLine, 12> delays_;

    void reset_state();
    InputFrame extract_frame(const audio_sample* samples, size_t frameIdx,
                             unsigned channels, unsigned mask, InputLayout layout);
    double bed_value(int outputChannel, const InputFrame& frame);
    double stereo_bed_value(int outputChannel, const InputFrame& frame);
    double mapped_5point1_value(int outputChannel, const InputFrame& frame) const;
    double mapped_7point1_value(int outputChannel, const InputFrame& frame) const;
    double apply_limiter(double value) const;
    float apply_channel_delay(int channelIdx, float value);
    static double db_to_linear(double db);
    static bool is_5point1_mask(unsigned mask);
    static bool is_7point1_mask(unsigned mask);
    static float sample_by_flag(const audio_sample* samples, size_t frame,
                                unsigned channels, unsigned mask,
                                unsigned flag, unsigned fallbackIndex);
};

} // namespace spatial_audio
