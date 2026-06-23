#include "stdafx.h"
#include "spatial_dsp.h"

namespace spatial_audio {
namespace {

constexpr double kPi = 3.14159265358979323846;

static constexpr GUID guid_dsp = { 0xAABBCCDD, 0x1122, 0x3344, { 0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC } };

} // namespace

GUID spatial_upmix_dsp::g_get_guid() {
    return guid_dsp;
}

void spatial_upmix_dsp::g_get_name(pfc::string_base& out) {
    out = "Spatial Audio Upmix";
}

void spatial_upmix_dsp::reset_state() {
    lfeState_ = 0.0;
    for (auto& dl : delays_) {
        dl.buffer.clear();
        dl.index = 0;
    }
}

double spatial_upmix_dsp::get_latency() {
    const DspConfig config = ReadDspConfig();
    const auto maxDelay = std::max_element(config.channelDelayMs.begin(), config.channelDelayMs.end());
    const double delayMs = maxDelay != config.channelDelayMs.end() ? *maxDelay : 0.0;
    return std::clamp(delayMs, 0.0, 80.0) / 1000.0;
}

bool spatial_upmix_dsp::need_track_change_mark() {
    return false;
}

bool spatial_upmix_dsp::on_chunk(audio_chunk* chunk, abort_callback&) {
    config_ = ReadDspConfig();
    sampleRate_ = chunk->get_sample_rate();
    const unsigned channels  = chunk->get_channel_count();
    const unsigned mask      = chunk->get_channel_config();
    const size_t frameCount  = chunk->get_sample_count();
    const audio_sample* in   = chunk->get_data();

    if (in == nullptr || channels < 2 || frameCount == 0) {
        return true;
    }

    inputLayout_ = InputLayout::Stereo;
    if (channels == 6 && (mask == 0 || is_5point1_mask(mask))) {
        inputLayout_ = InputLayout::FivePointOne;
    } else if (channels == 8 && (mask == 0 || is_7point1_mask(mask))) {
        inputLayout_ = InputLayout::SevenPointOne;
    }

    const double masterGain = db_to_linear(config_.masterGainDb + config_.headroomDb);
    std::vector<audio_sample> out(frameCount * 12, 0.0f);

    for (size_t i = 0; i < frameCount; ++i) {
        InputFrame frame = extract_frame(in, i, channels, mask, inputLayout_);
        for (int ch = 0; ch < 12; ++ch) {
            double value = bed_value(ch, frame);
            value *= masterGain * db_to_linear(config_.channelGainDb[static_cast<size_t>(ch)]);
            if (config_.channelInvert[static_cast<size_t>(ch)]) value = -value;
            float delayed = apply_channel_delay(ch, static_cast<float>(value));
            delayed = static_cast<float>(apply_limiter(delayed));
            out[i * 12 + ch] = delayed;
        }
    }

    chunk->set_data(out.data(), frameCount, 12, sampleRate_, 0);
    return true;
}

spatial_upmix_dsp::InputFrame spatial_upmix_dsp::extract_frame(
    const audio_sample* samples, size_t frameIdx,
    unsigned channels, unsigned mask, InputLayout layout)
{
    InputFrame frame;
    if (layout == InputLayout::FivePointOne || layout == InputLayout::SevenPointOne) {
        frame.frontLeft   = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_front_left,   0);
        frame.frontRight  = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_front_right,  1);
        frame.frontCenter = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_front_center, 2);
        frame.lfe         = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_lfe,          3);
        if (layout == InputLayout::SevenPointOne) {
            frame.backLeft    = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_back_left,  4);
            frame.backRight   = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_back_right, 5);
            frame.surroundLeft  = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_side_left,  6);
            frame.surroundRight = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_side_right, 7);
        } else {
            const bool usesSide = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
            frame.surroundLeft  = sample_by_flag(samples, frameIdx, channels, mask,
                usesSide ? audio_chunk::channel_side_left  : audio_chunk::channel_back_left,  4);
            frame.surroundRight = sample_by_flag(samples, frameIdx, channels, mask,
                usesSide ? audio_chunk::channel_side_right : audio_chunk::channel_back_right, 5);
        }
    } else {
        frame.frontLeft  = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_front_left,  0);
        frame.frontRight = sample_by_flag(samples, frameIdx, channels, mask, audio_chunk::channel_front_right, 1);
    }
    return frame;
}

