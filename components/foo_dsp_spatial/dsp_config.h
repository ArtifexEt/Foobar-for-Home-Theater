#pragma once
#include "stdafx.h"

namespace spatial_audio {

enum class UpmixMode   { Full = 0, Reference = 1, FrontOnly = 2 };
enum class LimiterMode { TransparentSoft = 0, HardCeiling = 1 };

static constexpr size_t target_count = 12;

enum ChannelTarget {
    target_disabled = -1,
    target_front_left = 0, target_front_right = 1, target_front_center = 2,
    target_low_frequency = 3, target_side_left = 4, target_side_right = 5,
    target_back_left = 6, target_back_right = 7,
    target_top_front_left = 8, target_top_front_right = 9,
    target_top_back_left = 10, target_top_back_right = 11,
};

struct DspConfig {
    UpmixMode upmixMode = UpmixMode::Reference;
    double masterGainDb = -12.0;
    double headroomDb = 0.0;
    bool limiterEnabled = true;
    LimiterMode limiterMode = LimiterMode::TransparentSoft;
    double limiterCeilingDb = -1.0;
    double centerGainDb = -6.0;
    double surroundGainDb = -9.0;
    double rearGainDb = -12.0;
    double heightGainDb = -12.0;
    double sideAmount = 0.75;
    double heightFromMid = 0.20;
    double decorrelationAmount = 0.20;
    bool enableLfe = false;
    double lfeGainDb = -24.0;
    double lfeLowpassHz = 120.0;
    std::array<double, target_count> channelGainDb = {};
    std::array<double, target_count> channelDelayMs = {};
    std::array<bool, target_count> channelInvert = {};
    int map51FrontLeft   = target_front_left;
    int map51FrontRight  = target_front_right;
    int map51FrontCenter = target_front_center;
    int map51Lfe         = target_low_frequency;
    int map51SurroundLeft  = target_side_left;
    int map51SurroundRight = target_side_right;
};

DspConfig ReadDspConfig();
void WriteDspConfig(const DspConfig& config);
DspConfig DefaultDspConfig();
std::string SerializeDspConfig(const DspConfig& config);
bool DeserializeDspConfig(const std::string& text, DspConfig& config);

static const char* const kTargetKeys[12] = {
    "front_left","front_right","front_center","low_frequency",
    "side_left","side_right","back_left","back_right",
    "top_front_left","top_front_right","top_back_left","top_back_right"
};

static constexpr std::array<int, target_count> kOutputChannelTargets = {
    target_front_left,
    target_front_right,
    target_front_center,
    target_low_frequency,
    target_back_left,
    target_back_right,
    target_side_left,
    target_side_right,
    target_top_front_left,
    target_top_front_right,
    target_top_back_left,
    target_top_back_right,
};

static constexpr unsigned kOutputChannelMask =
    audio_chunk::channel_front_left |
    audio_chunk::channel_front_right |
    audio_chunk::channel_front_center |
    audio_chunk::channel_lfe |
    audio_chunk::channel_back_left |
    audio_chunk::channel_back_right |
    audio_chunk::channel_side_left |
    audio_chunk::channel_side_right |
    audio_chunk::channel_top_front_left |
    audio_chunk::channel_top_front_right |
    audio_chunk::channel_top_back_left |
    audio_chunk::channel_top_back_right;

static_assert(audio_chunk::g_count_channels(kOutputChannelMask) == target_count, "Spatial DSP output mask must match target count.");

} // namespace spatial_audio
