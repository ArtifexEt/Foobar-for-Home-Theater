#include "stdafx.h"
#include "component_config.h"

namespace spatial_audio {

static constexpr GUID guid_cfg_master_gain = { 0xe84f6d6e, 0xf21a, 0x4d07, { 0x94, 0x9e, 0xde, 0x37, 0xb1, 0x02, 0x1c, 0x56 } };
static constexpr GUID guid_cfg_layout_mode = { 0xb4dfd071, 0x7707, 0x4967, { 0xaa, 0x38, 0x2b, 0x81, 0xf1, 0xf1, 0x8d, 0xad } };
static constexpr GUID guid_cfg_sample_rate_mode = { 0x9df89513, 0x3f1b, 0x4548, { 0xa2, 0x01, 0x26, 0xc4, 0x55, 0x49, 0x8f, 0xf1 } };
static constexpr GUID guid_cfg_headroom = { 0x810b1a63, 0xf065, 0x417d, { 0x84, 0x66, 0x07, 0xce, 0x0f, 0xf5, 0x68, 0xf0 } };
static constexpr GUID guid_cfg_limiter_enabled = { 0x26f390c3, 0xf665, 0x4cd1, { 0x96, 0x72, 0x44, 0xaa, 0xa8, 0xa4, 0xe1, 0x7a } };
static constexpr GUID guid_cfg_limiter_mode = { 0x6dd2b549, 0x7f65, 0x4aa9, { 0xb7, 0x89, 0x1a, 0x9a, 0xdc, 0x44, 0x30, 0xe9 } };
static constexpr GUID guid_cfg_limiter_ceiling = { 0x02d2e53a, 0xcdf9, 0x44df, { 0x8c, 0xda, 0xea, 0x8f, 0x6e, 0x06, 0xe9, 0xfb } };
static constexpr GUID guid_cfg_center_gain = { 0x7e551f08, 0x60ae, 0x46d6, { 0x8b, 0x0f, 0x3b, 0x0d, 0x48, 0x44, 0x3d, 0xfb } };
static constexpr GUID guid_cfg_surround_gain = { 0x28113b00, 0x8e41, 0x4512, { 0xba, 0x32, 0x1a, 0x1f, 0xfb, 0x12, 0xb4, 0xec } };
static constexpr GUID guid_cfg_rear_gain = { 0x038dbedb, 0x6775, 0x4021, { 0x81, 0xf8, 0xaa, 0x60, 0x9d, 0x86, 0xaa, 0x49 } };
static constexpr GUID guid_cfg_height_gain = { 0xa60851df, 0xb4d6, 0x45ba, { 0x86, 0x81, 0xf6, 0x95, 0x12, 0x4c, 0x4b, 0x80 } };
static constexpr GUID guid_cfg_side_amount = { 0x52f09f72, 0xfcd9, 0x4f87, { 0x9b, 0x00, 0x89, 0x3f, 0xff, 0xa7, 0x29, 0x9a } };
static constexpr GUID guid_cfg_height_from_mid = { 0x8218113a, 0xf961, 0x4107, { 0xad, 0xaa, 0xe5, 0x71, 0x5d, 0x3e, 0x92, 0x7c } };
static constexpr GUID guid_cfg_decorrelation = { 0xfe89868e, 0x81e3, 0x4322, { 0x93, 0x04, 0x9c, 0x3a, 0xf3, 0xdc, 0x43, 0x51 } };
static constexpr GUID guid_cfg_enable_lfe = { 0x50c31342, 0x1910, 0x4040, { 0x99, 0x8f, 0xee, 0x92, 0x19, 0x2a, 0xbe, 0x42 } };
static constexpr GUID guid_cfg_lfe_gain = { 0x606a82f6, 0xf333, 0x4fb8, { 0xa0, 0x1f, 0xe7, 0x86, 0xb3, 0x8b, 0x24, 0x87 } };
static constexpr GUID guid_cfg_lfe_lowpass = { 0xcdcc3b34, 0xdfb8, 0x4b55, { 0x94, 0x17, 0xc2, 0x4b, 0xf5, 0xfa, 0x3b, 0x7c } };
static constexpr GUID guid_cfg_channel_gain_front_left = { 0x9fdf40ea, 0xd51e, 0x49bc, { 0xad, 0x5b, 0xe4, 0x15, 0x48, 0x1c, 0xd6, 0x0c } };
static constexpr GUID guid_cfg_channel_gain_front_right = { 0x56dc1b1a, 0x685f, 0x461f, { 0xa3, 0x4b, 0x14, 0x6b, 0x01, 0x9f, 0x05, 0x5e } };
static constexpr GUID guid_cfg_channel_gain_front_center = { 0x80e56c12, 0x15c3, 0x410d, { 0x98, 0xb9, 0xe5, 0x7f, 0xfa, 0x58, 0x6c, 0x22 } };
static constexpr GUID guid_cfg_channel_gain_lfe = { 0x37fec322, 0xb817, 0x42c6, { 0xb7, 0x29, 0x4c, 0x43, 0x18, 0x66, 0x4c, 0x0c } };
static constexpr GUID guid_cfg_channel_gain_side_left = { 0xd31c4838, 0x5f8e, 0x4176, { 0x9f, 0x01, 0x9e, 0xbc, 0x88, 0x97, 0x56, 0x45 } };
static constexpr GUID guid_cfg_channel_gain_side_right = { 0x4b85f376, 0x140e, 0x4d54, { 0xad, 0xaa, 0x75, 0xe8, 0xc7, 0x50, 0xcf, 0x0e } };
static constexpr GUID guid_cfg_channel_gain_back_left = { 0xea1ca85d, 0x00b9, 0x471f, { 0x8f, 0xc0, 0x39, 0xdf, 0x90, 0x91, 0xf2, 0x56 } };
static constexpr GUID guid_cfg_channel_gain_back_right = { 0x9d48255b, 0xdd71, 0x46e6, { 0x90, 0x56, 0x38, 0x71, 0x67, 0x8e, 0x8f, 0x06 } };
static constexpr GUID guid_cfg_channel_gain_top_front_left = { 0xef1937bf, 0xaf33, 0x4d15, { 0x98, 0x3e, 0xe7, 0x11, 0xeb, 0xef, 0xfe, 0xc0 } };
static constexpr GUID guid_cfg_channel_gain_top_front_right = { 0x607d4b1f, 0xe576, 0x4302, { 0x9b, 0x6d, 0x2a, 0x72, 0x67, 0x76, 0xfe, 0x70 } };
static constexpr GUID guid_cfg_channel_gain_top_back_left = { 0x5e37467a, 0xe697, 0x40e2, { 0xb0, 0x98, 0x4d, 0x25, 0xdd, 0x48, 0x6b, 0xe1 } };
static constexpr GUID guid_cfg_channel_gain_top_back_right = { 0x906a45bc, 0x2bd3, 0x43fe, { 0xa2, 0x11, 0x3c, 0xec, 0x48, 0xcc, 0xd2, 0x3e } };
static constexpr GUID guid_cfg_channel_delay_front_left = { 0x32e21083, 0x84b6, 0x4829, { 0xaf, 0xd0, 0xde, 0xe3, 0xa1, 0xb1, 0xfd, 0x10 } };
static constexpr GUID guid_cfg_channel_delay_front_right = { 0xbeaafbde, 0x5b95, 0x4ac6, { 0x9c, 0xba, 0xaa, 0xb9, 0x4a, 0x0f, 0xce, 0x39 } };
static constexpr GUID guid_cfg_channel_delay_front_center = { 0xfbad6922, 0x63b0, 0x4563, { 0xa0, 0xe8, 0x11, 0xb9, 0xe4, 0x49, 0x77, 0xda } };
static constexpr GUID guid_cfg_channel_delay_lfe = { 0xed185a3b, 0x9938, 0x4439, { 0x8e, 0x60, 0x88, 0x98, 0x64, 0xa2, 0xdc, 0x26 } };
static constexpr GUID guid_cfg_channel_delay_side_left = { 0xbba910ec, 0x3d35, 0x456b, { 0xa9, 0xff, 0x8c, 0xc8, 0x9b, 0xd3, 0x51, 0xc2 } };
static constexpr GUID guid_cfg_channel_delay_side_right = { 0x6058a7bb, 0x6886, 0x4e56, { 0xa1, 0xdf, 0xf4, 0x35, 0x3f, 0xa8, 0x55, 0x0d } };
static constexpr GUID guid_cfg_channel_delay_back_left = { 0x3953a185, 0xa0f2, 0x4209, { 0x81, 0x0c, 0x36, 0x52, 0x5e, 0xf6, 0x6e, 0x56 } };
static constexpr GUID guid_cfg_channel_delay_back_right = { 0xa943e6c8, 0xd511, 0x491c, { 0xb9, 0xf5, 0xa0, 0xe7, 0x94, 0x33, 0xee, 0x97 } };
static constexpr GUID guid_cfg_channel_delay_top_front_left = { 0xf9808900, 0x641d, 0x41df, { 0xa3, 0x99, 0x51, 0xee, 0x79, 0xb0, 0x53, 0xcf } };
static constexpr GUID guid_cfg_channel_delay_top_front_right = { 0x870a0c0c, 0xf1d6, 0x4f17, { 0xbf, 0xfc, 0xa8, 0x71, 0xf6, 0xcf, 0x1a, 0xfb } };
static constexpr GUID guid_cfg_channel_delay_top_back_left = { 0x7b76d1b2, 0x1cd5, 0x4636, { 0xb9, 0x17, 0xa1, 0x6f, 0x9d, 0x2a, 0xfa, 0x3d } };
static constexpr GUID guid_cfg_channel_delay_top_back_right = { 0x285552a0, 0x3c67, 0x4b5b, { 0xad, 0x96, 0x3a, 0x55, 0x33, 0x2b, 0x50, 0x00 } };
static constexpr GUID guid_cfg_map51_front_left = { 0x4eb609d7, 0xb273, 0x4dde, { 0x8d, 0xf9, 0x15, 0x93, 0x02, 0xac, 0x74, 0xda } };
static constexpr GUID guid_cfg_map51_front_right = { 0xe8f17fbd, 0x3591, 0x4709, { 0x84, 0x0d, 0xb7, 0x11, 0x2b, 0x33, 0x65, 0x44 } };
static constexpr GUID guid_cfg_map51_front_center = { 0x2bc81e7f, 0x9bff, 0x4820, { 0xa4, 0x31, 0xa0, 0x43, 0x23, 0x2d, 0xb2, 0xc9 } };
static constexpr GUID guid_cfg_map51_lfe = { 0xcb0694d1, 0xd8a3, 0x4bf1, { 0xb8, 0xb0, 0x19, 0xb9, 0xbe, 0xa4, 0x78, 0x47 } };
static constexpr GUID guid_cfg_map51_surround_left = { 0x98f8a0f9, 0xb867, 0x47a8, { 0x8b, 0x04, 0xfe, 0xc6, 0xbd, 0x6e, 0x76, 0x6e } };
static constexpr GUID guid_cfg_map51_surround_right = { 0xce34eb76, 0xb3ee, 0x4e50, { 0xbb, 0xb6, 0x36, 0x07, 0x11, 0x35, 0x68, 0x0d } };
static constexpr GUID guid_cfg_directional_test_enabled = { 0xe72fa1b9, 0x30f3, 0x480a, { 0x8c, 0xf1, 0xbc, 0xa8, 0xd1, 0xde, 0x09, 0xc7 } };
static constexpr GUID guid_cfg_directional_test_dynamic = { 0x59202fd2, 0x991f, 0x493f, { 0xba, 0xbe, 0x10, 0x93, 0x81, 0x78, 0x82, 0x2d } };
static constexpr GUID guid_cfg_directional_test_target = { 0x7fef32d8, 0x03e5, 0x4a89, { 0x98, 0x0d, 0x19, 0xe2, 0x8f, 0xeb, 0x67, 0x2b } };
static constexpr GUID guid_cfg_directional_test_gain = { 0xb0e4807b, 0xb7fd, 0x4b71, { 0x89, 0x7b, 0xa6, 0x26, 0x31, 0x19, 0x6e, 0x4b } };
static constexpr GUID guid_cfg_directional_test_frequency = { 0x8e136568, 0x5de0, 0x4ef9, { 0xb8, 0x5e, 0x47, 0xec, 0x43, 0x28, 0x01, 0xbf } };

static cfg_int cfg_layout_mode(guid_cfg_layout_mode, static_cast<int>(LayoutMode::Auto));
static cfg_int cfg_sample_rate_mode(guid_cfg_sample_rate_mode, static_cast<int>(SampleRateMode::AutoHighest));
static cfg_float cfg_master_gain(guid_cfg_master_gain, -12.0);
static cfg_float cfg_headroom(guid_cfg_headroom, 0.0);
static cfg_bool cfg_limiter_enabled(guid_cfg_limiter_enabled, true);
static cfg_int cfg_limiter_mode(guid_cfg_limiter_mode, static_cast<int>(LimiterMode::TransparentSoft));
static cfg_float cfg_limiter_ceiling(guid_cfg_limiter_ceiling, -1.0);
static cfg_float cfg_center_gain(guid_cfg_center_gain, -6.0);
static cfg_float cfg_surround_gain(guid_cfg_surround_gain, -9.0);
static cfg_float cfg_rear_gain(guid_cfg_rear_gain, -12.0);
static cfg_float cfg_height_gain(guid_cfg_height_gain, -12.0);
static cfg_float cfg_side_amount(guid_cfg_side_amount, 0.75);
static cfg_float cfg_height_from_mid(guid_cfg_height_from_mid, 0.20);
static cfg_float cfg_decorrelation(guid_cfg_decorrelation, 0.20);
static cfg_bool cfg_enable_lfe(guid_cfg_enable_lfe, false);
static cfg_float cfg_lfe_gain(guid_cfg_lfe_gain, -24.0);
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
static cfg_int cfg_map51_front_left(guid_cfg_map51_front_left, target_front_left);
static cfg_int cfg_map51_front_right(guid_cfg_map51_front_right, target_front_right);
static cfg_int cfg_map51_front_center(guid_cfg_map51_front_center, target_front_center);
static cfg_int cfg_map51_lfe(guid_cfg_map51_lfe, target_low_frequency);
static cfg_int cfg_map51_surround_left(guid_cfg_map51_surround_left, target_side_left);
static cfg_int cfg_map51_surround_right(guid_cfg_map51_surround_right, target_side_right);
static cfg_bool cfg_directional_test_enabled(guid_cfg_directional_test_enabled, false);
static cfg_bool cfg_directional_test_dynamic(guid_cfg_directional_test_dynamic, true);
static cfg_int cfg_directional_test_target(guid_cfg_directional_test_target, target_front_center);
static cfg_float cfg_directional_test_gain(guid_cfg_directional_test_gain, -18.0);
static cfg_float cfg_directional_test_frequency(guid_cfg_directional_test_frequency, 660.0);

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
};

