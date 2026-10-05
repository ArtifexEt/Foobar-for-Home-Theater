#include "../components/foo_out_spatial_audio/component_config.cpp"
#include <iostream>
#include <limits>

namespace {
using namespace spatial_audio;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void check_position(TopMiddlePosition actual, TopMiddlePosition expected) {
    check(std::fabs(actual.halfWidth - expected.halfWidth) < 0.000001, "half width must roundtrip");
    check(std::fabs(actual.height - expected.height) < 0.000001, "height must roundtrip");
    check(std::fabs(actual.frontBack - expected.frontBack) < 0.000001, "front/back must roundtrip");
}
}

int main() {
    try {
        for (int id = 0; id <= 9; ++id) {
            auto config = DefaultConfig();
            const std::string old = "[foo_out_spatial_audio]\nversion=1\nlayout_mode=" + std::to_string(id)
                + "\nsample_rate_mode=3\ndirectional_test_enabled=1\ndirectional_test_target=13\n";
            check(DeserializeConfig(old, config), "legacy output profile must parse");
            check(static_cast<int>(config.layoutMode) == id, "legacy output layout IDs must remain stable");
            check(config.sampleRateMode == SampleRateMode::Fixed48000, "legacy rate preserved");
            check(config.directionalTestTarget == target_front_wide_right, "legacy wide test target preserved");
            check(!config.directionalTestEnabled, "legacy profile cannot enable continuous test tone");
            check_position(config.topMiddlePosition, {});
        }
        for (const auto layout : {LayoutMode::SevenPointOneSix, LayoutMode::FivePointOneSix, LayoutMode::NinePointOneSix}) {
            auto config = DefaultConfig();
            config.layoutMode = layout;
            config.topMiddlePosition = {1.25, 2.5, -0.75};
            config.directionalTestEnabled = true;
            config.directionalTestTarget = target_top_middle_right;
            auto parsed = DefaultConfig();
            check(DeserializeConfig(SerializeConfig(config), parsed), "six-height output profile parses");
            check(parsed.layoutMode == layout, "six-height output layout roundtrip");
            check(parsed.directionalTestTarget == target_top_middle_right, "middle test target roundtrip");
            check_position(parsed.topMiddlePosition, config.topMiddlePosition);
            check(!parsed.directionalTestEnabled, "profile roundtrip disables continuous test tone");
            WriteConfig(config);
            const auto stored = ReadConfig();
            check(stored.layoutMode == layout, "six-height output layout persists");
            check_position(stored.topMiddlePosition, config.topMiddlePosition);
            check(!stored.directionalTestEnabled, "persisted config cannot enable continuous test tone");
        }
        auto invalid = DefaultConfig();
        invalid.topMiddlePosition = {-50, 50, std::numeric_limits<double>::quiet_NaN()};
        WriteConfig(invalid);
        check_position(ReadConfig().topMiddlePosition, {0.1, 10, 0});
        check(DeserializeConfig("version=1\nlayout_mode=12\ntop_middle_half_width=nan\ntop_middle_height=inf\ntop_middle_front_back=-50\n", invalid), "profile with invalid position can be sanitized");
        check_position(invalid.topMiddlePosition, {0.8, 1.4, -10});
        std::cout << "PASS output_config_compatibility\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
