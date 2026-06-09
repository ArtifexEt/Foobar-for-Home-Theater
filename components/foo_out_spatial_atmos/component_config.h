#pragma once

#include "stdafx.h"

namespace spatial_atmos {

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
};

RuntimeConfig ReadConfig();
void WriteConfig(const RuntimeConfig& config);
RuntimeConfig DefaultConfig();

}  // namespace spatial_atmos
