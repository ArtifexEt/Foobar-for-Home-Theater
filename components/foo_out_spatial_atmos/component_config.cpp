#include "stdafx.h"
#include "component_config.h"

namespace spatial_atmos {

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
    return config;
}

void WriteConfig(const RuntimeConfig& config) {
    cfg_master_gain.set(config.masterGainDb);
    cfg_center_gain.set(config.centerGainDb);
    cfg_surround_gain.set(config.surroundGainDb);
    cfg_rear_gain.set(config.rearGainDb);
    cfg_height_gain.set(config.heightGainDb);
    cfg_side_amount.set(config.sideAmount);
    cfg_height_from_mid.set(config.heightFromMid);
    cfg_enable_lfe.set(config.enableLfe);
    cfg_lfe_gain.set(config.lfeGainDb);
    cfg_lfe_lowpass.set(config.lfeLowpassHz);
}

}  // namespace spatial_atmos
