#include "stdafx.h"
#include "component_config.h"

namespace spatial_audio {

static constexpr GUID guid_cfg_master_gain = { 0xe84f6d6e, 0xf21a, 0x4d07, { 0x94, 0x9e, 0xde, 0x37, 0xb1, 0x02, 0x1c, 0x56 } };
static constexpr GUID guid_cfg_center_gain = { 0x7e551f08, 0x60ae, 0x46d6, { 0x8b, 0x0f, 0x3b, 0x0d, 0x48, 0x44, 0x3d, 0xfb } };
static constexpr GUID guid_cfg_surround_gain = { 0x28113b00, 0x8e41, 0x4512, { 0xba, 0x32, 0x1a, 0x1f, 0xfb, 0x12, 0xb4, 0xec } };
static constexpr GUID guid_cfg_rear_gain = { 0x038dbedb, 0x6775, 0x4021, { 0x81, 0xf8, 0xaa, 0x60, 0x9d, 0x86, 0xaa, 0x49 } };
static constexpr GUID guid_cfg_height_gain = { 0xa60851df, 0xb4d6, 0x45ba, { 0x86, 0x81, 0xf6, 0x95, 0x12, 0x4c, 0x4b, 0x80 } };
static constexpr GUID guid_cfg_side_amount = { 0x52f09f72, 0xfcd9, 0x4f87, { 0x9b, 0x00, 0x89, 0x3f, 0xff, 0xa7, 0x29, 0x9a } };
static constexpr GUID guid_cfg_height_from_mid = { 0x8218113a, 0xf961, 0x4107, { 0xad, 0xaa, 0xe5, 0x71, 0x5d, 0x3e, 0x92, 0x7c } };
static constexpr GUID guid_cfg_enable_lfe = { 0x50c31342, 0x1910, 0x4040, { 0x99, 0x8f, 0xee, 0x92, 0x19, 0x2a, 0xbe, 0x42 } };
static constexpr GUID guid_cfg_lfe_gain = { 0x606a82f6, 0xf333, 0x4fb8, { 0xa0, 0x1f, 0xe7, 0x86, 0xb3, 0x8b, 0x24, 0x87 } };
static constexpr GUID guid_cfg_lfe_lowpass = { 0xcdcc3b34, 0xdfb8, 0x4b55, { 0x94, 0x17, 0xc2, 0x4b, 0xf5, 0xfa, 0x3b, 0x7c } };
static constexpr GUID guid_cfg_map51_front_left = { 0x4eb609d7, 0xb273, 0x4dde, { 0x8d, 0xf9, 0x15, 0x93, 0x02, 0xac, 0x74, 0xda } };
static constexpr GUID guid_cfg_map51_front_right = { 0xe8f17fbd, 0x3591, 0x4709, { 0x84, 0x0d, 0xb7, 0x11, 0x2b, 0x33, 0x65, 0x44 } };
static constexpr GUID guid_cfg_map51_front_center = { 0x2bc81e7f, 0x9bff, 0x4820, { 0xa4, 0x31, 0xa0, 0x43, 0x23, 0x2d, 0xb2, 0xc9 } };
static constexpr GUID guid_cfg_map51_lfe = { 0xcb0694d1, 0xd8a3, 0x4bf1, { 0xb8, 0xb0, 0x19, 0xb9, 0xbe, 0xa4, 0x78, 0x47 } };
static constexpr GUID guid_cfg_map51_surround_left = { 0x98f8a0f9, 0xb867, 0x47a8, { 0x8b, 0x04, 0xfe, 0xc6, 0xbd, 0x6e, 0x76, 0x6e } };
static constexpr GUID guid_cfg_map51_surround_right = { 0xce34eb76, 0xb3ee, 0x4e50, { 0xbb, 0xb6, 0x36, 0x07, 0x11, 0x35, 0x68, 0x0d } };

static cfg_float cfg_master_gain(guid_cfg_master_gain, -12.0);
static cfg_float cfg_center_gain(guid_cfg_center_gain, -6.0);
static cfg_float cfg_surround_gain(guid_cfg_surround_gain, -9.0);
static cfg_float cfg_rear_gain(guid_cfg_rear_gain, -12.0);
static cfg_float cfg_height_gain(guid_cfg_height_gain, -12.0);
static cfg_float cfg_side_amount(guid_cfg_side_amount, 0.75);
static cfg_float cfg_height_from_mid(guid_cfg_height_from_mid, 0.20);
static cfg_bool cfg_enable_lfe(guid_cfg_enable_lfe, false);
static cfg_float cfg_lfe_gain(guid_cfg_lfe_gain, -24.0);
static cfg_float cfg_lfe_lowpass(guid_cfg_lfe_lowpass, 120.0);
static cfg_int cfg_map51_front_left(guid_cfg_map51_front_left, target_front_left);
static cfg_int cfg_map51_front_right(guid_cfg_map51_front_right, target_front_right);
static cfg_int cfg_map51_front_center(guid_cfg_map51_front_center, target_front_center);
static cfg_int cfg_map51_lfe(guid_cfg_map51_lfe, target_low_frequency);
static cfg_int cfg_map51_surround_left(guid_cfg_map51_surround_left, target_side_left);
static cfg_int cfg_map51_surround_right(guid_cfg_map51_surround_right, target_side_right);

RuntimeConfig DefaultConfig() {
    return {};
}

RuntimeConfig ReadConfig() {
    RuntimeConfig config;
    config.masterGainDb = cfg_master_gain.get();
    config.centerGainDb = cfg_center_gain.get();
    config.surroundGainDb = cfg_surround_gain.get();
    config.rearGainDb = cfg_rear_gain.get();
    config.heightGainDb = cfg_height_gain.get();
    config.sideAmount = cfg_side_amount.get();
    config.heightFromMid = cfg_height_from_mid.get();
    config.enableLfe = cfg_enable_lfe.get();
    config.lfeGainDb = cfg_lfe_gain.get();
    config.lfeLowpassHz = cfg_lfe_lowpass.get();
    config.map51FrontLeft = static_cast<int>(cfg_map51_front_left.get());
    config.map51FrontRight = static_cast<int>(cfg_map51_front_right.get());
    config.map51FrontCenter = static_cast<int>(cfg_map51_front_center.get());
    config.map51Lfe = static_cast<int>(cfg_map51_lfe.get());
    config.map51SurroundLeft = static_cast<int>(cfg_map51_surround_left.get());
    config.map51SurroundRight = static_cast<int>(cfg_map51_surround_right.get());
    return config;
}

void WriteConfig(const RuntimeConfig& config) {
    cfg_master_gain = static_cast<float>(config.masterGainDb);
    cfg_center_gain = static_cast<float>(config.centerGainDb);
    cfg_surround_gain = static_cast<float>(config.surroundGainDb);
    cfg_rear_gain = static_cast<float>(config.rearGainDb);
    cfg_height_gain = static_cast<float>(config.heightGainDb);
    cfg_side_amount = static_cast<float>(config.sideAmount);
    cfg_height_from_mid = static_cast<float>(config.heightFromMid);
    cfg_enable_lfe = config.enableLfe;
    cfg_lfe_gain = static_cast<float>(config.lfeGainDb);
    cfg_lfe_lowpass = static_cast<float>(config.lfeLowpassHz);
    cfg_map51_front_left = config.map51FrontLeft;
    cfg_map51_front_right = config.map51FrontRight;
    cfg_map51_front_center = config.map51FrontCenter;
    cfg_map51_lfe = config.map51Lfe;
    cfg_map51_surround_left = config.map51SurroundLeft;
    cfg_map51_surround_right = config.map51SurroundRight;
}

}  // namespace spatial_audio
