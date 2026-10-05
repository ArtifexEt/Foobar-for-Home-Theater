#pragma once

#include "../shared/spatial_channels.h"
#include <algorithm>
#include <cmath>

namespace spatial_audio {

// Append values: these IDs are persisted in existing foobar2000 profiles.
enum class LayoutMode {
    Auto = 0, Stereo = 1, FivePointOne = 2, SevenPointOne = 3,
    FivePointOneTwo = 4, FivePointOneFour = 5, SevenPointOneFour = 6,
    NinePointOne = 7, NinePointOneTwo = 8, NinePointOneFour = 9,
    SevenPointOneSix = 10, FivePointOneSix = 11, NinePointOneSix = 12,
};

inline constexpr unsigned front_wide_pair = (1u << 6) | (1u << 7);

constexpr unsigned dynamic_channel_mask(LayoutMode layout, unsigned inputMask) {
    switch (layout) {
    case LayoutMode::Auto:
        return inputMask & (front_wide_pair | spatial_channels::top_middle_pair);
    case LayoutMode::NinePointOne:
    case LayoutMode::NinePointOneTwo:
    case LayoutMode::NinePointOneFour:
        return front_wide_pair;
    case LayoutMode::FivePointOneSix:
    case LayoutMode::SevenPointOneSix:
        return spatial_channels::top_middle_pair;
    case LayoutMode::NinePointOneSix:
        return front_wide_pair | spatial_channels::top_middle_pair;
    default:
        return 0;
    }
}

enum class DynamicInputStatus { Ready, MissingTopMiddle, MissingFrontWide };

constexpr DynamicInputStatus dynamic_input_status(LayoutMode layout, unsigned inputMask) {
    const unsigned required = dynamic_channel_mask(layout, inputMask);
    if ((required & spatial_channels::top_middle_pair) == 0) return DynamicInputStatus::Ready;
    if ((inputMask & spatial_channels::top_middle_pair) != spatial_channels::top_middle_pair)
        return DynamicInputStatus::MissingTopMiddle;
    if ((inputMask & required) != required) return DynamicInputStatus::MissingFrontWide;
    return DynamicInputStatus::Ready;
}

inline double bounded_position(double value, double fallback, double minimum, double maximum) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

struct TopMiddlePosition {
    double halfWidth = 0.8;
    double height = 1.4;
    double frontBack = 0.0;
};

inline TopMiddlePosition sanitize_position(TopMiddlePosition value) {
    value.halfWidth = bounded_position(value.halfWidth, 0.8, 0.1, 10.0);
    value.height = bounded_position(value.height, 1.4, 0.1, 10.0);
    value.frontBack = bounded_position(value.frontBack, 0.0, -10.0, 10.0);
    return value;
}

} // namespace spatial_audio
