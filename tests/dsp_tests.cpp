// Execute the production processing and configuration code. Only the SDK/GUI
// boundary is adapted; no DSP algorithm is duplicated in the test adapter.
#include "../components/foo_dsp_spatial/dsp_config.cpp"
#include "../components/foo_dsp_spatial/spatial_dsp.cpp"
#include "../components/foo_dsp_height/height_dsp.cpp"
#include "../components/shared/spatial_channels.h"
#include "../components/foo_out_spatial_audio/output_routing.h"
#include <functional>
#include <iostream>
#include <limits>

namespace {
using namespace spatial_audio;
constexpr unsigned middle = spatial_channels::top_middle_pair;
constexpr unsigned front_height = audio_chunk::channel_top_front_left | audio_chunk::channel_top_front_right;
constexpr unsigned rear_height = audio_chunk::channel_top_back_left | audio_chunk::channel_top_back_right;
constexpr unsigned four_height = front_height | rear_height;
constexpr unsigned mask714 = audio_chunk::channel_config_7point1 | four_height;
constexpr unsigned mask716 = mask714 | middle;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void near(double actual, double expected, const char* message, double tolerance = 0.000001) {
    if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << message << ": actual=" << actual << " expected=" << expected;
        throw std::runtime_error(out.str());
    }
}
audio_chunk make_chunk(unsigned mask, size_t frames = 4) {
    const unsigned channels = spatial_channels::count(mask);
    std::vector<audio_sample> data(channels * frames);
    for (size_t f = 0; f < frames; ++f) {
        for (unsigned c = 0; c < channels; ++c) {
            data[f * channels + c] = static_cast<float>((c + 1) * 0.01 + f * 0.001);
        }
    }
    audio_chunk chunk;
    chunk.set_data(data.data(), frames, channels, 48000, mask);
    return chunk;
}
float sample(const audio_chunk& chunk, unsigned flag, size_t frame = 0) {
    const unsigned index = spatial_channels::index(chunk.get_channel_config(), flag);
    require(index < chunk.get_channel_count(), "requested sample must exist");
    return chunk.get_data()[frame * chunk.get_channel_count() + index];
}
void unchanged_channels(const audio_chunk& before, const audio_chunk& after) {
    require(before.get_sample_count() == after.get_sample_count(), "frame count preserved");
    require(before.get_sample_rate() == after.get_sample_rate(), "sample rate preserved");
    for (unsigned c = 0; c < before.get_channel_count(); ++c) {
        const unsigned flag = spatial_channels::flag_at(before.get_channel_config(), c);
        for (size_t f = 0; f < before.get_sample_count(); ++f) {
            const float a = sample(before, flag, f), b = sample(after, flag, f);
            require(std::memcmp(&a, &b, sizeof(float)) == 0, "existing channel samples preserved bit for bit");
        }
    }
}
DspConfig neutral_config(DspOutputLayout layout) {
    DspConfig config = DefaultDspConfig();
    config.outputLayout = layout;
    config.upmixMode = UpmixMode::Full;
    config.limiterEnabled = false;
    return config;
}
audio_chunk spatial_process(audio_chunk input, const DspConfig& config) {
    WriteDspConfig(config);
    spatial_upmix_dsp dsp;
    abort_callback abort;
    require(dsp.on_chunk(&input, abort), "spatial processing succeeds");
    return input;
}
audio_chunk height_process(audio_chunk input, const HeightDspConfig& config) {
    dsp_preset_impl preset;
    make_preset(config, preset);
    height_only_dsp dsp(preset);
    abort_callback abort;
    require(dsp.on_chunk(&input, abort), "height processing succeeds");
    return input;
}

