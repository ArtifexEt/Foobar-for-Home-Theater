#include "stdafx.h"
#include "dsp_config.h"

namespace spatial_audio {

static constexpr GUID guid_cfg_master_gain         = { 0xe84f6d6e, 0xf21a, 0x4d07, { 0x94, 0x9e, 0xde, 0x37, 0xb1, 0x02, 0x1c, 0x56 } };
static constexpr GUID guid_cfg_output_layout       = { 0xbbd71693, 0x5d73, 0x4f7b, { 0xaa, 0x11, 0x31, 0x4c, 0xa0, 0x58, 0x8c, 0x6e } };
static constexpr GUID guid_cfg_upmix_mode          = { 0x7ee6e772, 0x6f02, 0x49e6, { 0xb5, 0x4a, 0x34, 0xe7, 0x7d, 0xcf, 0x7c, 0xf6 } };
static constexpr GUID guid_cfg_headroom            = { 0x810b1a63, 0xf065, 0x417d, { 0x84, 0x66, 0x07, 0xce, 0x0f, 0xf5, 0x68, 0xf0 } };
static constexpr GUID guid_cfg_limiter_enabled     = { 0x26f390c3, 0xf665, 0x4cd1, { 0x96, 0x72, 0x44, 0xaa, 0xa8, 0xa4, 0xe1, 0x7a } };
static constexpr GUID guid_cfg_limiter_mode        = { 0x6dd2b549, 0x7f65, 0x4aa9, { 0xb7, 0x89, 0x1a, 0x9a, 0xdc, 0x44, 0x30, 0xe9 } };
static constexpr GUID guid_cfg_limiter_ceiling     = { 0x02d2e53a, 0xcdf9, 0x44df, { 0x8c, 0xda, 0xea, 0x8f, 0x6e, 0x06, 0xe9, 0xfb } };
static constexpr GUID guid_cfg_center_gain         = { 0x7e551f08, 0x60ae, 0x46d6, { 0x8b, 0x0f, 0x3b, 0x0d, 0x48, 0x44, 0x3d, 0xfb } };
static constexpr GUID guid_cfg_surround_gain       = { 0x28113b00, 0x8e41, 0x4512, { 0xba, 0x32, 0x1a, 0x1f, 0xfb, 0x12, 0xb4, 0xec } };
static constexpr GUID guid_cfg_rear_gain           = { 0x038dbedb, 0x6775, 0x4021, { 0x81, 0xf8, 0xaa, 0x60, 0x9d, 0x86, 0xaa, 0x49 } };
static constexpr GUID guid_cfg_height_gain         = { 0xa60851df, 0xb4d6, 0x45ba, { 0x86, 0x81, 0xf6, 0x95, 0x12, 0x4c, 0x4b, 0x80 } };
static constexpr GUID guid_cfg_side_amount         = { 0x52f09f72, 0xfcd9, 0x4f87, { 0x9b, 0x00, 0x89, 0x3f, 0xff, 0xa7, 0x29, 0x9a } };
static constexpr GUID guid_cfg_height_from_mid     = { 0x8218113a, 0xf961, 0x4107, { 0xad, 0xaa, 0xe5, 0x71, 0x5d, 0x3e, 0x92, 0x7c } };
static constexpr GUID guid_cfg_decorrelation       = { 0xfe89868e, 0x81e3, 0x4322, { 0x93, 0x04, 0x9c, 0x3a, 0xf3, 0xdc, 0x43, 0x51 } };
static constexpr GUID guid_cfg_enable_lfe          = { 0x50c31342, 0x1910, 0x4040, { 0x99, 0x8f, 0xee, 0x92, 0x19, 0x2a, 0xbe, 0x42 } };
static constexpr GUID guid_cfg_lfe_gain            = { 0x606a82f6, 0xf333, 0x4fb8, { 0xa0, 0x1f, 0xe7, 0x86, 0xb3, 0x8b, 0x24, 0x87 } };
static constexpr GUID guid_cfg_lfe_lowpass         = { 0xcdcc3b34, 0xdfb8, 0x4b55, { 0x94, 0x17, 0xc2, 0x4b, 0xf5, 0xfa, 0x3b, 0x7c } };
static constexpr GUID guid_cfg_channel_gain_front_left       = { 0x9fdf40ea, 0xd51e, 0x49bc, { 0xad, 0x5b, 0xe4, 0x15, 0x48, 0x1c, 0xd6, 0x0c } };
static constexpr GUID guid_cfg_channel_gain_front_right      = { 0x56dc1b1a, 0x685f, 0x461f, { 0xa3, 0x4b, 0x14, 0x6b, 0x01, 0x9f, 0x05, 0x5e } };
static constexpr GUID guid_cfg_channel_gain_front_center     = { 0x80e56c12, 0x15c3, 0x410d, { 0x98, 0xb9, 0xe5, 0x7f, 0xfa, 0x58, 0x6c, 0x22 } };
static constexpr GUID guid_cfg_channel_gain_lfe              = { 0x37fec322, 0xb817, 0x42c6, { 0xb7, 0x29, 0x4c, 0x43, 0x18, 0x66, 0x4c, 0x0c } };
static constexpr GUID guid_cfg_channel_gain_side_left        = { 0xd31c4838, 0x5f8e, 0x4176, { 0x9f, 0x01, 0x9e, 0xbc, 0x88, 0x97, 0x56, 0x45 } };
static constexpr GUID guid_cfg_channel_gain_side_right       = { 0x4b85f376, 0x140e, 0x4d54, { 0xad, 0xaa, 0x75, 0xe8, 0xc7, 0x50, 0xcf, 0x0e } };
static constexpr GUID guid_cfg_channel_gain_back_left        = { 0xea1ca85d, 0x00b9, 0x471f, { 0x8f, 0xc0, 0x39, 0xdf, 0x90, 0x91, 0xf2, 0x56 } };
static constexpr GUID guid_cfg_channel_gain_back_right       = { 0x9d48255b, 0xdd71, 0x46e6, { 0x90, 0x56, 0x38, 0x71, 0x67, 0x8e, 0x8f, 0x06 } };
static constexpr GUID guid_cfg_channel_gain_top_front_left   = { 0xef1937bf, 0xaf33, 0x4d15, { 0x98, 0x3e, 0xe7, 0x11, 0xeb, 0xef, 0xfe, 0xc0 } };
static constexpr GUID guid_cfg_channel_gain_top_front_right  = { 0x607d4b1f, 0xe576, 0x4302, { 0x9b, 0x6d, 0x2a, 0x72, 0x67, 0x76, 0xfe, 0x70 } };
static constexpr GUID guid_cfg_channel_gain_top_back_left    = { 0x5e37467a, 0xe697, 0x40e2, { 0xb0, 0x98, 0x4d, 0x25, 0xdd, 0x48, 0x6b, 0xe1 } };
static constexpr GUID guid_cfg_channel_gain_top_back_right   = { 0x906a45bc, 0x2bd3, 0x43fe, { 0xa2, 0x11, 0x3c, 0xec, 0x48, 0xcc, 0xd2, 0x3e } };
static constexpr GUID guid_cfg_channel_gain_front_wide_left  = { 0x8781ee1e, 0x2de3, 0x4aae, { 0x88, 0xb1, 0x55, 0xe1, 0xe6, 0x10, 0xa4, 0x7b } };
static constexpr GUID guid_cfg_channel_gain_front_wide_right = { 0xdfb1a355, 0x5048, 0x4982, { 0x9e, 0x74, 0xb5, 0x21, 0xb0, 0xe7, 0x4b, 0x0f } };
static constexpr GUID guid_cfg_channel_delay_front_left      = { 0x32e21083, 0x84b6, 0x4829, { 0xaf, 0xd0, 0xde, 0xe3, 0xa1, 0xb1, 0xfd, 0x10 } };
static constexpr GUID guid_cfg_channel_delay_front_right     = { 0xbeaafbde, 0x5b95, 0x4ac6, { 0x9c, 0xba, 0xaa, 0xb9, 0x4a, 0x0f, 0xce, 0x39 } };
static constexpr GUID guid_cfg_channel_delay_front_center    = { 0xfbad6922, 0x63b0, 0x4563, { 0xa0, 0xe8, 0x11, 0xb9, 0xe4, 0x49, 0x77, 0xda } };
static constexpr GUID guid_cfg_channel_delay_lfe             = { 0xed185a3b, 0x9938, 0x4439, { 0x8e, 0x60, 0x88, 0x98, 0x64, 0xa2, 0xdc, 0x26 } };
static constexpr GUID guid_cfg_channel_delay_side_left       = { 0xbba910ec, 0x3d35, 0x456b, { 0xa9, 0xff, 0x8c, 0xc8, 0x9b, 0xd3, 0x51, 0xc2 } };
static constexpr GUID guid_cfg_channel_delay_side_right      = { 0x6058a7bb, 0x6886, 0x4e56, { 0xa1, 0xdf, 0xf4, 0x35, 0x3f, 0xa8, 0x55, 0x0d } };
static constexpr GUID guid_cfg_channel_delay_back_left       = { 0x3953a185, 0xa0f2, 0x4209, { 0x81, 0x0c, 0x36, 0x52, 0x5e, 0xf6, 0x6e, 0x56 } };
static constexpr GUID guid_cfg_channel_delay_back_right      = { 0xa943e6c8, 0xd511, 0x491c, { 0xb9, 0xf5, 0xa0, 0xe7, 0x94, 0x33, 0xee, 0x97 } };
static constexpr GUID guid_cfg_channel_delay_top_front_left  = { 0xf9808900, 0x641d, 0x41df, { 0xa3, 0x99, 0x51, 0xee, 0x79, 0xb0, 0x53, 0xcf } };
static constexpr GUID guid_cfg_channel_delay_top_front_right = { 0x870a0c0c, 0xf1d6, 0x4f17, { 0xbf, 0xfc, 0xa8, 0x71, 0xf6, 0xcf, 0x1a, 0xfb } };
static constexpr GUID guid_cfg_channel_delay_top_back_left   = { 0x7b76d1b2, 0x1cd5, 0x4636, { 0xb9, 0x17, 0xa1, 0x6f, 0x9d, 0x2a, 0xfa, 0x3d } };
static constexpr GUID guid_cfg_channel_delay_top_back_right  = { 0x285552a0, 0x3c67, 0x4b5b, { 0xad, 0x96, 0x3a, 0x55, 0x33, 0x2b, 0x50, 0x00 } };
static constexpr GUID guid_cfg_channel_delay_front_wide_left = { 0x8eda49bc, 0xbe76, 0x49d3, { 0xb4, 0x7a, 0x7a, 0x78, 0x55, 0xde, 0x2c, 0x3f } };
static constexpr GUID guid_cfg_channel_delay_front_wide_right= { 0xc1e0b4fe, 0x151e, 0x40ab, { 0x9d, 0xec, 0x74, 0xed, 0x7d, 0x57, 0x6e, 0x9d } };
static constexpr GUID guid_cfg_channel_invert_mask           = { 0xd60c8a95, 0xefea, 0x4a88, { 0xb2, 0xd5, 0x59, 0xd6, 0x77, 0x94, 0x7d, 0xda } };
static constexpr GUID guid_cfg_map51_front_left              = { 0x4eb609d7, 0xb273, 0x4dde, { 0x8d, 0xf9, 0x15, 0x93, 0x02, 0xac, 0x74, 0xda } };
static constexpr GUID guid_cfg_map51_front_right             = { 0xe8f17fbd, 0x3591, 0x4709, { 0x84, 0x0d, 0xb7, 0x11, 0x2b, 0x33, 0x65, 0x44 } };
static constexpr GUID guid_cfg_map51_front_center            = { 0x2bc81e7f, 0x9bff, 0x4820, { 0xa4, 0x31, 0xa0, 0x43, 0x23, 0x2d, 0xb2, 0xc9 } };
static constexpr GUID guid_cfg_map51_lfe                     = { 0xcb0694d1, 0xd8a3, 0x4bf1, { 0xb8, 0xb0, 0x19, 0xb9, 0xbe, 0xa4, 0x78, 0x47 } };
static constexpr GUID guid_cfg_map51_surround_left           = { 0x98f8a0f9, 0xb867, 0x47a8, { 0x8b, 0x04, 0xfe, 0xc6, 0xbd, 0x6e, 0x76, 0x6e } };
static constexpr GUID guid_cfg_map51_surround_right          = { 0xce34eb76, 0xb3ee, 0x4e50, { 0xbb, 0xb6, 0x36, 0x07, 0x11, 0x35, 0x68, 0x0d } };

static cfg_int   cfg_output_layout(guid_cfg_output_layout, static_cast<int>(DspOutputLayout::SevenPointOneFour));
static cfg_int   cfg_upmix_mode(guid_cfg_upmix_mode, static_cast<int>(UpmixMode::Reference));
static cfg_float cfg_master_gain(guid_cfg_master_gain, 0.0);
static cfg_float cfg_headroom(guid_cfg_headroom, 0.0);
static cfg_bool  cfg_limiter_enabled(guid_cfg_limiter_enabled, true);
static cfg_int   cfg_limiter_mode(guid_cfg_limiter_mode, static_cast<int>(LimiterMode::TransparentSoft));
static cfg_float cfg_limiter_ceiling(guid_cfg_limiter_ceiling, -1.0);
static cfg_float cfg_center_gain(guid_cfg_center_gain, 0.0);
static cfg_float cfg_surround_gain(guid_cfg_surround_gain, 0.0);
static cfg_float cfg_rear_gain(guid_cfg_rear_gain, 0.0);
static cfg_float cfg_height_gain(guid_cfg_height_gain, 0.0);
static cfg_float cfg_side_amount(guid_cfg_side_amount, 0.75);
static cfg_float cfg_height_from_mid(guid_cfg_height_from_mid, 0.20);
static cfg_float cfg_decorrelation(guid_cfg_decorrelation, 0.20);
static cfg_bool  cfg_enable_lfe(guid_cfg_enable_lfe, true);
static cfg_float cfg_lfe_gain(guid_cfg_lfe_gain, 0.0);
static cfg_float cfg_lfe_lowpass(guid_cfg_lfe_lowpass, 120.0);
static cfg_float cfg_channel_gain_front_left(guid_cfg_channel_gain_front_left, 0.0);
static cfg_float cfg_channel_gain_front_right(guid_cfg_channel_gain_front_right, 0.0);
static cfg_float cfg_channel_gain_front_center(guid_cfg_channel_gain_front_center, 0.0);
static cfg_float cfg_channel_gain_lfe(guid_cfg_channel_gain_lfe, 0.0);
static cfg_float cfg_channel_gain_side_left(guid_cfg_channel_gain_side_left, 0.0);
static cfg_float cfg_channel_gain_side_right(guid_cfg_channel_gain_side_right, 0.0);
static cfg_float cfg_channel_gain_back_left(guid_cfg_channel_gain_back_left, 0.0);
static cfg_float cfg_channel_gain_back_right(guid_cfg_channel_gain_back_right, 0.0);
static cfg_float cfg_channel_gain_top_front_left(guid_cfg_channel_gain_top_front_left, 0.0);
static cfg_float cfg_channel_gain_top_front_right(guid_cfg_channel_gain_top_front_right, 0.0);
static cfg_float cfg_channel_gain_top_back_left(guid_cfg_channel_gain_top_back_left, 0.0);
static cfg_float cfg_channel_gain_top_back_right(guid_cfg_channel_gain_top_back_right, 0.0);
static cfg_float cfg_channel_gain_front_wide_left(guid_cfg_channel_gain_front_wide_left, 0.0);
static cfg_float cfg_channel_gain_front_wide_right(guid_cfg_channel_gain_front_wide_right, 0.0);
static cfg_float cfg_channel_delay_front_left(guid_cfg_channel_delay_front_left, 0.0);
static cfg_float cfg_channel_delay_front_right(guid_cfg_channel_delay_front_right, 0.0);
static cfg_float cfg_channel_delay_front_center(guid_cfg_channel_delay_front_center, 0.0);
static cfg_float cfg_channel_delay_lfe(guid_cfg_channel_delay_lfe, 0.0);
static cfg_float cfg_channel_delay_side_left(guid_cfg_channel_delay_side_left, 0.0);
static cfg_float cfg_channel_delay_side_right(guid_cfg_channel_delay_side_right, 0.0);
static cfg_float cfg_channel_delay_back_left(guid_cfg_channel_delay_back_left, 0.0);
static cfg_float cfg_channel_delay_back_right(guid_cfg_channel_delay_back_right, 0.0);
static cfg_float cfg_channel_delay_top_front_left(guid_cfg_channel_delay_top_front_left, 0.0);
static cfg_float cfg_channel_delay_top_front_right(guid_cfg_channel_delay_top_front_right, 0.0);
static cfg_float cfg_channel_delay_top_back_left(guid_cfg_channel_delay_top_back_left, 0.0);
static cfg_float cfg_channel_delay_top_back_right(guid_cfg_channel_delay_top_back_right, 0.0);
static cfg_float cfg_channel_delay_front_wide_left(guid_cfg_channel_delay_front_wide_left, 0.0);
static cfg_float cfg_channel_delay_front_wide_right(guid_cfg_channel_delay_front_wide_right, 0.0);
static cfg_int   cfg_channel_invert_mask(guid_cfg_channel_invert_mask, 0);
static cfg_int   cfg_map51_front_left(guid_cfg_map51_front_left, target_front_left);
static cfg_int   cfg_map51_front_right(guid_cfg_map51_front_right, target_front_right);
static cfg_int   cfg_map51_front_center(guid_cfg_map51_front_center, target_front_center);
static cfg_int   cfg_map51_lfe(guid_cfg_map51_lfe, target_low_frequency);
static cfg_int   cfg_map51_surround_left(guid_cfg_map51_surround_left, target_side_left);
static cfg_int   cfg_map51_surround_right(guid_cfg_map51_surround_right, target_side_right);

static cfg_float* const channel_gain_cfgs[target_count] = {
    &cfg_channel_gain_front_left,
    &cfg_channel_gain_front_right,
    &cfg_channel_gain_front_center,
    &cfg_channel_gain_lfe,
    &cfg_channel_gain_side_left,
    &cfg_channel_gain_side_right,
    &cfg_channel_gain_back_left,
    &cfg_channel_gain_back_right,
    &cfg_channel_gain_top_front_left,
    &cfg_channel_gain_top_front_right,
    &cfg_channel_gain_top_back_left,
    &cfg_channel_gain_top_back_right,
    &cfg_channel_gain_front_wide_left,
    &cfg_channel_gain_front_wide_right,
};

static cfg_float* const channel_delay_cfgs[target_count] = {
    &cfg_channel_delay_front_left,
    &cfg_channel_delay_front_right,
    &cfg_channel_delay_front_center,
    &cfg_channel_delay_lfe,
    &cfg_channel_delay_side_left,
    &cfg_channel_delay_side_right,
    &cfg_channel_delay_back_left,
    &cfg_channel_delay_back_right,
    &cfg_channel_delay_top_front_left,
    &cfg_channel_delay_top_front_right,
    &cfg_channel_delay_top_back_left,
    &cfg_channel_delay_top_back_right,
    &cfg_channel_delay_front_wide_left,
    &cfg_channel_delay_front_wide_right,
};

static LimiterMode limiter_mode_from_int(int value) {
    return value == static_cast<int>(LimiterMode::HardCeiling) ? LimiterMode::HardCeiling : LimiterMode::TransparentSoft;
}

static UpmixMode upmix_mode_from_int(int value) {
    switch (value) {
    case static_cast<int>(UpmixMode::Reference): return UpmixMode::Reference;
    case static_cast<int>(UpmixMode::FrontOnly): return UpmixMode::FrontOnly;
    default: return UpmixMode::Full;
    }
}

static DspOutputLayout output_layout_from_int(int value) {
    switch (value) {
    case static_cast<int>(DspOutputLayout::FivePointOne): return DspOutputLayout::FivePointOne;
    case static_cast<int>(DspOutputLayout::SevenPointOne): return DspOutputLayout::SevenPointOne;
    case static_cast<int>(DspOutputLayout::FivePointOneTwo): return DspOutputLayout::FivePointOneTwo;
    case static_cast<int>(DspOutputLayout::FivePointOneFour): return DspOutputLayout::FivePointOneFour;
    case static_cast<int>(DspOutputLayout::NinePointOne): return DspOutputLayout::NinePointOne;
    case static_cast<int>(DspOutputLayout::NinePointOneTwo): return DspOutputLayout::NinePointOneTwo;
    case static_cast<int>(DspOutputLayout::NinePointOneFour): return DspOutputLayout::NinePointOneFour;
    default: return DspOutputLayout::SevenPointOneFour;
    }
}

static std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

static std::map<std::string, std::string> parse_profile_values(const std::string& text) {
    std::map<std::string, std::string> values;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (!key.empty()) values[key] = value;
    }
    return values;
}