static LayoutMode layout_from_int(int value) {
    switch (value) {
    case static_cast<int>(LayoutMode::Stereo):
        return LayoutMode::Stereo;
    case static_cast<int>(LayoutMode::FivePointOne):
        return LayoutMode::FivePointOne;
    case static_cast<int>(LayoutMode::SevenPointOne):
        return LayoutMode::SevenPointOne;
    case static_cast<int>(LayoutMode::FivePointOneTwo):
        return LayoutMode::FivePointOneTwo;
    case static_cast<int>(LayoutMode::SevenPointOneFour):
        return LayoutMode::SevenPointOneFour;
    default:
        return LayoutMode::Auto;
    }
}

static SampleRateMode sample_rate_mode_from_int(int value) {
    switch (value) {
    case static_cast<int>(SampleRateMode::SourceIfSupported):
        return SampleRateMode::SourceIfSupported;
    case static_cast<int>(SampleRateMode::Fixed44100):
        return SampleRateMode::Fixed44100;
    case static_cast<int>(SampleRateMode::Fixed48000):
        return SampleRateMode::Fixed48000;
    case static_cast<int>(SampleRateMode::Fixed88200):
        return SampleRateMode::Fixed88200;
    case static_cast<int>(SampleRateMode::Fixed96000):
        return SampleRateMode::Fixed96000;
    case static_cast<int>(SampleRateMode::Fixed176400):
        return SampleRateMode::Fixed176400;
    case static_cast<int>(SampleRateMode::Fixed192000):
        return SampleRateMode::Fixed192000;
    default:
        return SampleRateMode::AutoHighest;
    }
}