double spatial_upmix_dsp::bed_value(int outputChannel, const InputFrame& frame) {
    if (inputLayout_ == InputLayout::SevenPointOne) return mapped_7point1_value(outputChannel, frame);
    if (inputLayout_ == InputLayout::FivePointOne)  return mapped_5point1_value(outputChannel, frame);
    return stereo_bed_value(outputChannel, frame);
}

double spatial_upmix_dsp::stereo_bed_value(int outputChannel, const InputFrame& frame) {
    const double left  = frame.frontLeft;
    const double right = frame.frontRight;
    const double mid   = (left + right) * 0.5;
    const char* key    = kTargetKeys[outputChannel];

    if (strcmp(key, "front_left")  == 0) return left;
    if (strcmp(key, "front_right") == 0) return right;
    if (config_.upmixMode == UpmixMode::FrontOnly) return 0.0;

    const bool referenceMode    = config_.upmixMode == UpmixMode::Reference;
    const double sideAmount     = referenceMode ? std::min(config_.sideAmount, 0.45) : config_.sideAmount;
    const double heightFromMid  = referenceMode ? std::min(config_.heightFromMid, 0.08) : config_.heightFromMid;
    const double centerTrim     = referenceMode ? db_to_linear(-4.0) : 1.0;
    const double surroundTrim   = referenceMode ? db_to_linear(-6.0) : 1.0;
    const double rearTrim       = referenceMode ? db_to_linear(-8.0) : 1.0;
    const double heightTrim     = referenceMode ? db_to_linear(-10.0) : 1.0;
    const double lfeTrim        = referenceMode ? db_to_linear(-6.0) : 1.0;
    const double side           = (left - right) * 0.5 * sideAmount;
    const double decorrelation  = std::clamp(
        referenceMode ? std::min(config_.decorrelationAmount, 0.12) : config_.decorrelationAmount,
        0.0, 1.0);

    if (strcmp(key, "front_center")    == 0) return mid * db_to_linear(config_.centerGainDb) * centerTrim;
    if (strcmp(key, "side_left")       == 0) return  side * db_to_linear(config_.surroundGainDb) * surroundTrim;
    if (strcmp(key, "side_right")      == 0) return -side * db_to_linear(config_.surroundGainDb) * surroundTrim;
    if (strcmp(key, "back_left")       == 0) return (side * (1.0 + decorrelation * 0.35) + left  * 0.10) * db_to_linear(config_.rearGainDb) * rearTrim;
    if (strcmp(key, "back_right")      == 0) return (-side * (1.0 - decorrelation * 0.35) + right * 0.10) * db_to_linear(config_.rearGainDb) * rearTrim;
    if (strcmp(key, "top_front_left")  == 0) return (side * (1.0 - decorrelation * 0.20) + mid * heightFromMid) * db_to_linear(config_.heightGainDb) * heightTrim;
    if (strcmp(key, "top_front_right") == 0) return (-side * (1.0 + decorrelation * 0.20) + mid * heightFromMid) * db_to_linear(config_.heightGainDb) * heightTrim;
    if (strcmp(key, "top_back_left")   == 0) return (side * (0.7 + decorrelation * 0.25) + mid * heightFromMid) * db_to_linear(config_.heightGainDb) * heightTrim;
    if (strcmp(key, "top_back_right")  == 0) return (-side * (0.7 - decorrelation * 0.25) + mid * heightFromMid) * db_to_linear(config_.heightGainDb) * heightTrim;
    if (strcmp(key, "low_frequency") == 0) {
        if (!config_.enableLfe) return 0.0;
        const double cutoffHz = std::clamp(config_.lfeLowpassHz, 20.0, 250.0);
        const double alpha = 1.0 - std::exp((-2.0 * kPi * cutoffHz) / static_cast<double>(sampleRate_));
        lfeState_ += alpha * (mid - lfeState_);
        return lfeState_ * db_to_linear(config_.lfeGainDb) * lfeTrim;
    }
    return 0.0;
}