void channel_transport() {
    constexpr unsigned mask914 = mask714 | audio_chunk::channel_front_center_left | audio_chunk::channel_front_center_right;
    require(spatial_channels::count(mask716) == 14 && spatial_channels::count(mask914) == 14, "both layouts carry fourteen channels");
    require(mask716 != mask914, "fourteen channels must not imply one fixed layout");
    require(spatial_channels::index(mask716, spatial_channels::top_middle_left) == 12, "TML appended after static bed");
    require(spatial_channels::index(mask716, spatial_channels::top_middle_right) == 13, "TMR appended after static bed");
    require(spatial_channels::index(mask914, spatial_channels::top_middle_left) == ~0u, "wide layout cannot alias TML");
    require(spatial_channels::index(mask716, audio_chunk::channel_front_center_left) == ~0u, "middle layout cannot alias wide left");
    for (unsigned mask : {mask716, mask914, mask716 | audio_chunk::channel_front_center_left}) {
        for (unsigned i = 0; i < spatial_channels::count(mask); ++i) {
            require(spatial_channels::index(mask, spatial_channels::flag_at(mask, i)) == i, "channel mask index roundtrip");
        }
    }
    const auto spatial = spatial_process(make_chunk(audio_chunk::channel_config_stereo), neutral_config(DspOutputLayout::SevenPointOneSix));
    const auto wide = spatial_process(make_chunk(audio_chunk::channel_config_stereo), neutral_config(DspOutputLayout::NinePointOneFour));
    require(spatial.get_channel_config() == mask716 && wide.get_channel_config() == mask914, "production DSP emits distinct 7.1.6 and 9.1.4 transport masks");
}

void output_routing() {
    require(dynamic_channel_mask(LayoutMode::Auto, mask714) == 0, "7.1.4 Auto needs no dynamic objects");
    require(dynamic_channel_mask(LayoutMode::Auto, mask716) == middle, "7.1.6 Auto requests middle pair");
    require(dynamic_channel_mask(LayoutMode::Auto, mask714 | front_wide_pair) == front_wide_pair, "9.1.4 Auto requests only wides");
    require(dynamic_channel_mask(LayoutMode::Auto, mask716 | front_wide_pair) == (middle | front_wide_pair), "9.1.6 Auto requests four dynamic channels");
    require(spatial_channels::count(dynamic_channel_mask(LayoutMode::FivePointOneSix, 0)) == 2, "fixed5.1.6 requests two objects");
    require(spatial_channels::count(dynamic_channel_mask(LayoutMode::SevenPointOneSix, 0)) == 2, "fixed7.1.6 requests two objects");
    require(spatial_channels::count(dynamic_channel_mask(LayoutMode::NinePointOneSix, 0)) == 4, "fixed9.1.6 requests four objects");
    require(dynamic_channel_mask(LayoutMode::Auto, spatial_channels::top_middle_left) == spatial_channels::top_middle_left, "Auto respects actual single private channel");
    require(dynamic_channel_mask(LayoutMode::SevenPointOneFour, mask716) == 0, "fixed7.1.4 never invents middle objects");
    require(dynamic_input_status(LayoutMode::SevenPointOneSix, mask714) == DynamicInputStatus::MissingTopMiddle, "fixed .6 rejects missing middle PCM");
    require(dynamic_input_status(LayoutMode::Auto, mask714 | spatial_channels::top_middle_left) == DynamicInputStatus::MissingTopMiddle, "Auto rejects incomplete middle pair");
    require(dynamic_input_status(LayoutMode::NinePointOneSix, mask716) == DynamicInputStatus::MissingFrontWide, "9.1.6 rejects missing wide PCM");
    require(dynamic_input_status(LayoutMode::NinePointOneSix, mask716 | front_wide_pair) == DynamicInputStatus::Ready, "complete9.1.6 can activate dynamic channels");
    require(dynamic_input_status(LayoutMode::Auto, mask716) == DynamicInputStatus::Ready, "complete7.1.6 can activate dynamic channels");
    const auto defaults = sanitize_position({});
    near(defaults.halfWidth, 0.8, "default width"); near(defaults.height, 1.4, "default height"); near(defaults.frontBack, 0, "default plane");
    const auto finite = sanitize_position({-1, 100, -100});
    near(finite.halfWidth, 0.1, "width bounded away from center"); near(finite.height, 10, "height bounded"); near(finite.frontBack, -10, "front/back bounded");
    const auto invalid = sanitize_position({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()});
    near(invalid.halfWidth, defaults.halfWidth, "invalid width defaults"); near(invalid.height, defaults.height, "invalid height defaults"); near(invalid.frontBack, defaults.frontBack, "invalid depth defaults");
}