static LimiterMode limiter_mode_from_int(int value) {
    return value == static_cast<int>(LimiterMode::HardCeiling) ? LimiterMode::HardCeiling : LimiterMode::TransparentSoft;
}

RuntimeConfig DefaultConfig() {
    return {};
}

RuntimeConfig ReadConfig() {
    RuntimeConfig config;
    config.layoutMode = layout_from_int(static_cast<int>(cfg_layout_mode.get()));
    config.sampleRateMode = sample_rate_mode_from_int(static_cast<int>(cfg_sample_rate_mode.get()));
    config.masterGainDb = cfg_master_gain.get();
    config.headroomDb = cfg_headroom.get();
    config.limiterEnabled = cfg_limiter_enabled.get();
    config.limiterMode = limiter_mode_from_int(static_cast<int>(cfg_limiter_mode.get()));
    config.limiterCeilingDb = cfg_limiter_ceiling.get();
    config.centerGainDb = cfg_center_gain.get();
    config.surroundGainDb = cfg_surround_gain.get();
    config.rearGainDb = cfg_rear_gain.get();
    config.heightGainDb = cfg_height_gain.get();
    config.sideAmount = cfg_side_amount.get();
    config.heightFromMid = cfg_height_from_mid.get();
    config.decorrelationAmount = cfg_decorrelation.get();
    config.enableLfe = cfg_enable_lfe.get();
    config.lfeGainDb = cfg_lfe_gain.get();
    config.lfeLowpassHz = cfg_lfe_lowpass.get();
    for (size_t i = 0; i < target_count; ++i) {
        config.channelGainDb[i] = channel_gain_cfgs[i]->get();
        config.channelDelayMs[i] = channel_delay_cfgs[i]->get();
    }
    config.map51FrontLeft = static_cast<int>(cfg_map51_front_left.get());
    config.map51FrontRight = static_cast<int>(cfg_map51_front_right.get());
    config.map51FrontCenter = static_cast<int>(cfg_map51_front_center.get());
    config.map51Lfe = static_cast<int>(cfg_map51_lfe.get());
    config.map51SurroundLeft = static_cast<int>(cfg_map51_surround_left.get());
    config.map51SurroundRight = static_cast<int>(cfg_map51_surround_right.get());
    config.directionalTestEnabled = cfg_directional_test_enabled.get();
    config.directionalTestUseDynamicObject = cfg_directional_test_dynamic.get();
    config.directionalTestTarget = static_cast<int>(cfg_directional_test_target.get());
    config.directionalTestGainDb = cfg_directional_test_gain.get();
    config.directionalTestFrequencyHz = cfg_directional_test_frequency.get();
    return config;
}