double spatial_upmix_dsp::mapped_5point1_value(int outputChannel, const InputFrame& frame) const {
    double value = 0.0;
    if (config_.map51FrontLeft    == outputChannel) value += frame.frontLeft;
    if (config_.map51FrontRight   == outputChannel) value += frame.frontRight;
    if (config_.map51FrontCenter  == outputChannel) value += frame.frontCenter;
    if (config_.map51Lfe          == outputChannel) value += frame.lfe;
    if (config_.map51SurroundLeft == outputChannel) value += frame.surroundLeft;
    if (config_.map51SurroundRight== outputChannel) value += frame.surroundRight;
    return value;
}

double spatial_upmix_dsp::mapped_7point1_value(int outputChannel, const InputFrame& frame) const {
    switch (outputChannel) {
    case 0: return frame.frontLeft;
    case 1: return frame.frontRight;
    case 2: return frame.frontCenter;
    case 3: return frame.lfe;
    case 4: return frame.surroundLeft;
    case 5: return frame.surroundRight;
    case 6: return frame.backLeft;
    case 7: return frame.backRight;
    default: return 0.0;
    }
}

double spatial_upmix_dsp::apply_limiter(double value) const {
    if (!config_.limiterEnabled) return value;
    const double ceiling = db_to_linear(std::clamp(config_.limiterCeilingDb, -24.0, 0.0));
    if (config_.limiterMode == LimiterMode::HardCeiling) {
        return std::clamp(value, -ceiling, ceiling);
    }
    const double magnitude = std::fabs(value);
    const double kneeStart = ceiling * 0.85;
    if (magnitude <= kneeStart) return value;
    const double range  = std::max(ceiling - kneeStart, 0.000001);
    const double over   = magnitude - kneeStart;
    const double limited = kneeStart + range * std::tanh(over / range);
    return std::copysign(std::min(limited, ceiling), value);
}

float spatial_upmix_dsp::apply_channel_delay(int channelIdx, float value) {
    const double delayMs = std::clamp(config_.channelDelayMs[static_cast<size_t>(channelIdx)], 0.0, 80.0);
    const size_t delaySamples = static_cast<size_t>(std::round((delayMs / 1000.0) * sampleRate_));
    if (delaySamples == 0) {
        delays_[channelIdx].buffer.clear();
        delays_[channelIdx].index = 0;
        return value;
    }
    const size_t requiredSize = delaySamples + 1;
    auto& dl = delays_[channelIdx];
    if (dl.buffer.size() != requiredSize) {
        dl.buffer.assign(requiredSize, 0.0f);
        dl.index = 0;
    }
    const float delayed = dl.buffer[dl.index];
    dl.buffer[dl.index] = value;
    dl.index = (dl.index + 1) % dl.buffer.size();
    return delayed;
}

double spatial_upmix_dsp::db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

bool spatial_upmix_dsp::is_5point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront  = (mask & front) == front;
    const bool hasBack   = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide   = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && (hasBack || hasSide) && audio_chunk::g_count_channels(mask) == 6;
}

bool spatial_upmix_dsp::is_7point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront  = (mask & front) == front;
    const bool hasBack   = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide   = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && hasBack && hasSide && audio_chunk::g_count_channels(mask) == 8;
}

float spatial_upmix_dsp::sample_by_flag(const audio_sample* samples, size_t frame, unsigned channels, unsigned mask, unsigned flag, unsigned fallbackIndex) {
    unsigned index = audio_chunk::g_channel_index_from_flag(mask, flag);
    if (index == static_cast<unsigned>(-1) || index >= channels) index = fallbackIndex;
    return index < channels ? static_cast<float>(samples[(frame * channels) + index]) : 0.0f;
}

static dsp_factory_nopreset_t<spatial_upmix_dsp> g_dsp_factory;

} // namespace spatial_audio
