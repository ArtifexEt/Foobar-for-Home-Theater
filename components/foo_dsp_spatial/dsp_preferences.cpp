#include "stdafx.h"
#include "component_version.h"
#include "dsp_config.h"
#include "dsp_preferences_resource.h"

#include <helpers/atl-misc.h>

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

namespace spatial_audio {
namespace {

static constexpr GUID guid_dsp_preferences = { 0xBBCCDDEE, 0x2233, 0x4455, { 0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD } };
static constexpr COLORREF kDarkBackground  = RGB(32, 32, 32);
static constexpr COLORREF kDarkEditBackground = RGB(24, 24, 24);
static constexpr COLORREF kDarkText        = RGB(232, 232, 232);
static constexpr int kTooltipMaxWidthPixels = 360;

enum class Page { Upmix = 0, Channels, Mapping, Lfe, Limiter, About, Count };

struct MappingOption {
    int target;
    const wchar_t* label;
};

const MappingOption kMappingOptions[] = {
    {target_front_left,      L"Front left"},
    {target_front_right,     L"Front right"},
    {target_front_center,    L"Front center"},
    {target_low_frequency,   L"LFE"},
    {target_side_left,       L"Side left"},
    {target_side_right,      L"Side right"},
    {target_back_left,       L"Back left"},
    {target_back_right,      L"Back right"},
    {target_top_front_left,  L"Top front left"},
    {target_top_front_right, L"Top front right"},
    {target_top_back_left,   L"Top back left"},
    {target_top_back_right,  L"Top back right"},
    {target_disabled,        L"Disabled"},
};

struct LimiterOption { LimiterMode mode; const wchar_t* label; };
struct UpmixOption   { UpmixMode mode;   const wchar_t* label; };

const LimiterOption kLimiterOptions[] = {
    {LimiterMode::TransparentSoft, L"Transparent soft"},
    {LimiterMode::HardCeiling,     L"Hard ceiling"},
};

const UpmixOption kUpmixOptions[] = {
    {UpmixMode::Reference, L"Reference"},
    {UpmixMode::Full,      L"Full spatial"},
    {UpmixMode::FrontOnly, L"Front only"},
};

struct SliderBinding {
    int editId;
    int sliderId;
    double minValue;
    double maxValue;
    double scale;
    int decimals;
};

struct PageEnumData { HWND parent = nullptr; int maxBottom = 0; };

static BOOL CALLBACK page_max_bottom_proc(HWND child, LPARAM lp) {
    auto* data = reinterpret_cast<PageEnumData*>(lp);
    if (::GetParent(child) != data->parent) return TRUE;
    RECT r = {};
    ::GetWindowRect(child, &r);
    ::MapWindowPoints(nullptr, data->parent, reinterpret_cast<POINT*>(&r), 2);
    wchar_t cls[16] = {};
    if (::GetClassNameW(child, cls, _countof(cls)) > 0 && _wcsicmp(cls, L"ComboBox") == 0) {
        COMBOBOXINFO cbi = {}; cbi.cbSize = sizeof(cbi);
        if (::GetComboBoxInfo(child, &cbi))
            r.bottom = r.top + std::max(cbi.rcItem.bottom, cbi.rcButton.bottom);
    }
    if (r.bottom > data->maxBottom) data->maxBottom = r.bottom;
    return TRUE;
}

static int measure_content_height(HWND pageWnd) {
    PageEnumData data = {pageWnd, 0};
    ::EnumChildWindows(pageWnd, page_max_bottom_proc, reinterpret_cast<LPARAM>(&data));
    return data.maxBottom > 0 ? data.maxBottom + 8 : 0;
}

struct ComboHeightData { HWND parent = nullptr; int selectionHeight = 0; int listHeight = 0; };

static BOOL CALLBACK set_combo_height_proc(HWND child, LPARAM lp) {
    auto* data = reinterpret_cast<ComboHeightData*>(lp);
    if (::GetParent(child) != data->parent) return TRUE;
    wchar_t cls[16] = {};
    if (::GetClassNameW(child, cls, _countof(cls)) > 0 && _wcsicmp(cls, L"ComboBox") == 0) {
        ::SendMessageW(child, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), static_cast<LPARAM>(data->selectionHeight));
        ::SendMessageW(child, CB_SETITEMHEIGHT, 0, static_cast<LPARAM>(data->listHeight));
    }
    return TRUE;
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

void add_combo_item(HWND combo, const wchar_t* label, LPARAM data) {
    if (combo == nullptr) return;
    const int index = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label)));
    if (index != CB_ERR && index != CB_ERRSPACE)
        SendMessageW(combo, CB_SETITEMDATA, static_cast<WPARAM>(index), data);
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

