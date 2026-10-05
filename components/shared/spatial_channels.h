#pragma once

#include <cstdint>

// Private PCM transport between the matching Spatial DSP, Height DSP and
// Spatial Audio Output components. The foobar2000 SDK defines speaker bits 0-17
// only. These extension bits are NOT standard WAVEFORMATEXTENSIBLE speakers.
// Keep them distinct from front-wide FCL/FCR; never infer them from channel count.
namespace spatial_channels {
inline constexpr std::uint32_t top_middle_left = 1u << 18;
inline constexpr std::uint32_t top_middle_right = 1u << 19;
inline constexpr std::uint32_t top_middle_pair = top_middle_left | top_middle_right;

constexpr unsigned count(std::uint32_t mask) {
    unsigned result = 0;
    while (mask != 0) { result += mask & 1u; mask >>= 1; }
    return result;
}

constexpr unsigned index(std::uint32_t mask, std::uint32_t flag) {
    return flag != 0 && (flag & (flag - 1u)) == 0 && (mask & flag) != 0
        ? count(mask & (flag - 1u)) : static_cast<unsigned>(-1);
}

constexpr std::uint32_t flag_at(std::uint32_t mask, unsigned index) {
    for (unsigned bit = 0; bit < 32; ++bit) {
        const std::uint32_t flag = 1u << bit;
        if ((mask & flag) != 0) {
            if (index == 0) return flag;
            --index;
        }
    }
    return 0;
}
} // namespace spatial_channels
