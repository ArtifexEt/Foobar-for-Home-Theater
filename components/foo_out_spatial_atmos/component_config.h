#pragma once

#include "stdafx.h"

namespace spatial_atmos {

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

struct RuntimeConfig {
    double masterGainDb = -12.0;
    double centerGainDb = -6.0;
    double surroundGainDb = -9.0;
    double rearGainDb = -12.0;
    double heightGainDb = -12.0;
    double sideAmount = 0.75;
    double heightFromMid = 0.20;
    bool enableLfe = false;
    double lfeGainDb = -24.0;
    double lfeLowpassHz = 120.0;
    int map51FrontLeft = target_front_left;
    int map51FrontRight = target_front_right;
    int map51FrontCenter = target_front_center;
    int map51Lfe = target_low_frequency;
    int map51SurroundLeft = target_side_left;
    int map51SurroundRight = target_side_right;
};

RuntimeConfig ReadConfig();
void WriteConfig(const RuntimeConfig& config);
RuntimeConfig DefaultConfig();

}  // namespace spatial_atmos
