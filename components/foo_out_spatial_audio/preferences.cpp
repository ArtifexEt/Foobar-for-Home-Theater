#include "stdafx.h"
#include "component_config.h"
#include "component_version.h"
#include "preferences_resource.h"

#include <helpers/atl-misc.h>

using Microsoft::WRL::ComPtr;

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

namespace spatial_audio {
namespace {

static constexpr GUID guid_preferences = { 0x9a26d4a8, 0x2f0b, 0x47b6, { 0xb5, 0x8d, 0xa0, 0x3c, 0x36, 0x26, 0x8f, 0x91 } };
static constexpr double kPi = 3.14159265358979323846;
static constexpr COLORREF kDarkBackground    = RGB(32, 32, 32);
static constexpr COLORREF kDarkEditBackground= RGB(24, 24, 24);
static constexpr COLORREF kDarkText          = RGB(232, 232, 232);
static constexpr int kTooltipMaxWidthPixels  = 360;

enum class Page {
    Layout = 0,
    Test,
    About,
    Count,
};

struct TargetDef {
    int target;
    const char* key;
    const wchar_t* label;
    AudioObjectType type;
    double frequencyHz;
    float x; float y; float z;
};

const TargetDef kTargets[] = {
    {target_front_left,      "front_left",      L"Front left",       AudioObjectType_FrontLeft,      220.0, -1.0f,  0.0f, -1.2f},
    {target_front_right,     "front_right",     L"Front right",      AudioObjectType_FrontRight,     247.0,  1.0f,  0.0f, -1.2f},
    {target_front_center,    "front_center",    L"Front center",     AudioObjectType_FrontCenter,    277.0,  0.0f,  0.0f, -1.3f},
    {target_low_frequency,   "low_frequency",   L"LFE",              AudioObjectType_LowFrequency,    55.0,  0.0f, -0.2f, -0.8f},
    {target_side_left,       "side_left",       L"Side left",        AudioObjectType_SideLeft,       311.0, -1.3f,  0.0f,  0.0f},
    {target_side_right,      "side_right",      L"Side right",       AudioObjectType_SideRight,      349.0,  1.3f,  0.0f,  0.0f},
    {target_back_left,       "back_left",       L"Back left",        AudioObjectType_BackLeft,       392.0, -1.0f,  0.0f,  1.1f},
    {target_back_right,      "back_right",      L"Back right",       AudioObjectType_BackRight,      440.0,  1.0f,  0.0f,  1.1f},
    {target_top_front_left,  "top_front_left",  L"Top front left",   AudioObjectType_TopFrontLeft,   523.25,-0.8f,  1.4f, -0.9f},
    {target_top_front_right, "top_front_right", L"Top front right",  AudioObjectType_TopFrontRight,  587.33, 0.8f,  1.4f, -0.9f},
    {target_top_back_left,   "top_back_left",   L"Top back left",    AudioObjectType_TopBackLeft,    659.25,-0.8f,  1.4f,  0.9f},
    {target_top_back_right,  "top_back_right",  L"Top back right",   AudioObjectType_TopBackRight,   739.99, 0.8f,  1.4f,  0.9f},
};

struct LayoutOption { LayoutMode mode; const wchar_t* label; };
struct SampleRateOption { SampleRateMode mode; const wchar_t* label; };

const LayoutOption kLayoutOptions[] = {
    {LayoutMode::Auto,            L"Auto (use all available)"},
    {LayoutMode::Stereo,          L"Stereo (2.0)"},
    {LayoutMode::FivePointOne,    L"Surround (5.1)"},
    {LayoutMode::SevenPointOne,   L"Surround (7.1)"},
    {LayoutMode::FivePointOneTwo, L"Surround + height (5.1.2)"},
    {LayoutMode::FivePointOneFour,L"Surround + height (5.1.4)"},
    {LayoutMode::SevenPointOneFour,L"Surround + height (7.1.4)"},
};

const SampleRateOption kSampleRateOptions[] = {
    {SampleRateMode::AutoHighest,       L"Auto (highest supported)"},
    {SampleRateMode::SourceIfSupported, L"Source rate (if supported)"},
    {SampleRateMode::Fixed44100,        L"44100 Hz"},
    {SampleRateMode::Fixed48000,        L"48000 Hz"},
    {SampleRateMode::Fixed88200,        L"88200 Hz"},
    {SampleRateMode::Fixed96000,        L"96000 Hz"},
    {SampleRateMode::Fixed176400,       L"176400 Hz"},
    {SampleRateMode::Fixed192000,       L"192000 Hz"},
};

static std::string narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string result(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    return result;
}

int combo_get_cur_sel(HWND combo) {
    return combo != nullptr ? static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0)) : CB_ERR;
}