bool read_check(HWND wnd, int id) {
    HWND button = find_dlg_item(wnd, id);
    return button != nullptr && SendMessageW(button, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void set_check(HWND wnd, int id, bool checked) {
    HWND button = find_dlg_item(wnd, id);
    if (button != nullptr) SendMessageW(button, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool set_clipboard_text(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return false;
    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) { CloseClipboard(); return false; }
    void* destination = GlobalLock(memory);
    if (destination == nullptr) { GlobalFree(memory); CloseClipboard(); return false; }
    std::memcpy(destination, text.c_str(), bytes);
    GlobalUnlock(memory);
    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) { GlobalFree(memory); CloseClipboard(); return false; }
    CloseClipboard();
    return true;
}

bool get_clipboard_text(HWND owner, std::wstring& text) {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(owner)) return false;
    HGLOBAL memory = GetClipboardData(CF_UNICODETEXT);
    if (memory == nullptr) { CloseClipboard(); return false; }
    const wchar_t* source = static_cast<const wchar_t*>(GlobalLock(memory));
    if (source == nullptr) { CloseClipboard(); return false; }
    text = source;
    GlobalUnlock(memory);
    CloseClipboard();
    return true;
}

std::string narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string result(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    return result;
}

std::wstring widen(const std::string& text) {
    if (text.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) return {};
    std::wstring result(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), required);
    return result;
}

std::wstring widen_ascii(const char* text) {
    if (text == nullptr || *text == '\0') return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (required <= 1) return {};
    std::wstring result(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), required);
    result.resize(static_cast<size_t>(required - 1));
    return result;
}

int read_combo_target(HWND wnd, int id, int fallback) {
    HWND combo = find_dlg_item(wnd, id);
    if (combo == nullptr) return fallback;
    const int index = combo_get_cur_sel(combo);
    if (index == CB_ERR) return fallback;
    return static_cast<int>(combo_get_item_data(combo, index));
}

void set_combo_target(HWND wnd, int id, int target) {
    HWND combo = find_dlg_item(wnd, id);
    if (combo == nullptr) return;
    for (int i = 0; i < combo_get_count(combo); ++i) {
        if (static_cast<int>(combo_get_item_data(combo, i)) == target) {
            combo_set_cur_sel(combo, i); return;
        }
    }
    combo_set_cur_sel(combo, 0);
}

UpmixMode read_upmix_mode(HWND wnd) {
    HWND combo = find_dlg_item(wnd, idUpmixMode);
    if (combo == nullptr) return UpmixMode::Reference;
    const int index = combo_get_cur_sel(combo);
    if (index == CB_ERR) return UpmixMode::Reference;
    return static_cast<UpmixMode>(combo_get_item_data(combo, index));
}

void set_upmix_mode(HWND wnd, UpmixMode mode) {
    HWND combo = find_dlg_item(wnd, idUpmixMode);
    if (combo == nullptr) return;
    for (int i = 0; i < combo_get_count(combo); ++i) {
        if (static_cast<UpmixMode>(combo_get_item_data(combo, i)) == mode) {
            combo_set_cur_sel(combo, i); return;
        }
    }
    combo_set_cur_sel(combo, 0);
}

LimiterMode read_limiter_mode(HWND wnd) {
    HWND combo = find_dlg_item(wnd, idLimiterMode);
    if (combo == nullptr) return LimiterMode::TransparentSoft;
    const int index = combo_get_cur_sel(combo);
    if (index == CB_ERR) return LimiterMode::TransparentSoft;
    return static_cast<LimiterMode>(combo_get_item_data(combo, index));
}

void set_limiter_mode(HWND wnd, LimiterMode mode) {
    HWND combo = find_dlg_item(wnd, idLimiterMode);
    if (combo == nullptr) return;
    for (int i = 0; i < combo_get_count(combo); ++i) {
        if (static_cast<LimiterMode>(combo_get_item_data(combo, i)) == mode) {
            combo_set_cur_sel(combo, i); return;
        }
    }
    combo_set_cur_sel(combo, 0);
}

