#pragma once

#include "stdafx.h"

namespace spatial_audio {

enum ChannelTarget {
    target_disabled = -1,
    target_front_left = 0,
    target_front_right = 1,
    target_front_center = 2,
    target_low_frequency = 3,
    target_side_left = 4,
    target_side_right = 5,
    target_back_left = 6,
    target_back_right = 7,
    target_top_front_left = 8,
    target_top_front_right = 9,
    target_top_back_left = 10,
    target_top_back_right = 11,
};

static constexpr size_t target_count = 12;

enum class LayoutMode {
    Auto = 0,
    Stereo = 1,
    FivePointOne = 2,
    SevenPointOne = 3,
    FivePointOneTwo = 4,
    SevenPointOneFour = 5,
};

enum class SampleRateMode {
    AutoHighest = 0,
    SourceIfSupported = 1,
    Fixed44100 = 2,
    Fixed48000 = 3,
    Fixed88200 = 4,
    Fixed96000 = 5,
    Fixed176400 = 6,
    Fixed192000 = 7,
};

enum class LimiterMode {
    TransparentSoft = 0,
    HardCeiling = 1,
};

struct RuntimeConfig {
    LayoutMode layoutMode = LayoutMode::Auto;
    SampleRateMode sampleRateMode = SampleRateMode::AutoHighest;
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
    int map51FrontLeft = target_front_left;
    int map51FrontRight = target_front_right;
    int map51FrontCenter = target_front_center;
    int map51Lfe = target_low_frequency;
    int map51SurroundLeft = target_side_left;
    int map51SurroundRight = target_side_right;
    bool directionalTestEnabled = false;
    bool directionalTestUseDynamicObject = true;
    int directionalTestTarget = target_front_center;
    double directionalTestGainDb = -18.0;
    double directionalTestFrequencyHz = 660.0;
};

RuntimeConfig ReadConfig();
void WriteConfig(const RuntimeConfig& config);
RuntimeConfig DefaultConfig();

}  // namespace spatial_audio