static bool read_int_key(const std::map<std::string, std::string>& values, const char* key, int& output) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    char* end = nullptr;
    const long parsed = std::strtol(it->second.c_str(), &end, 10);
    if (end == it->second.c_str()) return false;
    output = static_cast<int>(parsed);
    return true;
}

static bool read_double_key(const std::map<std::string, std::string>& values, const char* key, double& output) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    char* end = nullptr;
    const double parsed = std::strtod(it->second.c_str(), &end);
    if (end == it->second.c_str()) return false;
    output = parsed;
    return true;
}

static bool read_bool_key(const std::map<std::string, std::string>& values, const char* key, bool& output) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    const std::string value = it->second;
    if (value == "1" || value == "true" || value == "yes" || value == "on")  { output = true;  return true; }
    if (value == "0" || value == "false" || value == "no" || value == "off") { output = false; return true; }
    return false;
}

DspConfig DefaultDspConfig() {
    return {};
}

DspConfig ReadDspConfig() {
    DspConfig config;
    config.outputLayout   = output_layout_from_int(static_cast<int>(cfg_output_layout.get()));
    config.upmixMode      = upmix_mode_from_int(static_cast<int>(cfg_upmix_mode.get()));
    config.masterGainDb   = cfg_master_gain.get();
    config.headroomDb     = cfg_headroom.get();
    config.limiterEnabled = cfg_limiter_enabled.get();
    config.limiterMode    = limiter_mode_from_int(static_cast<int>(cfg_limiter_mode.get()));
    config.limiterCeilingDb  = cfg_limiter_ceiling.get();
    config.centerGainDb      = cfg_center_gain.get();
    config.surroundGainDb    = cfg_surround_gain.get();
    config.rearGainDb        = cfg_rear_gain.get();
    config.heightGainDb      = cfg_height_gain.get();
    config.sideAmount        = cfg_side_amount.get();
    config.heightFromMid     = cfg_height_from_mid.get();
    config.decorrelationAmount = cfg_decorrelation.get();
    config.enableLfe         = cfg_enable_lfe.get();
    config.lfeGainDb         = cfg_lfe_gain.get();
    config.lfeLowpassHz      = cfg_lfe_lowpass.get();
    for (size_t i = 0; i < target_count; ++i) {
        config.channelGainDb[i]  = channel_gain_cfgs[i]->get();
        config.channelDelayMs[i] = channel_delay_cfgs[i]->get();
        config.channelInvert[i]  = (static_cast<int>(cfg_channel_invert_mask.get()) & (1 << i)) != 0;
    }
    config.map51FrontLeft    = static_cast<int>(cfg_map51_front_left.get());
    config.map51FrontRight   = static_cast<int>(cfg_map51_front_right.get());
    config.map51FrontCenter  = static_cast<int>(cfg_map51_front_center.get());
    config.map51Lfe          = static_cast<int>(cfg_map51_lfe.get());
    config.map51SurroundLeft = static_cast<int>(cfg_map51_surround_left.get());
    config.map51SurroundRight= static_cast<int>(cfg_map51_surround_right.get());
    return config;
}