LPARAM combo_get_item_data(HWND combo, int index) {
    return combo != nullptr ? SendMessageW(combo, CB_GETITEMDATA, static_cast<WPARAM>(index), 0) : 0;
}

int combo_get_count(HWND combo) {
    return combo != nullptr ? static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0)) : 0;
}

void combo_set_cur_sel(HWND combo, int index) {
    if (combo != nullptr) SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
}

void add_combo_item(HWND combo, const wchar_t* label, LPARAM data) {
    if (combo == nullptr) return;
    const int index = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label)));
    if (index != CB_ERR && index != CB_ERRSPACE)
        SendMessageW(combo, CB_SETITEMDATA, static_cast<WPARAM>(index), data);
}

struct FindControlData { int id = 0; HWND control = nullptr; };

BOOL CALLBACK find_control_proc(HWND child, LPARAM context) {
    auto* data = reinterpret_cast<FindControlData*>(context);
    if (GetDlgCtrlID(child) == data->id) { data->control = child; return FALSE; }
    return TRUE;
}

HWND find_dlg_item(HWND root, int id) {
    if (root == nullptr) return nullptr;
    HWND direct = GetDlgItem(root, id);
    if (direct != nullptr) return direct;
    FindControlData data = {}; data.id = id;
    EnumChildWindows(root, find_control_proc, reinterpret_cast<LPARAM>(&data));
    return data.control;
}