void config_compatibility() {
    require(static_cast<int>(DspOutputLayout::SevenPointOneSix) == 8, "new layout appended");
    require(target_front_wide_left == 12 && target_front_wide_right == 13, "old wide target IDs remain stable");
    require(target_top_middle_left == 14 && target_top_middle_right == 15, "middle target IDs appended");
    for (int layout = 0; layout <= 7; ++layout) {
        auto config = DefaultDspConfig();
        const std::string old = "[foo_dsp_spatial]\nversion=1\noutput_layout=" + std::to_string(layout)
            + "\nchannel_gain_front_wide_left=-3.5\nchannel_delay_top_back_right_ms=4.25\nchannel_invert_front_wide_right=1\nmap51_surround_left=12\n";
        require(DeserializeDspConfig(old, config), "old profile parses");
        require(static_cast<int>(config.outputLayout) == layout, "old layout number preserved");
        near(config.channelGainDb[12], -3.5, "old wide gain preserved");
        near(config.channelDelayMs[11], 4.25, "old rear height delay preserved");
        require(config.channelInvert[13] && config.map51SurroundLeft == 12, "old wide routing preserved");
        near(config.channelGainDb[14], 0, "old profile gets unity middle gain");
        near(config.channelDelayMs[15], 0, "old profile gets zero middle delay");
    }
    auto config = neutral_config(DspOutputLayout::SevenPointOneSix);
    config.channelGainDb[14] = -7.5; config.channelGainDb[15] = 2.5;
    config.channelDelayMs[14] = 3.25; config.channelInvert[15] = true;
    config.map51SurroundLeft = 14;
    auto decoded = DefaultDspConfig();
    require(DeserializeDspConfig(SerializeDspConfig(config), decoded), "7.1.6 profile parses");
    require(decoded.outputLayout == config.outputLayout, "7.1.6 layout roundtrip");
    require(decoded.channelGainDb == config.channelGainDb && decoded.channelDelayMs == config.channelDelayMs && decoded.channelInvert == config.channelInvert, "sixteen channel controls roundtrip");
    require(decoded.map51SurroundLeft == 14, "middle channel mapping roundtrip");
    WriteDspConfig(decoded);
    const auto stored = ReadDspConfig();
    require(stored.channelGainDb == decoded.channelGainDb && stored.channelDelayMs == decoded.channelDelayMs && stored.channelInvert == decoded.channelInvert, "new persistent settings read back");
    for (auto layout : {DspOutputLayout::FivePointOneSix, DspOutputLayout::NinePointOneSix}) {
        config.outputLayout = layout;
        require(DeserializeDspConfig(SerializeDspConfig(config), decoded), "additional .6 layout parses");
        require(decoded.outputLayout == layout, "additional .6 layout survives serialization");
        WriteDspConfig(decoded);
        require(ReadDspConfig().outputLayout == layout, "additional .6 layout persists");
    }
}

void spatial_synthesis() {
    const auto config = neutral_config(DspOutputLayout::SevenPointOneSix);
    for (unsigned inputMask : {audio_chunk::channel_config_stereo, audio_chunk::channel_config_5point1_side, audio_chunk::channel_config_5point1, audio_chunk::channel_config_7point1}) {
        const auto output = spatial_process(make_chunk(inputMask), config);
        require(output.get_channel_config() == mask716 && output.get_channel_count() == 14, "7.1.6 channel count and mask");
        for (unsigned flag : std::initializer_list<unsigned>{audio_chunk::channel_top_front_left, audio_chunk::channel_top_front_right, audio_chunk::channel_top_back_left, audio_chunk::channel_top_back_right, spatial_channels::top_middle_left, spatial_channels::top_middle_right}) {
            require(std::isfinite(sample(output, flag)) && sample(output, flag) != 0.0f, "all six generated height channels contain audio");
        }
        require(sample(output, spatial_channels::top_middle_left) != sample(output, spatial_channels::top_middle_right), "left and right middle signals are independent");
    }
    auto mapped = config;
    mapped.map51SurroundLeft = target_top_middle_left;
    const auto input51 = make_chunk(audio_chunk::channel_config_5point1_side);
    const auto mappedOutput = spatial_process(input51, mapped);
    near(sample(mappedOutput, spatial_channels::top_middle_left), sample(input51, audio_chunk::channel_side_left), "explicit 5.1 middle mapping takes priority");

    const auto legacy = spatial_process(make_chunk(audio_chunk::channel_config_7point1), neutral_config(DspOutputLayout::SevenPointOneFour));
    require(legacy.get_channel_config() == mask714, "legacy 7.1.4 mask unchanged");
    near(sample(legacy, audio_chunk::channel_top_front_left), 0, "legacy multichannel height behavior unchanged");

    const std::array<std::pair<DspOutputLayout, unsigned>, 3> layouts = {{
        {DspOutputLayout::FivePointOneSix, audio_chunk::channel_config_5point1_side | four_height | middle},
        {DspOutputLayout::SevenPointOneSix, mask716},
        {DspOutputLayout::NinePointOneSix, mask716 | front_wide_pair},
    }};
    for (const auto& [layout, mask] : layouts) {
        const auto output = spatial_process(make_chunk(audio_chunk::channel_config_stereo), neutral_config(layout));
        require(output.get_channel_config() == mask, "selected .6 layout emits exact mask");
        require(output.get_channel_count() == spatial_channels::count(mask), "selected .6 layout emits correct12/14/16 channel count");
        require(sample(output, spatial_channels::top_middle_left) != 0 && sample(output, spatial_channels::top_middle_right) != 0, "each .6 layout has middle audio");
    }
}

