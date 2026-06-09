#include "stdafx.h"
#include "component_config.h"

namespace spatial_atmos {
namespace {

static constexpr GUID guid_preferences = { 0x9a26d4a8, 0x2f0b, 0x47b6, { 0xb5, 0x8d, 0xa0, 0x3c, 0x36, 0x26, 0x8f, 0x91 } };

enum ControlId {
    idMasterGain = 1001,
    idCenterGain,
    idSurroundGain,
    idRearGain,
    idHeightGain,
    idSideAmount,
    idHeightFromMid,
    idEnableLfe,
    idLfeGain,
    idLfeLowpass,
};

double read_double(HWND wnd, int id, double fallback) {
    wchar_t buffer[64] = {};
    GetDlgItemTextW(wnd, id, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    const double value = wcstod(buffer, &end);
    return end != buffer ? value : fallback;
}

void set_double(HWND wnd, int id, double value) {
    wchar_t buffer[64] = {};
    swprintf_s(buffer, L"%.2f", value);
    SetDlgItemTextW(wnd, id, buffer);
}

void create_label(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
    CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y + 3, w, h, parent, nullptr, core_api::get_my_instance(), nullptr);
}

HWND create_edit(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr);
}

class preferences_instance : public preferences_page_instance {
public:
    preferences_instance(HWND parent, preferences_page_callback::ptr callback) : callback_(callback), initial_(ReadConfig()) {
        register_class();
        wnd_ = CreateWindowExW(0, class_name(), L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 520, 330, parent, nullptr, core_api::get_my_instance(), this);
        populate();
    }

    ~preferences_instance() override {
        if (wnd_ != nullptr && IsWindow(wnd_)) {
            DestroyWindow(wnd_);
        }
    }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable;
        if (has_changed()) {
            state |= preferences_state::changed | preferences_state::needs_restart_playback;
        }
        return state;
    }

    fb2k::hwnd_t get_wnd() override {
        return wnd_;
    }

    void apply() override {
        WriteConfig(read_from_controls());
        initial_ = ReadConfig();
        callback_->on_state_changed();
    }

    void reset() override {
        write_to_controls(DefaultConfig());
        callback_->on_state_changed();
    }

private:
    static const wchar_t* class_name() {
        return L"foo_out_spatial_atmos_preferences";
    }

    static void register_class() {
        static bool registered = false;
        if (registered) {
            return;
        }

        WNDCLASSW wc = {};
        wc.lpfnWndProc = window_proc;
        wc.hInstance = core_api::get_my_instance();
        wc.lpszClassName = class_name();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassW(&wc);
        registered = true;
    }

    static LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
        preferences_instance* self = reinterpret_cast<preferences_instance*>(GetWindowLongPtrW(wnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<preferences_instance*>(create->lpCreateParams);
            SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }

        if (self != nullptr && msg == WM_COMMAND) {
            const WORD code = HIWORD(wp);
            if (code == EN_CHANGE || code == BN_CLICKED) {
                self->callback_->on_state_changed();
            }
        }

        return DefWindowProcW(wnd, msg, wp, lp);
    }

    void populate() {
        create_label(wnd_, L"Master gain (dB)", 12, 14, 170, 24);
        create_edit(wnd_, idMasterGain, 210, 12, 80, 24);
        create_label(wnd_, L"Center gain (dB)", 12, 44, 170, 24);
        create_edit(wnd_, idCenterGain, 210, 42, 80, 24);
        create_label(wnd_, L"Surround gain (dB)", 12, 74, 170, 24);
        create_edit(wnd_, idSurroundGain, 210, 72, 80, 24);
        create_label(wnd_, L"Rear gain (dB)", 12, 104, 170, 24);
        create_edit(wnd_, idRearGain, 210, 102, 80, 24);
        create_label(wnd_, L"Height gain (dB)", 12, 134, 170, 24);
        create_edit(wnd_, idHeightGain, 210, 132, 80, 24);
        create_label(wnd_, L"Side amount", 12, 164, 170, 24);
        create_edit(wnd_, idSideAmount, 210, 162, 80, 24);
        create_label(wnd_, L"Height from mid", 12, 194, 170, 24);
        create_edit(wnd_, idHeightFromMid, 210, 192, 80, 24);

        CreateWindowExW(0, L"BUTTON", L"Enable LFE extraction", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 12, 226, 220, 24, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(idEnableLfe)), core_api::get_my_instance(), nullptr);
        create_label(wnd_, L"LFE gain (dB)", 12, 260, 170, 24);
        create_edit(wnd_, idLfeGain, 210, 258, 80, 24);
        create_label(wnd_, L"LFE low-pass (Hz)", 12, 290, 170, 24);
        create_edit(wnd_, idLfeLowpass, 210, 288, 80, 24);