static const CDialogResizeHelper::Param kMainResizeParams[] = {
    {idTabs, 0.f, 0.f, 1.f, 1.f},
};

class dsp_preferences_instance : public CDialogImpl<dsp_preferences_instance>, public preferences_page_instance {
public:
    enum { IDD = IDD_DSP_PREFERENCES };

    dsp_preferences_instance(preferences_page_callback::ptr callback)
        : callback_(callback), initial_(ReadDspConfig()) {
        m_resizer.m_autoSizeGrip = false;
        INITCOMMONCONTROLSEX cc = {};
        cc.dwSize = sizeof(cc);
        cc.dwICC  = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES;
        InitCommonControlsEx(&cc);
    }

    ~dsp_preferences_instance() {
        if (tooltip_ != nullptr && ::IsWindow(tooltip_)) { ::DestroyWindow(tooltip_); tooltip_ = nullptr; }
        DeleteObject(backgroundBrush_);
        DeleteObject(editBrush_);
    }

    BEGIN_MSG_MAP_EX(dsp_preferences_instance)
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
        MESSAGE_HANDLER(WM_HSCROLL, on_scroll_message)
        MESSAGE_HANDLER(WM_NOTIFY, on_notify_message)
    END_MSG_MAP()

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (has_changed()) state |= preferences_state::changed;
        return state;
    }

    void apply() override {
        WriteDspConfig(read_from_controls());
        initial_ = ReadDspConfig();
        callback_->on_state_changed();
    }

    void reset() override {
        write_to_controls(DefaultDspConfig());
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
    LRESULT on_scroll_message(UINT, WPARAM, LPARAM lp, BOOL&) { return on_scroll(reinterpret_cast<HWND>(lp)); }
    LRESULT on_notify_message(UINT, WPARAM, LPARAM lp, BOOL&) { return on_notify(reinterpret_cast<NMHDR*>(lp)); }

    static INT_PTR CALLBACK page_dialog_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
        dsp_preferences_instance* self = reinterpret_cast<dsp_preferences_instance*>(::GetWindowLongPtrW(wnd, GWLP_USERDATA));
        if (msg == WM_INITDIALOG) {
            self = reinterpret_cast<dsp_preferences_instance*>(lp);
            ::SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            const int ch = measure_content_height(wnd);
            ::SetPropW(wnd, L"dsp_ch", reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(ch)));
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
        case WM_HSCROLL: self->on_scroll(reinterpret_cast<HWND>(lp)); return TRUE;
        case WM_SIZE: self->on_page_size(wnd, static_cast<int>(HIWORD(lp))); return FALSE;
        case WM_VSCROLL: self->on_page_vscroll(wnd, wp); return TRUE;
        case WM_MOUSEWHEEL: self->on_page_mousewheel(wnd, wp); return TRUE;
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

    static size_t page_index(Page page) { return static_cast<size_t>(page); }

    HWND create_page(Page page, int resourceId) {
        HWND pageWnd = CreateDialogParamW(core_api::get_my_instance(), MAKEINTRESOURCEW(resourceId), wnd_, page_dialog_proc, reinterpret_cast<LPARAM>(this));
        if (pageWnd == nullptr) throw std::runtime_error("Could not create DSP preferences page.");
        pageWnds_[page_index(page)] = pageWnd;
        dark_.AddDialogWithControls(pageWnd);
        return pageWnd;
    }

    void on_page_size(HWND pageWnd, int windowHeight) {
        const int contentH = static_cast<int>(reinterpret_cast<LONG_PTR>(::GetPropW(pageWnd, L"dsp_ch")));
        if (contentH <= 0) return;
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        const int prevPos = si.nPos;
        si.nMin = 0; si.nMax = contentH - 1; si.nPage = static_cast<UINT>(std::max(1, windowHeight));
        si.fMask = SIF_RANGE | SIF_PAGE;
        ::SetScrollInfo(pageWnd, SB_VERT, &si, TRUE);
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        if (si.nPos != prevPos)
            ::ScrollWindowEx(pageWnd, 0, prevPos - si.nPos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
    }

    void on_page_vscroll(HWND pageWnd, WPARAM wp) {
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        const int prevPos = si.nPos;
        switch (LOWORD(wp)) {
        case SB_LINEUP:     si.nPos -= 20; break;
        case SB_LINEDOWN:   si.nPos += 20; break;
        case SB_PAGEUP:     si.nPos -= static_cast<int>(si.nPage); break;
        case SB_PAGEDOWN:   si.nPos += static_cast<int>(si.nPage); break;
        case SB_TOP:        si.nPos = si.nMin; break;
        case SB_BOTTOM:     si.nPos = si.nMax; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        default: break;
        }
        si.fMask = SIF_POS;
        ::SetScrollInfo(pageWnd, SB_VERT, &si, TRUE);
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        if (si.nPos != prevPos)
            ::ScrollWindowEx(pageWnd, 0, prevPos - si.nPos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
    }

    void on_page_mousewheel(HWND pageWnd, WPARAM wp) {
        const int delta = GET_WHEEL_DELTA_WPARAM(wp);
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        const int prevPos = si.nPos;
        si.nPos -= (delta / WHEEL_DELTA) * 40;
        si.fMask = SIF_POS;
        ::SetScrollInfo(pageWnd, SB_VERT, &si, TRUE);
        ::GetScrollInfo(pageWnd, SB_VERT, &si);
        if (si.nPos != prevPos)
            ::ScrollWindowEx(pageWnd, 0, prevPos - si.nPos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
    }

    void normalize_combo_heights() {
        HWND refWnd = nullptr;
        for (HWND pw : pageWnds_) {
            if (pw != nullptr) { refWnd = pw; break; }
        }
        if (refWnd == nullptr) return;
        const int targetH = combo_selection_height(refWnd);

        for (HWND pageWnd : pageWnds_) {
            if (pageWnd == nullptr) continue;
            ComboHeightData data = {pageWnd, targetH, targetH};
            ::EnumChildWindows(pageWnd, set_combo_height_proc, reinterpret_cast<LPARAM>(&data));
            const int newH = measure_content_height(pageWnd);
            if (newH > 0)
                ::SetPropW(pageWnd, L"dsp_ch", reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(newH)));
        }
    }

    int combo_selection_height(HWND refWnd) const {
        HDC dc = ::GetDC(refWnd);
        if (dc == nullptr) return 20;
        HFONT font = reinterpret_cast<HFONT>(::SendMessageW(refWnd, WM_GETFONT, 0, 0));
        HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc, font) : nullptr;
        TEXTMETRICW tm = {};
        ::GetTextMetricsW(dc, &tm);
        const int dpiY = ::GetDeviceCaps(dc, LOGPIXELSY);
        if (oldFont != nullptr) ::SelectObject(dc, oldFont);
        ::ReleaseDC(refWnd, dc);
        return std::max(20, tm.tmHeight + tm.tmExternalLeading + MulDiv(7, dpiY > 0 ? dpiY : 96, 96));
    }

    void position_pages() {
        HWND tabs = find_dlg_item(wnd_, idTabs);
        if (tabs == nullptr) return;
        RECT tabRect = {};
        ::GetWindowRect(tabs, &tabRect);
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
        const WORD id   = LOWORD(wp);
        const WORD code = HIWORD(wp);
        if (id == idCopyProfileButton && code == BN_CLICKED) {
            const std::string profile = SerializeDspConfig(read_from_controls());
            if (!set_clipboard_text(wnd_, widen(profile)))
                ::MessageBoxW(wnd_, L"Could not copy profile to clipboard.", L"Spatial Audio for Home Theater DSP", MB_ICONWARNING | MB_OK);
            return 0;
        }
        if (id == idPasteProfileButton && code == BN_CLICKED) {
            std::wstring clipboard;
            DspConfig imported = read_from_controls();
            if (!get_clipboard_text(wnd_, clipboard) || !DeserializeDspConfig(narrow(clipboard.c_str()), imported)) {
                ::MessageBoxW(wnd_, L"Clipboard does not contain a Spatial Audio for Home Theater DSP profile.", L"Spatial Audio for Home Theater DSP", MB_ICONWARNING | MB_OK);
                return 0;
            }
            write_to_controls(imported);
            callback_->on_state_changed();
            return 0;
        }
        if (id == idRepoButton && code == BN_CLICKED) {
            ::ShellExecuteW(wnd_, L"open", L"https://github.com/ArtifexEt/Foobar-for-Home-Theater", nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (id == idSupportButton && code == BN_CLICKED) {
            ::ShellExecuteW(wnd_, L"open", L"https://buymeacoffee.com/szymonrybka", nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (id == idBeginnerDefaultsButton && code == BN_CLICKED) {
            write_to_controls(DefaultDspConfig());
            callback_->on_state_changed();
            return 0;
        }
        if (code == EN_CHANGE && !updatingControls_) {
            sync_slider_from_edit(id);
            callback_->on_state_changed();
        } else if (code == CBN_SELCHANGE || code == BN_CLICKED) {
            callback_->on_state_changed();
        }
        return 0;
    }

    LRESULT on_scroll(HWND source) {
        if (source == nullptr || updatingControls_) return 0;
        const int sliderId = ::GetDlgCtrlID(source);
        if (sync_edit_from_slider(sliderId)) callback_->on_state_changed();
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

    void populate() {
        create_tooltips();
        HWND tabs = find_dlg_item(wnd_, idTabs);
        add_tab(tabs, 0, L"Upmix");
        add_tab(tabs, 1, L"Channels");
        add_tab(tabs, 2, L"Channel Mapping");
        add_tab(tabs, 3, L"LFE");
        add_tab(tabs, 4, L"Limiter");
        add_tab(tabs, 5, L"About");

        create_page(Page::Upmix,    IDD_DSP_PAGE_UPMIX);
        create_page(Page::Channels, IDD_DSP_PAGE_CHANNELS);
        create_page(Page::Mapping,  IDD_DSP_PAGE_MAPPING);
        create_page(Page::Lfe,      IDD_DSP_PAGE_LFE);
        create_page(Page::Limiter,  IDD_DSP_PAGE_LIMITER);
        create_page(Page::About,    IDD_DSP_PAGE_ABOUT);
        position_pages();

        populate_upmix_page();
        populate_channels_page();
        populate_mapping_page();
        populate_lfe_page();
        populate_limiter_page();
        populate_about_page();

        write_to_controls(initial_);
        normalize_combo_heights();
        position_pages();
        selectedPage_ = 0;
        show_selected_page();
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

    void bind_slider(int editId, int sliderId, double minValue, double maxValue, double scale, int decimals) {
        HWND slider = find_dlg_item(wnd_, sliderId);
        if (slider == nullptr) return;
        SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELPARAM(static_cast<int>(std::round(minValue * scale)), static_cast<int>(std::round(maxValue * scale))));
        SendMessageW(slider, TBM_SETPAGESIZE, 0, static_cast<LPARAM>(std::max(1.0, scale)));
        sliders_.push_back({editId, sliderId, minValue, maxValue, scale, decimals});
    }

    void populate_upmix_page() {
        HWND upmixCombo = find_dlg_item(wnd_, idUpmixMode);
        for (const auto& option : kUpmixOptions)
            add_combo_item(upmixCombo, option.label, static_cast<LPARAM>(option.mode));
        add_tooltip(upmixCombo, L"Reference is the beginner default. Full spatial is wider. Front only is useful for A/B comparison.");
        add_tooltip(find_dlg_item(wnd_, idBeginnerDefaultsButton), L"Restore safe defaults: Reference upmix, limiter on, conservative gains.");

        bind_slider(idMasterGain,    idMasterGainSlider,    -60.0, 12.0, 10.0, 1);
        bind_slider(idHeadroom,      idHeadroomSlider,      -24.0,  6.0, 10.0, 1);
        bind_slider(idCenterGain,    idCenterGainSlider,    -60.0, 12.0, 10.0, 1);
        bind_slider(idSurroundGain,  idSurroundGainSlider,  -60.0, 12.0, 10.0, 1);
        bind_slider(idRearGain,      idRearGainSlider,      -60.0, 12.0, 10.0, 1);
        bind_slider(idHeightGain,    idHeightGainSlider,    -60.0, 12.0, 10.0, 1);
        bind_slider(idSideAmount,    idSideAmountSlider,      0.0,  2.0, 100.0, 2);
        bind_slider(idHeightFromMid, idHeightFromMidSlider,   0.0,  1.0, 100.0, 2);
        bind_slider(idDecorrelation, idDecorrelationSlider,   0.0,  1.0, 100.0, 2);
        add_tooltip(find_dlg_item(wnd_, idCopyProfileButton),  L"Copy every setting as a shareable text profile.");
        add_tooltip(find_dlg_item(wnd_, idPasteProfileButton), L"Load a copied profile into this page. Use Apply to save it.");
    }

    void populate_channels_page() {
        for (size_t i = 0; i < target_count; ++i) {
            bind_slider(idChannelGainEditBase  + static_cast<int>(i), idChannelGainSliderBase  + static_cast<int>(i), -24.0, 12.0, 10.0, 1);
            bind_slider(idChannelDelayEditBase + static_cast<int>(i), idChannelDelaySliderBase + static_cast<int>(i),   0.0, 80.0,  1.0, 0);
        }
    }

    void populate_mapping_page() {
        HWND mapFrontLeft    = find_dlg_item(wnd_, idMap51FrontLeft);    populate_mapping_combo(mapFrontLeft);
        HWND mapFrontRight   = find_dlg_item(wnd_, idMap51FrontRight);   populate_mapping_combo(mapFrontRight);
        HWND mapFrontCenter  = find_dlg_item(wnd_, idMap51FrontCenter);  populate_mapping_combo(mapFrontCenter);
        HWND mapLfe          = find_dlg_item(wnd_, idMap51Lfe);          populate_mapping_combo(mapLfe);
        HWND mapSurrLeft     = find_dlg_item(wnd_, idMap51SurroundLeft); populate_mapping_combo(mapSurrLeft);
        HWND mapSurrRight    = find_dlg_item(wnd_, idMap51SurroundRight);populate_mapping_combo(mapSurrRight);
    }

    void populate_lfe_page() {
        add_tooltip(find_dlg_item(wnd_, idEnableLfe), L"Create optional low-frequency content from stereo. Off is the safest music default.");
        bind_slider(idLfeGain,    idLfeGainSlider,    -60.0, 12.0, 10.0, 1);
        bind_slider(idLfeLowpass, idLfeLowpassSlider,  40.0, 250.0, 1.0, 0);
    }

    void populate_limiter_page() {
        add_tooltip(find_dlg_item(wnd_, idLimiterEnabled), L"Keeps summed upmix output from clipping when several channels add together.");
        HWND limiterCombo = find_dlg_item(wnd_, idLimiterMode);
        for (const auto& option : kLimiterOptions)
            add_combo_item(limiterCombo, option.label, static_cast<LPARAM>(option.mode));
        bind_slider(idLimiterCeiling, idLimiterCeilingSlider, -12.0, 0.0, 10.0, 1);
    }

    void populate_about_page() {
        HWND versionLabel = find_dlg_item(wnd_, idAboutVersion);
        if (versionLabel != nullptr) {
            const std::wstring versionText = widen_ascii(SPATIAL_DSP_COMPONENT_VERSION);
            ::SetWindowTextW(versionLabel, versionText.c_str());
        }
        add_tooltip(find_dlg_item(wnd_, idRepoButton), L"Open the GitHub repository in your default browser.");
        add_tooltip(find_dlg_item(wnd_, idSupportButton), L"Open the support page.");
    }

    void populate_mapping_combo(HWND combo) {
        if (combo == nullptr) return;
        for (const auto& option : kMappingOptions)
            add_combo_item(combo, option.label, option.target);
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

    const SliderBinding* slider_from_edit(int editId) const {
        for (const auto& b : sliders_) if (b.editId == editId) return &b;
        return nullptr;
    }

    const SliderBinding* slider_from_slider(int sliderId) const {
        for (const auto& b : sliders_) if (b.sliderId == sliderId) return &b;
        return nullptr;
    }

    int slider_pos_from_value(const SliderBinding& b, double value) const {
        return static_cast<int>(std::round(std::clamp(value, b.minValue, b.maxValue) * b.scale));
    }

    double value_from_slider_pos(const SliderBinding& b, int pos) const {
        return std::clamp(static_cast<double>(pos) / b.scale, b.minValue, b.maxValue);
    }

    void set_numeric(int editId, double value) {
        const SliderBinding* b = slider_from_edit(editId);
        set_double_text(wnd_, editId, value, b != nullptr ? b->decimals : 2);
        sync_slider_from_edit(editId);
    }

    void sync_slider_from_edit(int editId) {
        const SliderBinding* b = slider_from_edit(editId);
        if (b == nullptr) return;
        const double value = read_double(wnd_, editId, 0.0);
        HWND slider = find_dlg_item(wnd_, b->sliderId);
        if (slider != nullptr) SendMessageW(slider, TBM_SETPOS, TRUE, slider_pos_from_value(*b, value));
    }

    bool sync_edit_from_slider(int sliderId) {
        const SliderBinding* b = slider_from_slider(sliderId);
        if (b == nullptr) return false;
        HWND slider = find_dlg_item(wnd_, sliderId);
        if (slider == nullptr) return false;
        updatingControls_ = true;
        const int pos = static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0));
        set_double_text(wnd_, b->editId, value_from_slider_pos(*b, pos), b->decimals);
        updatingControls_ = false;
        return true;
    }

    DspConfig read_from_controls() const {
        DspConfig config;
        config.upmixMode         = read_upmix_mode(wnd_);
        config.masterGainDb      = read_double(wnd_, idMasterGain,    config.masterGainDb);
        config.headroomDb        = read_double(wnd_, idHeadroom,      config.headroomDb);
        config.limiterEnabled    = read_check(wnd_, idLimiterEnabled);
        config.limiterMode       = read_limiter_mode(wnd_);
        config.limiterCeilingDb  = read_double(wnd_, idLimiterCeiling, config.limiterCeilingDb);
        config.centerGainDb      = read_double(wnd_, idCenterGain,    config.centerGainDb);
        config.surroundGainDb    = read_double(wnd_, idSurroundGain,  config.surroundGainDb);
        config.rearGainDb        = read_double(wnd_, idRearGain,      config.rearGainDb);
        config.heightGainDb      = read_double(wnd_, idHeightGain,    config.heightGainDb);
        config.sideAmount        = read_double(wnd_, idSideAmount,    config.sideAmount);
        config.heightFromMid     = read_double(wnd_, idHeightFromMid, config.heightFromMid);
        config.decorrelationAmount = read_double(wnd_, idDecorrelation, config.decorrelationAmount);
        config.enableLfe         = read_check(wnd_, idEnableLfe);
        config.lfeGainDb         = read_double(wnd_, idLfeGain,       config.lfeGainDb);
        config.lfeLowpassHz      = read_double(wnd_, idLfeLowpass,    config.lfeLowpassHz);
        for (size_t i = 0; i < target_count; ++i) {
            config.channelGainDb[i]  = read_double(wnd_, idChannelGainEditBase  + static_cast<int>(i), config.channelGainDb[i]);
            config.channelDelayMs[i] = read_double(wnd_, idChannelDelayEditBase + static_cast<int>(i), config.channelDelayMs[i]);
            config.channelInvert[i]  = read_check(wnd_, idChannelInvertCheckBase + static_cast<int>(i));
        }
        config.map51FrontLeft    = read_combo_target(wnd_, idMap51FrontLeft,     config.map51FrontLeft);
        config.map51FrontRight   = read_combo_target(wnd_, idMap51FrontRight,    config.map51FrontRight);
        config.map51FrontCenter  = read_combo_target(wnd_, idMap51FrontCenter,   config.map51FrontCenter);
        config.map51Lfe          = read_combo_target(wnd_, idMap51Lfe,           config.map51Lfe);
        config.map51SurroundLeft = read_combo_target(wnd_, idMap51SurroundLeft,  config.map51SurroundLeft);
        config.map51SurroundRight= read_combo_target(wnd_, idMap51SurroundRight, config.map51SurroundRight);
        return config;
    }

    void write_to_controls(const DspConfig& config) {
        updatingControls_ = true;
        set_upmix_mode(wnd_, config.upmixMode);
        set_numeric(idMasterGain,   config.masterGainDb);
        set_numeric(idHeadroom,     config.headroomDb);
        set_check(wnd_, idLimiterEnabled, config.limiterEnabled);
        set_limiter_mode(wnd_, config.limiterMode);
        set_numeric(idLimiterCeiling,   config.limiterCeilingDb);
        set_numeric(idCenterGain,   config.centerGainDb);
        set_numeric(idSurroundGain, config.surroundGainDb);
        set_numeric(idRearGain,     config.rearGainDb);
        set_numeric(idHeightGain,   config.heightGainDb);
        set_numeric(idSideAmount,   config.sideAmount);
        set_numeric(idHeightFromMid,config.heightFromMid);
        set_numeric(idDecorrelation,config.decorrelationAmount);
        set_check(wnd_, idEnableLfe, config.enableLfe);
        set_numeric(idLfeGain,      config.lfeGainDb);
        set_numeric(idLfeLowpass,   config.lfeLowpassHz);
        for (size_t i = 0; i < target_count; ++i) {
            set_numeric(idChannelGainEditBase  + static_cast<int>(i), config.channelGainDb[i]);
            set_numeric(idChannelDelayEditBase + static_cast<int>(i), config.channelDelayMs[i]);
            set_check(wnd_, idChannelInvertCheckBase + static_cast<int>(i), config.channelInvert[i]);
        }
        set_combo_target(wnd_, idMap51FrontLeft,     config.map51FrontLeft);
        set_combo_target(wnd_, idMap51FrontRight,    config.map51FrontRight);
        set_combo_target(wnd_, idMap51FrontCenter,   config.map51FrontCenter);
        set_combo_target(wnd_, idMap51Lfe,           config.map51Lfe);
        set_combo_target(wnd_, idMap51SurroundLeft,  config.map51SurroundLeft);
        set_combo_target(wnd_, idMap51SurroundRight, config.map51SurroundRight);
        updatingControls_ = false;
    }

    static bool different(double a, double b) { return std::fabs(a - b) > 0.0001; }

    bool has_changed() const {
        const DspConfig current = read_from_controls();
        if (current.upmixMode != initial_.upmixMode
            || different(current.masterGainDb, initial_.masterGainDb)
            || different(current.headroomDb, initial_.headroomDb)
            || current.limiterEnabled != initial_.limiterEnabled
            || current.limiterMode != initial_.limiterMode
            || different(current.limiterCeilingDb, initial_.limiterCeilingDb)
            || different(current.centerGainDb, initial_.centerGainDb)
            || different(current.surroundGainDb, initial_.surroundGainDb)
            || different(current.rearGainDb, initial_.rearGainDb)
            || different(current.heightGainDb, initial_.heightGainDb)
            || different(current.sideAmount, initial_.sideAmount)
            || different(current.heightFromMid, initial_.heightFromMid)
            || different(current.decorrelationAmount, initial_.decorrelationAmount)
            || current.enableLfe != initial_.enableLfe
            || different(current.lfeGainDb, initial_.lfeGainDb)
            || different(current.lfeLowpassHz, initial_.lfeLowpassHz))
            return true;
        for (size_t i = 0; i < target_count; ++i) {
            if (different(current.channelGainDb[i], initial_.channelGainDb[i])
                || different(current.channelDelayMs[i], initial_.channelDelayMs[i])
                || current.channelInvert[i] != initial_.channelInvert[i])
                return true;
        }
        return current.map51FrontLeft    != initial_.map51FrontLeft
            || current.map51FrontRight   != initial_.map51FrontRight
            || current.map51FrontCenter  != initial_.map51FrontCenter
            || current.map51Lfe          != initial_.map51Lfe
            || current.map51SurroundLeft != initial_.map51SurroundLeft
            || current.map51SurroundRight!= initial_.map51SurroundRight;
    }

    HWND wnd_ = nullptr;
    preferences_page_callback::ptr callback_;
    DspConfig initial_;
    fb2k::CCoreDarkModeHooks dark_;
    CDialogResizeHelper m_resizer{kMainResizeParams};
    HWND tooltip_ = nullptr;
    std::array<HWND, static_cast<size_t>(Page::Count)> pageWnds_ = {};
    std::vector<SliderBinding> sliders_;
    int selectedPage_ = 0;
    bool updatingControls_ = false;
    HBRUSH backgroundBrush_ = CreateSolidBrush(kDarkBackground);
    HBRUSH editBrush_       = CreateSolidBrush(kDarkEditBackground);
};

class preferences_page_dsp_spatial : public preferences_page_impl<dsp_preferences_instance> {
public:
    const char* get_name() override { return "Spatial Audio for Home Theater DSP"; }
    GUID get_guid() override { return guid_dsp_preferences; }
    GUID get_parent_guid() override { return preferences_page::guid_dsp; }
};

static preferences_page_factory_t<preferences_page_dsp_spatial> g_dsp_preferences_factory;

} // namespace
} // namespace spatial_audio