void WriteDspConfig(const DspConfig& config) {
    cfg_output_layout = static_cast<int>(config.outputLayout);
    cfg_upmix_mode    = static_cast<int>(config.upmixMode);
    cfg_master_gain   = static_cast<float>(config.masterGainDb);
    cfg_headroom      = static_cast<float>(config.headroomDb);
    cfg_limiter_enabled = config.limiterEnabled;
    cfg_limiter_mode  = static_cast<int>(config.limiterMode);
    cfg_limiter_ceiling = static_cast<float>(config.limiterCeilingDb);
    cfg_center_gain   = static_cast<float>(config.centerGainDb);
    cfg_surround_gain = static_cast<float>(config.surroundGainDb);
    cfg_rear_gain     = static_cast<float>(config.rearGainDb);
    cfg_height_gain   = static_cast<float>(config.heightGainDb);
    cfg_side_amount   = static_cast<float>(config.sideAmount);
    cfg_height_from_mid = static_cast<float>(config.heightFromMid);
    cfg_decorrelation = static_cast<float>(config.decorrelationAmount);
    cfg_enable_lfe    = config.enableLfe;
    cfg_lfe_gain      = static_cast<float>(config.lfeGainDb);
    cfg_lfe_lowpass   = static_cast<float>(config.lfeLowpassHz);
    for (size_t i = 0; i < target_count; ++i) {
        *channel_gain_cfgs[i]  = static_cast<float>(config.channelGainDb[i]);
        *channel_delay_cfgs[i] = static_cast<float>(config.channelDelayMs[i]);
    }
    int invertMask = 0;
    for (size_t i = 0; i < target_count; ++i) {
        if (config.channelInvert[i]) invertMask |= 1 << i;
    }
    cfg_channel_invert_mask   = invertMask;
    cfg_map51_front_left      = config.map51FrontLeft;
    cfg_map51_front_right     = config.map51FrontRight;
    cfg_map51_front_center    = config.map51FrontCenter;
    cfg_map51_lfe             = config.map51Lfe;
    cfg_map51_surround_left   = config.map51SurroundLeft;
    cfg_map51_surround_right  = config.map51SurroundRight;
}

