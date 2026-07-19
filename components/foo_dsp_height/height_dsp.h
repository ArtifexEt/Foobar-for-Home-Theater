#pragma once
#include "stdafx.h"

namespace spatial_audio {

enum class HeightLayout : uint32_t { Two = 2, Four = 4 };

struct HeightDspConfig {
    HeightLayout layout = HeightLayout::Four;
    double heightGainDb = -10.0;
    double frontDifference = 0.35;
    double surroundFeed = 0.45;
    double midFeed = 0.08;
};

class height_only_dsp : public dsp_impl_base {
public:
    explicit height_only_dsp(const dsp_preset& preset);

    static GUID g_get_guid();
    static void g_get_name(pfc::string_base& out);
    static bool g_get_default_preset(dsp_preset& out);
    static bool g_have_config_popup();
    static void g_show_config_popup(const dsp_preset& data, HWND parent, dsp_preset_edit_callback& callback);

    bool on_chunk(audio_chunk* chunk, abort_callback&) override;
    void on_endoftrack(abort_callback&) override {}
    void on_endofplayback(abort_callback&) override {}
    void flush() override {}
    double get_latency() override { return 0.0; }
    bool need_track_change_mark() override { return false; }

private:
    HeightDspConfig config_;
};

} // namespace spatial_audio
