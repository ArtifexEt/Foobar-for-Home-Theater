#include "stdafx.h"
#include "component_config.h"

namespace spatial_audio {

static constexpr GUID guid_cfg_layout_mode               = { 0xb4dfd071, 0x7707, 0x4967, { 0xaa, 0x38, 0x2b, 0x81, 0xf1, 0xf1, 0x8d, 0xad } };
static constexpr GUID guid_cfg_sample_rate_mode          = { 0x9df89513, 0x3f1b, 0x4548, { 0xa2, 0x01, 0x26, 0xc4, 0x55, 0x49, 0x8f, 0xf1 } };
static constexpr GUID guid_cfg_directional_test_enabled  = { 0xe72fa1b9, 0x30f3, 0x480a, { 0x8c, 0xf1, 0xbc, 0xa8, 0xd1, 0xde, 0x09, 0xc7 } };
static constexpr GUID guid_cfg_directional_test_dynamic  = { 0x59202fd2, 0x991f, 0x493f, { 0xba, 0xbe, 0x10, 0x93, 0x81, 0x78, 0x82, 0x2d } };
static constexpr GUID guid_cfg_directional_test_target   = { 0x7fef32d8, 0x03e5, 0x4a89, { 0x98, 0x0d, 0x19, 0xe2, 0x8f, 0xeb, 0x67, 0x2b } };
static constexpr GUID guid_cfg_directional_test_gain     = { 0xb0e4807b, 0xb7fd, 0x4b71, { 0x89, 0x7b, 0xa6, 0x26, 0x31, 0x19, 0x6e, 0x4b } };
static constexpr GUID guid_cfg_directional_test_frequency= { 0x8e136568, 0x5de0, 0x4ef9, { 0xb8, 0x5e, 0x47, 0xec, 0x43, 0x28, 0x01, 0xbf } };

static cfg_int   cfg_layout_mode(guid_cfg_layout_mode, static_cast<int>(LayoutMode::Auto));
static cfg_int   cfg_sample_rate_mode(guid_cfg_sample_rate_mode, static_cast<int>(SampleRateMode::Fixed48000));
static cfg_bool  cfg_directional_test_enabled(guid_cfg_directional_test_enabled, false);
static cfg_bool  cfg_directional_test_dynamic(guid_cfg_directional_test_dynamic, true);
static cfg_int   cfg_directional_test_target(guid_cfg_directional_test_target, target_front_center);
static cfg_float cfg_directional_test_gain(guid_cfg_directional_test_gain, -18.0);
static cfg_float cfg_directional_test_frequency(guid_cfg_directional_test_frequency, 660.0);

static LayoutMode layout_from_int(int value) {
    switch (value) {
    case static_cast<int>(LayoutMode::Stereo):          return LayoutMode::Stereo;
    case static_cast<int>(LayoutMode::FivePointOne):    return LayoutMode::FivePointOne;
    case static_cast<int>(LayoutMode::SevenPointOne):   return LayoutMode::SevenPointOne;
    case static_cast<int>(LayoutMode::FivePointOneTwo): return LayoutMode::FivePointOneTwo;
    case static_cast<int>(LayoutMode::FivePointOneFour):return LayoutMode::FivePointOneFour;
    case static_cast<int>(LayoutMode::SevenPointOneFour):return LayoutMode::SevenPointOneFour;
    default: return LayoutMode::Auto;
    }
}

static SampleRateMode sample_rate_mode_from_int(int value) {
    switch (value) {
    case static_cast<int>(SampleRateMode::SourceIfSupported): return SampleRateMode::SourceIfSupported;
    case static_cast<int>(SampleRateMode::Fixed44100):  return SampleRateMode::Fixed44100;
    case static_cast<int>(SampleRateMode::Fixed48000):  return SampleRateMode::Fixed48000;
    case static_cast<int>(SampleRateMode::Fixed88200):  return SampleRateMode::Fixed88200;
    case static_cast<int>(SampleRateMode::Fixed96000):  return SampleRateMode::Fixed96000;
    case static_cast<int>(SampleRateMode::Fixed176400): return SampleRateMode::Fixed176400;
    case static_cast<int>(SampleRateMode::Fixed192000): return SampleRateMode::Fixed192000;
    default: return SampleRateMode::AutoHighest;
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

OutputConfig DefaultConfig() {
    return {};
}

OutputConfig ReadConfig() {
    OutputConfig config;
    config.layoutMode    = layout_from_int(static_cast<int>(cfg_layout_mode.get()));
    config.sampleRateMode= sample_rate_mode_from_int(static_cast<int>(cfg_sample_rate_mode.get()));
    config.directionalTestEnabled          = cfg_directional_test_enabled.get();
    config.directionalTestUseDynamicObject = cfg_directional_test_dynamic.get();
    config.directionalTestTarget           = static_cast<int>(cfg_directional_test_target.get());
    config.directionalTestGainDb           = cfg_directional_test_gain.get();
    config.directionalTestFrequencyHz      = cfg_directional_test_frequency.get();
    return config;
}

void WriteConfig(const OutputConfig& config) {
    cfg_layout_mode      = static_cast<int>(config.layoutMode);
    cfg_sample_rate_mode = static_cast<int>(config.sampleRateMode);
    cfg_directional_test_enabled          = config.directionalTestEnabled;
    cfg_directional_test_dynamic          = config.directionalTestUseDynamicObject;
    cfg_directional_test_target           = config.directionalTestTarget;
    cfg_directional_test_gain             = static_cast<float>(config.directionalTestGainDb);
    cfg_directional_test_frequency        = static_cast<float>(config.directionalTestFrequencyHz);
}

std::string SerializeConfig(const OutputConfig& config) {
    std::ostringstream output;
    output << std::setprecision(10);
    output << "[foo_out_spatial_audio]\n";
    output << "version=1\n";
    output << "layout_mode=" << static_cast<int>(config.layoutMode) << "\n";
    output << "sample_rate_mode=" << static_cast<int>(config.sampleRateMode) << "\n";
    output << "directional_test_enabled=" << (config.directionalTestEnabled ? 1 : 0) << "\n";
    output << "directional_test_dynamic=" << (config.directionalTestUseDynamicObject ? 1 : 0) << "\n";
    output << "directional_test_target=" << config.directionalTestTarget << "\n";
    output << "directional_test_gain_db=" << config.directionalTestGainDb << "\n";
    output << "directional_test_frequency_hz=" << config.directionalTestFrequencyHz << "\n";
    return output.str();
}

bool DeserializeConfig(const std::string& text, OutputConfig& config) {
    const auto values = parse_profile_values(text);
    if (values.empty()) return false;
    const bool hasKnownKey = values.find("version") != values.end()
        || values.find("layout_mode") != values.end()
        || values.find("sample_rate_mode") != values.end();
    if (!hasKnownKey) return false;

    OutputConfig next = config;
    int intValue = 0;
    if (read_int_key(values, "layout_mode", intValue)) next.layoutMode = layout_from_int(intValue);
    if (read_int_key(values, "sample_rate_mode", intValue)) next.sampleRateMode = sample_rate_mode_from_int(intValue);
    read_bool_key(values, "directional_test_enabled", next.directionalTestEnabled);
    read_bool_key(values, "directional_test_dynamic", next.directionalTestUseDynamicObject);
    read_int_key(values, "directional_test_target", next.directionalTestTarget);
    read_double_key(values, "directional_test_gain_db", next.directionalTestGainDb);
    read_double_key(values, "directional_test_frequency_hz", next.directionalTestFrequencyHz);

    config = next;
    return true;
}

}  // namespace spatial_audio