std::string SerializeDspConfig(const DspConfig& config) {
    std::ostringstream output;
    output << std::setprecision(10);
    output << "[foo_dsp_spatial]\n";
    output << "version=1\n";
    output << "output_layout=" << static_cast<int>(config.outputLayout) << "\n";
    output << "upmix_mode=" << static_cast<int>(config.upmixMode) << "\n";
    output << "master_gain_db=" << config.masterGainDb << "\n";
    output << "headroom_db=" << config.headroomDb << "\n";
    output << "limiter_enabled=" << (config.limiterEnabled ? 1 : 0) << "\n";
    output << "limiter_mode=" << static_cast<int>(config.limiterMode) << "\n";
    output << "limiter_ceiling_db=" << config.limiterCeilingDb << "\n";
    output << "center_gain_db=" << config.centerGainDb << "\n";
    output << "surround_gain_db=" << config.surroundGainDb << "\n";
    output << "rear_gain_db=" << config.rearGainDb << "\n";
    output << "height_gain_db=" << config.heightGainDb << "\n";
    output << "side_amount=" << config.sideAmount << "\n";
    output << "height_from_mid=" << config.heightFromMid << "\n";
    output << "decorrelation_amount=" << config.decorrelationAmount << "\n";
    output << "enable_lfe=" << (config.enableLfe ? 1 : 0) << "\n";
    output << "lfe_gain_db=" << config.lfeGainDb << "\n";
    output << "lfe_lowpass_hz=" << config.lfeLowpassHz << "\n";
    for (size_t i = 0; i < target_count; ++i) {
        output << "channel_gain_" << kTargetKeys[i] << "=" << config.channelGainDb[i] << "\n";
        output << "channel_delay_" << kTargetKeys[i] << "_ms=" << config.channelDelayMs[i] << "\n";
        output << "channel_invert_" << kTargetKeys[i] << "=" << (config.channelInvert[i] ? 1 : 0) << "\n";
    }
    output << "map51_front_left="     << config.map51FrontLeft << "\n";
    output << "map51_front_right="    << config.map51FrontRight << "\n";
    output << "map51_front_center="   << config.map51FrontCenter << "\n";
    output << "map51_lfe="            << config.map51Lfe << "\n";
    output << "map51_surround_left="  << config.map51SurroundLeft << "\n";
    output << "map51_surround_right=" << config.map51SurroundRight << "\n";
    return output.str();
}

