#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace spatial_audio {

// Ceiling array order is TF L/R, TB L/R, TM L/R. existingMask uses these
// six local array bits, not the foobar PCM channel mask.
struct SixHeightInput {
    std::array<double, 2> front = {};
    std::array<double, 2> surround = {};
    std::array<double, 2> back = {};
    std::array<double, 6> existing = {};
    unsigned existingMask = 0;
    bool stereo = false;
    bool hasBack = false;
};

struct SixHeightSettings {
    double sideAmount = 0.75;
    double heightFromMid = 0.20;
    double decorrelation = 0.20;
    double heightGain = 1.0;
    bool reference = true;
    bool frontOnly = false;
};

inline std::array<double, 6> synthesize_six_heights(
    const SixHeightInput& input, const SixHeightSettings& settings)
{
    std::array<double, 6> output = {};
    if (!settings.frontOnly) {
        const double sideAmount = settings.reference ? std::min(settings.sideAmount, 0.45) : settings.sideAmount;
        const double fromMid = settings.reference ? std::min(settings.heightFromMid, 0.08) : settings.heightFromMid;
        const double decorrelation = std::clamp(
            settings.reference ? std::min(settings.decorrelation, 0.12) : settings.decorrelation, 0.0, 1.0);
        const double gain = settings.reference ? std::pow(10.0, -10.0 / 20.0) : 1.0;
        const double mid = (input.front[0] + input.front[1]) * 0.5;
        const double side = (input.front[0] - input.front[1]) * 0.5 * sideAmount;
        output[0] = (side * (1.0 - decorrelation * 0.20) + mid * fromMid) * gain;
        output[1] = (-side * (1.0 + decorrelation * 0.20) + mid * fromMid) * gain;

        // Stereo retains the existing four-height upmix. Multichannel rear
        // heights instead derive from the surround/back content on that side.
        const std::array<double, 2> rear = input.hasBack
            ? std::array<double, 2>{(input.surround[0] + input.back[0]) * 0.5,
                                    (input.surround[1] + input.back[1]) * 0.5}
            : input.surround;
        const double rearMid = input.stereo ? mid : (rear[0] + rear[1]) * 0.5;
        const double rearSide = input.stereo ? side : (rear[0] - rear[1]) * 0.5 * sideAmount;
        output[2] = (rearSide * (0.7 + decorrelation * 0.25) + rearMid * fromMid) * gain;
        output[3] = (-rearSide * (0.7 - decorrelation * 0.25) + rearMid * fromMid) * gain;
    }

    // Never synthesize over a supplied channel, including a silent channel.
    for (unsigned i = 0; i < 4; ++i) {
        if ((input.existingMask & (1u << i)) != 0) output[i] = input.existing[i];
    }
    for (unsigned side = 0; side < 2; ++side) {
        const unsigned middle = 4 + side;
        if ((input.existingMask & (1u << middle)) != 0) {
            output[middle] = input.existing[middle];
        } else if (!settings.frontOnly) {
            // Fixed-position middle sources interpolate the adjacent heights.
            // Averaging (rather than summing) avoids increasing their peak.
            output[middle] = (output[side] + output[2 + side]) * 0.5;
        }
    }
    // Apply the generation gain once, including when the middle signal was
    // interpolated from native heights. Supplied channels retain their levels.
    for (unsigned i = 0; i < output.size(); ++i) {
        if ((input.existingMask & (1u << i)) == 0) output[i] *= settings.heightGain;
    }
    return output;
}

} // namespace spatial_audio