void spatial_preservation() {
    const auto config = neutral_config(DspOutputLayout::SevenPointOneSix);
    for (unsigned mask : {mask714, mask716}) {
        const auto input = make_chunk(mask);
        const auto output = spatial_process(input, config);
        unchanged_channels(input, output);
        require(output.get_channel_config() == mask716, "existing height layout expands to 7.1.6");
    }
    for (const auto& [layout, mask] : std::array<std::pair<DspOutputLayout, unsigned>, 2>{{
        {DspOutputLayout::FivePointOneSix, audio_chunk::channel_config_5point1_side | four_height | middle},
        {DspOutputLayout::NinePointOneSix, mask716 | front_wide_pair},
    }}) {
        const auto input = make_chunk(mask);
        unchanged_channels(input, spatial_process(input, neutral_config(layout)));
    }
    auto input = make_chunk(mask714, 1);
    std::vector<float> values(input.get_data(), input.get_data() + input.get_channel_count());
    values[spatial_channels::index(mask714, audio_chunk::channel_top_front_left)] = 0;
    input.set_data(values.data(), 1, input.get_channel_count(), 48000, mask714);
    const auto baseline = spatial_process(input, config);
    auto louderConfig = config; louderConfig.heightGainDb = 6.0;
    const auto louder = spatial_process(input, louderConfig);
    unchanged_channels(input, louder);
    near(sample(louder, audio_chunk::channel_top_front_left), 0, "present silent height not synthesized over");
    near(sample(louder, spatial_channels::top_middle_left), sample(baseline, spatial_channels::top_middle_left) * std::pow(10.0, 6.0 / 20.0), "generation gain affects new middle derived from native heights");

    // With no usable channel mask, a fourteen-channel chunk cannot safely be
    // identified as either7.1.6 or9.1.4. The DSP must not collapse it to stereo.
    std::vector<float> unknownSamples(14, 0.125f);
    audio_chunk unknown;
    unknown.set_data(unknownSamples.data(), 1, 14, 48000, 0);
    const auto unknownOutput = spatial_process(unknown, config);
    require(unknownOutput.get_channel_count() == 14 && unknownOutput.get_channel_config() == 0, "unknown multichannel layout passes through");
    require(std::memcmp(unknownOutput.get_data(), unknownSamples.data(), unknownSamples.size() * sizeof(float)) == 0, "unknown samples pass through unchanged");
}

void spatial_controls() {
    const auto input = make_chunk(audio_chunk::channel_config_stereo);
    auto config = neutral_config(DspOutputLayout::SevenPointOneSix);
    const auto baseline = spatial_process(input, config);
    config.channelGainDb[14] = -6.0;
    config.channelInvert[15] = true;
    const auto changed = spatial_process(input, config);
    near(sample(changed, spatial_channels::top_middle_left), sample(baseline, spatial_channels::top_middle_left) * std::pow(10.0, -6.0 / 20.0), "middle left gain applied");
    near(sample(changed, spatial_channels::top_middle_right), -sample(baseline, spatial_channels::top_middle_right), "middle right polarity applied");
    near(sample(changed, audio_chunk::channel_top_front_left), sample(baseline, audio_chunk::channel_top_front_left), "middle trim does not change front height");
    config.channelDelayMs[14] = 10.0;
    const auto delayed = spatial_process(input, config);
    near(sample(delayed, spatial_channels::top_middle_left), 0, "middle delay starts with silence");
    config.upmixMode = UpmixMode::FrontOnly;
    const auto frontOnly = spatial_process(input, config);
    near(sample(frontOnly, spatial_channels::top_middle_right), 0, "Front only suppresses middle synthesis");
}