bool DeserializeDspConfig(const std::string& text, DspConfig& config) {
    const auto values = parse_profile_values(text);
    if (values.empty()) return false;
    const bool hasKnownKey = values.find("version") != values.end()
        || values.find("output_layout") != values.end()
        || values.find("upmix_mode") != values.end()
        || values.find("master_gain_db") != values.end()
        || values.find("map51_front_left") != values.end();
    if (!hasKnownKey) return false;

    DspConfig next = config;
    int intValue = 0;
    if (read_int_key(values, "output_layout", intValue)) next.outputLayout = output_layout_from_int(intValue);
    if (read_int_key(values, "upmix_mode", intValue)) next.upmixMode = upmix_mode_from_int(intValue);
    read_double_key(values, "master_gain_db", next.masterGainDb);
    read_double_key(values, "headroom_db", next.headroomDb);
    read_bool_key(values, "limiter_enabled", next.limiterEnabled);
    if (read_int_key(values, "limiter_mode", intValue)) next.limiterMode = limiter_mode_from_int(intValue);
    read_double_key(values, "limiter_ceiling_db", next.limiterCeilingDb);
    read_double_key(values, "center_gain_db", next.centerGainDb);
    read_double_key(values, "surround_gain_db", next.surroundGainDb);
    read_double_key(values, "rear_gain_db", next.rearGainDb);
    read_double_key(values, "height_gain_db", next.heightGainDb);
    read_double_key(values, "side_amount", next.sideAmount);
    read_double_key(values, "height_from_mid", next.heightFromMid);
    read_double_key(values, "decorrelation_amount", next.decorrelationAmount);
    read_bool_key(values, "enable_lfe", next.enableLfe);
    read_double_key(values, "lfe_gain_db", next.lfeGainDb);
    read_double_key(values, "lfe_lowpass_hz", next.lfeLowpassHz);
    for (size_t i = 0; i < target_count; ++i) {
        read_double_key(values, ("channel_gain_" + std::string(kTargetKeys[i])).c_str(), next.channelGainDb[i]);
        read_double_key(values, ("channel_delay_" + std::string(kTargetKeys[i]) + "_ms").c_str(), next.channelDelayMs[i]);
        read_bool_key(values, ("channel_invert_" + std::string(kTargetKeys[i])).c_str(), next.channelInvert[i]);
    }
    read_int_key(values, "map51_front_left",     next.map51FrontLeft);
    read_int_key(values, "map51_front_right",    next.map51FrontRight);
    read_int_key(values, "map51_front_center",   next.map51FrontCenter);
    read_int_key(values, "map51_lfe",            next.map51Lfe);
    read_int_key(values, "map51_surround_left",  next.map51SurroundLeft);
    read_int_key(values, "map51_surround_right", next.map51SurroundRight);

    config = next;
    return true;
}

} // namespace spatial_audio
