#pragma once

#include "stdafx.h"
#include "output_routing.h"

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
    target_front_wide_left = 12,
    target_front_wide_right = 13,
    target_top_middle_left = 14,
    target_top_middle_right = 15,
};

static constexpr size_t target_count = 16;

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

struct OutputConfig {
    TopMiddlePosition topMiddlePosition;
    LayoutMode layoutMode = LayoutMode::Auto;
    SampleRateMode sampleRateMode = SampleRateMode::Fixed48000;
    bool directionalTestEnabled = false;
    bool directionalTestUseDynamicObject = true;
    int directionalTestTarget = target_front_center;
    double directionalTestGainDb = -18.0;
    double directionalTestFrequencyHz = 660.0;
};

OutputConfig ReadConfig();
void WriteConfig(const OutputConfig& config);
OutputConfig DefaultConfig();
std::string SerializeConfig(const OutputConfig& config);
bool DeserializeConfig(const std::string& text, OutputConfig& config);

}  // namespace spatial_audio