void height_compatibility() {
    for (int count : {2, 4}) {
        dsp_preset_impl old;
        old.set_owner(height_only_dsp::g_get_guid());
        const std::string text = "height_dsp_version=1\nlayout=" + std::to_string(count) + "\nheight_gain_db=-12.5\nfront_difference=0.2\n";
        old.set_data(text.data(), text.size());
        const auto parsed = parse_config(old);
        require(static_cast<int>(parsed.layout) == count, "old height layout preserved");
        near(parsed.heightGainDb, -12.5, "old height gain preserved");
        near(parsed.topMiddleGainDb, 0, "old height preset gets unity middle trim");
    }
    HeightDspConfig config;
    config.layout = HeightLayout::Six; config.topMiddleGainDb = -7.25;
    dsp_preset_impl encoded;
    make_preset(config, encoded);
    const auto decoded = parse_config(encoded);
    require(decoded.layout == HeightLayout::Six, "six ceiling layout roundtrip");
    near(decoded.topMiddleGainDb, config.topMiddleGainDb, "middle trim roundtrip");
    dsp_preset_impl defaultPreset;
    height_only_dsp::g_get_default_preset(defaultPreset);
    require(parse_config(defaultPreset).layout == HeightLayout::Four, "default remains four ceiling speakers");
}

void height_preservation() {
    HeightDspConfig config; config.layout = HeightLayout::Six;
    for (unsigned mask : std::initializer_list<unsigned>{audio_chunk::channel_config_mono, audio_chunk::channel_config_stereo, audio_chunk::channel_config_5point1_side, audio_chunk::channel_config_5point1, audio_chunk::channel_config_7point1, audio_chunk::channel_config_7point1 | front_wide_pair, mask714, mask716, mask714 | spatial_channels::top_middle_left}) {
        const auto input = make_chunk(mask);
        const auto output = height_process(input, config);
        require(output.get_channel_config() == (mask | four_height | middle), "height-only unions transport masks");
        unchanged_channels(input, output);
    }
    // Signed zero is an observable bit pattern even though numeric equality
    // would accept an accidental conversion to positive zero.
    auto input = make_chunk(audio_chunk::channel_config_stereo, 1);
    const audio_sample special[] = {-0.0f, 0.25f};
    input.set_data(special, 1, 2, 48000, audio_chunk::channel_config_stereo);
    unchanged_channels(input, height_process(input, config));
}

void height_synthesis() {
    HeightDspConfig config; config.layout = HeightLayout::Six;
    for (unsigned mask : {audio_chunk::channel_config_mono, audio_chunk::channel_config_stereo, audio_chunk::channel_config_5point1_side, audio_chunk::channel_config_7point1}) {
        const auto output = height_process(make_chunk(mask), config);
        require(sample(output, spatial_channels::top_middle_left) != 0, "height-only generates left middle");
        require(sample(output, spatial_channels::top_middle_right) != 0, "height-only generates right middle");
    }
    const auto input = make_chunk(audio_chunk::channel_config_7point1);
    const auto baseline = height_process(input, config);
    config.topMiddleGainDb = -6.0;
    const auto trimmed = height_process(input, config);
    near(sample(trimmed, spatial_channels::top_middle_left), sample(baseline, spatial_channels::top_middle_left) * std::pow(10.0, -6.0 / 20.0), "height-only middle trim applied");
    near(sample(trimmed, audio_chunk::channel_top_front_left), sample(baseline, audio_chunk::channel_top_front_left), "height-only middle trim isolated");
    config.layout = HeightLayout::Four;
    const auto four = height_process(input, config);
    require((four.get_channel_config() & middle) == 0, "four height mode adds no private middle flags");
    unchanged_channels(four, baseline);
    config.layout = HeightLayout::Two;
    const auto two = height_process(input, config);
    require(two.get_channel_config() == (input.get_channel_config() | front_height), "two height mode remains top front pair only");
    unchanged_channels(two, four);
}
}

int main(int argc, char** argv) {
    const std::map<std::string, std::function<void()>> cases = {
        {"channel_transport", channel_transport}, {"config_compatibility", config_compatibility},
        {"output_routing", output_routing},
        {"spatial_synthesis", spatial_synthesis}, {"spatial_preservation", spatial_preservation},
        {"spatial_controls", spatial_controls}, {"height_compatibility", height_compatibility},
        {"height_preservation", height_preservation}, {"height_synthesis", height_synthesis},
    };
    try {
        for (const auto& [name, run] : cases) {
            if (argc == 1 || name == argv[1]) { run(); std::cout << "PASS " << name << '\n'; }
        }
        if (argc > 1 && !cases.contains(argv[1])) throw std::runtime_error("unknown test case");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