        write_to_controls(initial_);
    }

    RuntimeConfig read_from_controls() const {
        RuntimeConfig config;
        config.masterGainDb = read_double(wnd_, idMasterGain, config.masterGainDb);
        config.centerGainDb = read_double(wnd_, idCenterGain, config.centerGainDb);
        config.surroundGainDb = read_double(wnd_, idSurroundGain, config.surroundGainDb);
        config.rearGainDb = read_double(wnd_, idRearGain, config.rearGainDb);
        config.heightGainDb = read_double(wnd_, idHeightGain, config.heightGainDb);
        config.sideAmount = read_double(wnd_, idSideAmount, config.sideAmount);
        config.heightFromMid = read_double(wnd_, idHeightFromMid, config.heightFromMid);
        config.enableLfe = Button_GetCheck(GetDlgItem(wnd_, idEnableLfe)) == BST_CHECKED;
        config.lfeGainDb = read_double(wnd_, idLfeGain, config.lfeGainDb);
        config.lfeLowpassHz = read_double(wnd_, idLfeLowpass, config.lfeLowpassHz);
        return config;
    }

    void write_to_controls(const RuntimeConfig& config) {
        set_double(wnd_, idMasterGain, config.masterGainDb);
        set_double(wnd_, idCenterGain, config.centerGainDb);
        set_double(wnd_, idSurroundGain, config.surroundGainDb);
        set_double(wnd_, idRearGain, config.rearGainDb);
        set_double(wnd_, idHeightGain, config.heightGainDb);
        set_double(wnd_, idSideAmount, config.sideAmount);
        set_double(wnd_, idHeightFromMid, config.heightFromMid);
        Button_SetCheck(GetDlgItem(wnd_, idEnableLfe), config.enableLfe ? BST_CHECKED : BST_UNCHECKED);
        set_double(wnd_, idLfeGain, config.lfeGainDb);
        set_double(wnd_, idLfeLowpass, config.lfeLowpassHz);
    }

    static bool different(double a, double b) {
        return std::fabs(a - b) > 0.0001;
    }

    bool has_changed() const {
        const RuntimeConfig current = read_from_controls();
        return different(current.masterGainDb, initial_.masterGainDb)
            || different(current.centerGainDb, initial_.centerGainDb)
            || different(current.surroundGainDb, initial_.surroundGainDb)
            || different(current.rearGainDb, initial_.rearGainDb)
            || different(current.heightGainDb, initial_.heightGainDb)
            || different(current.sideAmount, initial_.sideAmount)
            || different(current.heightFromMid, initial_.heightFromMid)
            || current.enableLfe != initial_.enableLfe
            || different(current.lfeGainDb, initial_.lfeGainDb)
            || different(current.lfeLowpassHz, initial_.lfeLowpassHz);
    }

    HWND wnd_ = nullptr;
    preferences_page_callback::ptr callback_;
    RuntimeConfig initial_;
};

class preferences_page_spatial_atmos : public preferences_page_v3 {
public:
    const char* get_name() override {
        return "Spatial Atmos";
    }

    GUID get_guid() override {
        return guid_preferences;
    }

    GUID get_parent_guid() override {
        return preferences_page::guid_output;
    }

    preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) override {
        return fb2k::service_new<preferences_instance>(parent, callback);
    }
};

static preferences_page_factory_t<preferences_page_spatial_atmos> g_preferences_page_factory;

}  // namespace
}  // namespace spatial_atmos