void WriteConfig(const RuntimeConfig& config) {
    cfg_layout_mode = static_cast<int>(config.layoutMode);
    cfg_sample_rate_mode = static_cast<int>(config.sampleRateMode);
    cfg_master_gain = static_cast<float>(config.masterGainDb);
    cfg_headroom = static_cast<float>(config.headroomDb);
    cfg_limiter_enabled = config.limiterEnabled;
    cfg_limiter_mode = static_cast<int>(config.limiterMode);
    cfg_limiter_ceiling = static_cast<float>(config.limiterCeilingDb);
    cfg_center_gain = static_cast<float>(config.centerGainDb);
    cfg_surround_gain = static_cast<float>(config.surroundGainDb);
    cfg_rear_gain = static_cast<float>(config.rearGainDb);
    cfg_height_gain = static_cast<float>(config.heightGainDb);
    cfg_side_amount = static_cast<float>(config.sideAmount);
    cfg_height_from_mid = static_cast<float>(config.heightFromMid);
    cfg_decorrelation = static_cast<float>(config.decorrelationAmount);
    cfg_enable_lfe = config.enableLfe;
    cfg_lfe_gain = static_cast<float>(config.lfeGainDb);
    cfg_lfe_lowpass = static_cast<float>(config.lfeLowpassHz);
    for (size_t i = 0; i < target_count; ++i) {
        *channel_gain_cfgs[i] = static_cast<float>(config.channelGainDb[i]);
        *channel_delay_cfgs[i] = static_cast<float>(config.channelDelayMs[i]);
    }
    cfg_map51_front_left = config.map51FrontLeft;
    cfg_map51_front_right = config.map51FrontRight;
    cfg_map51_front_center = config.map51FrontCenter;
    cfg_map51_lfe = config.map51Lfe;
    cfg_map51_surround_left = config.map51SurroundLeft;
    cfg_map51_surround_right = config.map51SurroundRight;
    cfg_directional_test_enabled = config.directionalTestEnabled;
    cfg_directional_test_dynamic = config.directionalTestUseDynamicObject;
    cfg_directional_test_target = config.directionalTestTarget;
    cfg_directional_test_gain = static_cast<float>(config.directionalTestGainDb);
    cfg_directional_test_frequency = static_cast<float>(config.directionalTestFrequencyHz);
}

}  // namespace spatial_audio