bool read_check(HWND wnd, int id) {
    HWND button = find_dlg_item(wnd, id);
    return button != nullptr && SendMessageW(button, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void set_check(HWND wnd, int id, bool checked) {
    HWND button = find_dlg_item(wnd, id);
    if (button != nullptr) SendMessageW(button, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

double read_double(HWND wnd, int id, double fallback) {
    wchar_t buffer[64] = {};
    HWND control = find_dlg_item(wnd, id);
    if (control == nullptr) return fallback;
    GetWindowTextW(control, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    const double value = wcstod(buffer, &end);
    return end != buffer ? value : fallback;
}

void set_double_text(HWND wnd, int id, double value, int decimals) {
    wchar_t buffer[64] = {};
    swprintf_s(buffer, decimals <= 0 ? L"%.0f" : decimals == 1 ? L"%.1f" : L"%.2f", value);
    HWND control = find_dlg_item(wnd, id);
    if (control != nullptr) SetWindowTextW(control, buffer);
}

static const CDialogResizeHelper::Param kMainResizeParams[] = {
    {idTabs, 0.f, 0.f, 1.f, 1.f},
};

struct PageEnumData { HWND parent = nullptr; int maxBottom = 0; };

BOOL CALLBACK page_max_bottom_proc(HWND child, LPARAM lp) {
    auto* data = reinterpret_cast<PageEnumData*>(lp);
    if (::GetParent(child) != data->parent) return TRUE;
    RECT r = {}; ::GetWindowRect(child, &r); ::MapWindowPoints(nullptr, data->parent, reinterpret_cast<POINT*>(&r), 2);
    if (r.bottom > data->maxBottom) data->maxBottom = r.bottom;
    return TRUE;
}

static int measure_content_height(HWND pageWnd) {
    PageEnumData data = {pageWnd, 0};
    ::EnumChildWindows(pageWnd, page_max_bottom_proc, reinterpret_cast<LPARAM>(&data));
    return data.maxBottom > 0 ? data.maxBottom + 8 : 0;
}

class preferences_instance : public CDialogImpl<preferences_instance>, public preferences_page_instance {
public:
    enum { IDD = IDD_SPATIAL_AUDIO_PREFERENCES };

    preferences_instance(preferences_page_callback::ptr callback)
        : callback_(callback), initial_(ReadConfig()) {
        m_resizer.m_autoSizeGrip = false;
        INITCOMMONCONTROLSEX cc = {};
        cc.dwSize = sizeof(cc);
        cc.dwICC  = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES;
        InitCommonControlsEx(&cc);
    }

    ~preferences_instance() {
        if (tooltip_ != nullptr && ::IsWindow(tooltip_)) { ::DestroyWindow(tooltip_); tooltip_ = nullptr; }
        DeleteObject(backgroundBrush_);
        DeleteObject(editBrush_);
    }

    BEGIN_MSG_MAP_EX(preferences_instance)
        CHAIN_MSG_MAP_MEMBER(m_resizer)
        MESSAGE_HANDLER(WM_INITDIALOG, on_init_dialog_message)
        MESSAGE_HANDLER(WM_ERASEBKGND, on_erase_message)
        MESSAGE_HANDLER(WM_SIZE, on_size_message)
        MESSAGE_HANDLER(WM_DPICHANGED, on_dpi_changed_message)
        MESSAGE_HANDLER(WM_DPICHANGED_AFTERPARENT, on_dpi_changed_message)
        MESSAGE_HANDLER(WM_THEMECHANGED, on_theme_changed_message)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, on_theme_changed_message)
        MESSAGE_HANDLER(WM_COMMAND, on_command_message)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, on_control_color_message)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, on_control_color_message)
        MESSAGE_HANDLER(WM_CTLCOLORBTN, on_control_color_message)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, on_control_color_message)
        MESSAGE_HANDLER(WM_NOTIFY, on_notify_message)
    END_MSG_MAP()

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (has_changed()) state |= preferences_state::changed;
        return state;
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
    LRESULT on_init_dialog_message(UINT, WPARAM, LPARAM, BOOL&) {
        wnd_ = m_hWnd;
        populate();
        dark_.AddDialogWithControls(m_hWnd);
        return FALSE;
    }
    LRESULT on_erase_message(UINT, WPARAM wp, LPARAM, BOOL&) { return on_erase(m_hWnd, reinterpret_cast<HDC>(wp)); }
    LRESULT on_size_message(UINT, WPARAM, LPARAM, BOOL&) { position_pages(); return TRUE; }
    LRESULT on_dpi_changed_message(UINT, WPARAM, LPARAM, BOOL&) { update_tooltip_width(); position_pages(); return TRUE; }
    LRESULT on_theme_changed_message(UINT, WPARAM, LPARAM, BOOL&) {
        update_tooltip_width(); position_pages();
        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return TRUE;
    }
    LRESULT on_command_message(UINT, WPARAM wp, LPARAM lp, BOOL&) { return on_command(wp, lp); }
    LRESULT on_control_color_message(UINT msg, WPARAM wp, LPARAM lp, BOOL&) {
        return on_control_color(reinterpret_cast<HDC>(wp), reinterpret_cast<HWND>(lp), msg);
    }
    LRESULT on_notify_message(UINT, WPARAM, LPARAM lp, BOOL&) { return on_notify(reinterpret_cast<NMHDR*>(lp)); }

    static INT_PTR CALLBACK page_dialog_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
        preferences_instance* self = reinterpret_cast<preferences_instance*>(::GetWindowLongPtrW(wnd, GWLP_USERDATA));
        if (msg == WM_INITDIALOG) {
            self = reinterpret_cast<preferences_instance*>(lp);
            ::SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            const int ch = measure_content_height(wnd);
            ::SetPropW(wnd, L"spatial_ch", reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(ch)));
            return FALSE;
        }
        if (self == nullptr) return FALSE;
        switch (msg) {
        case WM_ERASEBKGND: return self->on_erase(wnd, reinterpret_cast<HDC>(wp));
        case WM_COMMAND: self->on_command(wp, lp); return TRUE;
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG: return self->on_control_color(reinterpret_cast<HDC>(wp), reinterpret_cast<HWND>(lp), msg);
        case WM_DPICHANGED:
        case WM_DPICHANGED_AFTERPARENT:
            self->update_tooltip_width();
            ::RedrawWindow(wnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            return TRUE;
        case WM_NOTIFY: self->on_notify(reinterpret_cast<NMHDR*>(lp)); return TRUE;
        default: break;
        }
        return FALSE;
    }

    HWND create_page(Page page, int resourceId) {
        HWND pageWnd = CreateDialogParamW(core_api::get_my_instance(), MAKEINTRESOURCEW(resourceId), wnd_, page_dialog_proc, reinterpret_cast<LPARAM>(this));
        if (pageWnd == nullptr) throw std::runtime_error("Could not create output preferences page.");
        pageWnds_[static_cast<size_t>(page)] = pageWnd;
        dark_.AddDialogWithControls(pageWnd);
        return pageWnd;
    }

    void position_pages() {
        HWND tabs = find_dlg_item(wnd_, idTabs);
        if (tabs == nullptr) return;
        RECT tabRect = {}; ::GetWindowRect(tabs, &tabRect);
        ::MapWindowPoints(nullptr, wnd_, reinterpret_cast<POINT*>(&tabRect), 2);
        RECT pageRect = {0, 0, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top};
        TabCtrl_AdjustRect(tabs, FALSE, &pageRect);
        const int x = tabRect.left + pageRect.left, y = tabRect.top + pageRect.top;
        const int width = pageRect.right - pageRect.left, height = pageRect.bottom - pageRect.top;
        for (HWND pageWnd : pageWnds_) {
            if (pageWnd != nullptr && ::IsWindow(pageWnd))
                ::SetWindowPos(pageWnd, HWND_TOP, x, y, width, height, SWP_NOACTIVATE);
        }
    }

    LRESULT on_erase(HWND target, HDC dc) {
        RECT rc = {}; ::GetClientRect(target, &rc);
        FillRect(dc, &rc, background_brush());
        return 1;
    }

    void update_tooltip_width() {
        if (tooltip_ != nullptr) SendMessageW(tooltip_, TTM_SETMAXTIPWIDTH, 0, kTooltipMaxWidthPixels);
    }

    LRESULT on_control_color(HDC dc, HWND control, UINT msg) {
        if (!dark_) return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        SetTextColor(dc, kDarkText);
        SetBkColor(dc, kDarkBackground);
        SetBkMode(dc, TRANSPARENT);
        if (msg == WM_CTLCOLOREDIT || is_edit_control(control)) {
            SetBkMode(dc, OPAQUE);
            SetBkColor(dc, kDarkEditBackground);
            return reinterpret_cast<LRESULT>(editBrush_ != nullptr ? editBrush_ : background_brush());
        }
        return reinterpret_cast<LRESULT>(background_brush());
    }

    LRESULT on_command(WPARAM wp, LPARAM) {
        const WORD code = HIWORD(wp);
        if (code == CBN_SELCHANGE || code == BN_CLICKED || code == EN_CHANGE)
            callback_->on_state_changed();
        return 0;
    }

    LRESULT on_notify(NMHDR* header) {
        if (header != nullptr && header->idFrom == idTabs && header->code == TCN_SELCHANGE) {
            HWND tabs = find_dlg_item(wnd_, idTabs);
            const int selected = tabs != nullptr ? TabCtrl_GetCurSel(tabs) : -1;
            if (selected >= 0 && selected < static_cast<int>(Page::Count)) {
                selectedPage_ = selected;
                show_selected_page();
            }
        }
        return 0;
    }

    void add_tab(HWND tabs, int index, const wchar_t* label) {
        if (tabs == nullptr) return;
        TCITEMW item = {}; item.mask = TCIF_TEXT; item.pszText = const_cast<wchar_t*>(label);
        TabCtrl_InsertItem(tabs, index, &item);
    }

    void create_tooltips() {
        if (tooltip_ != nullptr) return;
        tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, wnd_, nullptr, core_api::get_my_instance(), nullptr);
        if (tooltip_ != nullptr) {
            ::SetWindowPos(tooltip_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            update_tooltip_width();
        }
    }

    void add_tooltip(HWND control, const wchar_t* text) {
        if (tooltip_ == nullptr || control == nullptr || text == nullptr) return;
        TOOLINFOW tool = {}; tool.cbSize = sizeof(tool); tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        HWND owner = ::GetParent(control);
        tool.hwnd = owner != nullptr ? owner : wnd_;
        tool.uId = reinterpret_cast<UINT_PTR>(control);
        tool.lpszText = const_cast<wchar_t*>(text);
        SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
    }

    void populate() {
        create_tooltips();
        HWND tabs = find_dlg_item(wnd_, idTabs);
        add_tab(tabs, 0, L"Layout");
        add_tab(tabs, 1, L"Test");
        add_tab(tabs, 2, L"About");

        create_page(Page::Layout, IDD_SPATIAL_AUDIO_PAGE_LAYOUT);
        create_page(Page::Test,   IDD_SPATIAL_AUDIO_PAGE_TEST);
        create_page(Page::About,  IDD_SPATIAL_AUDIO_PAGE_ABOUT);
        position_pages();

        populate_layout_page();
        populate_test_page();
        populate_about_page();

        write_to_controls(initial_);
        selectedPage_ = 0;
        show_selected_page();
    }

    void populate_layout_page() {
        HWND layoutCombo = find_dlg_item(wnd_, idLayoutMode);
        for (const auto& option : kLayoutOptions)
            add_combo_item(layoutCombo, option.label, static_cast<LPARAM>(option.mode));
        add_tooltip(layoutCombo, L"Choose which spatial bed channels to activate. Auto uses everything the endpoint exposes.");

        HWND srCombo = find_dlg_item(wnd_, idSampleRateMode);
        for (const auto& option : kSampleRateOptions)
            add_combo_item(srCombo, option.label, static_cast<LPARAM>(option.mode));
        add_tooltip(srCombo, L"48000 Hz is the safest default. Higher rates use more CPU on some hardware.");
    }

    void populate_test_page() {
        HWND testTargetCombo = find_dlg_item(wnd_, idDirectionalTestTarget);
        for (const auto& target : kTargets)
            add_combo_item(testTargetCombo, target.label, target.target);
        add_tooltip(find_dlg_item(wnd_, idDirectionalTestEnabled), L"Play a test tone through a single speaker to verify routing.");
        add_tooltip(find_dlg_item(wnd_, idDirectionalTestDynamic), L"Use Windows dynamic spatial object instead of a static bed channel. May be more accurate on some hardware.");
    }

    void populate_about_page() {
        HWND versionLabel = find_dlg_item(wnd_, idAboutVersion);
        if (versionLabel != nullptr) {
            std::wstring versionText;
            const char* ver = SPATIAL_AUDIO_COMPONENT_VERSION;
            int len = MultiByteToWideChar(CP_UTF8, 0, ver, -1, nullptr, 0);
            if (len > 0) {
                versionText.resize(static_cast<size_t>(len - 1));
                MultiByteToWideChar(CP_UTF8, 0, ver, -1, versionText.data(), len);
            }
            SetWindowTextW(versionLabel, versionText.c_str());
        }

        HWND githubBtn = find_dlg_item(wnd_, idGitHubButton);
        if (githubBtn != nullptr) add_tooltip(githubBtn, L"Open the GitHub repository in your default browser.");

        HWND supportBtn = find_dlg_item(wnd_, idSupportButton);
        if (supportBtn != nullptr) add_tooltip(supportBtn, L"Open the support / discussion page.");
    }

    void show_selected_page() {
        position_pages();
        for (size_t page = 0; page < pageWnds_.size(); ++page) {
            const int command = page == static_cast<size_t>(selectedPage_) ? SW_SHOW : SW_HIDE;
            HWND pageWnd = pageWnds_[page];
            if (pageWnd == nullptr) continue;
            ::ShowWindow(pageWnd, command);
            if (command == SW_SHOW) {
                ::SetWindowPos(pageWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                ::InvalidateRect(pageWnd, nullptr, TRUE);
            }
        }
        ::RedrawWindow(wnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }

    HBRUSH background_brush() const {
        return dark_ && backgroundBrush_ != nullptr ? backgroundBrush_ : GetSysColorBrush(COLOR_WINDOW);
    }

    static bool is_edit_control(HWND control) {
        wchar_t className[16] = {};
        GetClassNameW(control, className, static_cast<int>(_countof(className)));
        return _wcsicmp(className, L"Edit") == 0;
    }

    LayoutMode read_layout_mode() const {
        HWND combo = find_dlg_item(wnd_, idLayoutMode);
        if (combo == nullptr) return LayoutMode::Auto;
        const int index = combo_get_cur_sel(combo);
        if (index == CB_ERR) return LayoutMode::Auto;
        return static_cast<LayoutMode>(combo_get_item_data(combo, index));
    }

    void set_layout_mode(LayoutMode mode) const {
        HWND combo = find_dlg_item(wnd_, idLayoutMode);
        if (combo == nullptr) return;
        for (int i = 0; i < combo_get_count(combo); ++i) {
            if (static_cast<LayoutMode>(combo_get_item_data(combo, i)) == mode) {
                combo_set_cur_sel(combo, i); return;
            }
        }
        combo_set_cur_sel(combo, 0);
    }

    SampleRateMode read_sample_rate_mode() const {
        HWND combo = find_dlg_item(wnd_, idSampleRateMode);
        if (combo == nullptr) return SampleRateMode::Fixed48000;
        const int index = combo_get_cur_sel(combo);
        if (index == CB_ERR) return SampleRateMode::Fixed48000;
        return static_cast<SampleRateMode>(combo_get_item_data(combo, index));
    }

    void set_sample_rate_mode(SampleRateMode mode) const {
        HWND combo = find_dlg_item(wnd_, idSampleRateMode);
        if (combo == nullptr) return;
        for (int i = 0; i < combo_get_count(combo); ++i) {
            if (static_cast<SampleRateMode>(combo_get_item_data(combo, i)) == mode) {
                combo_set_cur_sel(combo, i); return;
            }
        }
        combo_set_cur_sel(combo, 0);
    }

    int read_test_target() const {
        HWND combo = find_dlg_item(wnd_, idDirectionalTestTarget);
        if (combo == nullptr) return target_front_center;
        const int index = combo_get_cur_sel(combo);
        if (index == CB_ERR) return target_front_center;
        return static_cast<int>(combo_get_item_data(combo, index));
    }

    void set_test_target(int target) const {
        HWND combo = find_dlg_item(wnd_, idDirectionalTestTarget);
        if (combo == nullptr) return;
        for (int i = 0; i < combo_get_count(combo); ++i) {
            if (static_cast<int>(combo_get_item_data(combo, i)) == target) {
                combo_set_cur_sel(combo, i); return;
            }
        }
        combo_set_cur_sel(combo, 0);
    }

    OutputConfig read_from_controls() const {
        OutputConfig config;
        config.layoutMode = read_layout_mode();
        config.sampleRateMode = read_sample_rate_mode();
        config.directionalTestEnabled = read_check(wnd_, idDirectionalTestEnabled);
        config.directionalTestUseDynamicObject = read_check(wnd_, idDirectionalTestDynamic);
        config.directionalTestTarget = read_test_target();
        config.directionalTestGainDb = read_double(wnd_, idDirectionalTestGain, -18.0);
        config.directionalTestFrequencyHz = read_double(wnd_, idDirectionalTestFrequency, 660.0);
        return config;
    }

    void write_to_controls(const OutputConfig& config) const {
        set_layout_mode(config.layoutMode);
        set_sample_rate_mode(config.sampleRateMode);
        set_check(wnd_, idDirectionalTestEnabled, config.directionalTestEnabled);
        set_check(wnd_, idDirectionalTestDynamic, config.directionalTestUseDynamicObject);
        set_test_target(config.directionalTestTarget);
        set_double_text(wnd_, idDirectionalTestGain, config.directionalTestGainDb, 1);
        set_double_text(wnd_, idDirectionalTestFrequency, config.directionalTestFrequencyHz, 0);
    }

    static bool different(double a, double b) { return std::fabs(a - b) > 0.0001; }

    bool has_changed() const {
        const OutputConfig current = read_from_controls();
        return current.layoutMode    != initial_.layoutMode
            || current.sampleRateMode!= initial_.sampleRateMode
            || current.directionalTestEnabled           != initial_.directionalTestEnabled
            || current.directionalTestUseDynamicObject  != initial_.directionalTestUseDynamicObject
            || current.directionalTestTarget            != initial_.directionalTestTarget
            || different(current.directionalTestGainDb, initial_.directionalTestGainDb)
            || different(current.directionalTestFrequencyHz, initial_.directionalTestFrequencyHz);
    }

    HWND wnd_ = nullptr;
    preferences_page_callback::ptr callback_;
    OutputConfig initial_;
    fb2k::CCoreDarkModeHooks dark_;
    CDialogResizeHelper m_resizer{kMainResizeParams};
    HWND tooltip_ = nullptr;
    std::array<HWND, static_cast<size_t>(Page::Count)> pageWnds_ = {};
    int selectedPage_ = 0;
    HBRUSH backgroundBrush_ = CreateSolidBrush(kDarkBackground);
    HBRUSH editBrush_       = CreateSolidBrush(kDarkEditBackground);
};

class preferences_page_spatial_audio : public preferences_page_impl<preferences_instance> {
public:
    const char* get_name() override { return "Spatial Audio for Home Theater"; }
    GUID get_guid() override { return guid_preferences; }
    GUID get_parent_guid() override { return preferences_page::guid_output; }
};

static preferences_page_factory_t<preferences_page_spatial_audio> g_preferences_factory;

}  // namespace
}  // namespace spatial_audio
