// These tests instantiate the production dialog resources and scrolling helper.
// Font-scaled templates exercise Win32 dialog-unit layout without claiming a
// complete foobar2000 host session or physical per-monitor DPI coverage.
#include <windows.h>
#include <commctrl.h>
#include <objidl.h>
#include <gdiplus.h>
#include <filesystem>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../components/shared/dialog_scroll.h"

#if defined(TEST_SPATIAL_DSP_DIALOGS)
#include "../components/foo_dsp_spatial/dsp_preferences_resource.h"
#elif defined(TEST_SPATIAL_OUTPUT_DIALOGS)
#include "../components/foo_out_spatial_audio/preferences_resource.h"
#elif defined(TEST_HEIGHT_DSP_DIALOGS)
#include "../components/foo_dsp_height/height_resource.h"
#endif

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

RECT client(HWND window) {
    RECT value{};
    require(GetClientRect(window, &value) != FALSE, "GetClientRect failed");
    return value;
}

RECT control_rect(HWND dialog, HWND control) {
    RECT value{};
    require(GetWindowRect(control, &value) != FALSE, "GetWindowRect failed");
    MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&value), 2);
    // The editable/selection area is the visible part of a closed combo. Some
    // Win32 styles retain the dropdown's configured height in their bounds.
    wchar_t className[32]{};
    GetClassNameW(control, className, 32);
    if (std::wstring(className) == L"ComboBox") {
        COMBOBOXINFO info{sizeof(info)};
        if (GetComboBoxInfo(control, &info))
            value.bottom = std::min(value.bottom, value.top + std::max(info.rcItem.bottom, info.rcButton.bottom) + 2);
    }
    return value;
}

SCROLLINFO scroll_info(HWND dialog, int bar) {
    SCROLLINFO value{sizeof(value), SIF_ALL};
    require(GetScrollInfo(dialog, bar, &value) != FALSE, "GetScrollInfo failed");
    return value;
}

int limit(const SCROLLINFO& value) {
    return std::max(0, value.nMax - static_cast<int>(value.nPage) + 1);
}

bool inside(HWND dialog, HWND control) {
    const RECT bounds = control_rect(dialog, control);
    const RECT viewport = client(dialog);
    return bounds.left >= 0 && bounds.top >= 0 && bounds.right <= viewport.right && bounds.bottom <= viewport.bottom;
}

WORD read_word(const std::vector<unsigned char>& bytes, size_t offset) {
    require(offset + sizeof(WORD) <= bytes.size(), "Truncated dialog template");
    WORD result{};
    std::memcpy(&result, bytes.data() + offset, sizeof(result));
    return result;
}

size_t skip_string_or_ordinal(const std::vector<unsigned char>& bytes, size_t offset) {
    if (read_word(bytes, offset) == 0xffff) return offset + 2 * sizeof(WORD);
    while (read_word(bytes, offset) != 0) offset += sizeof(WORD);
    return offset + sizeof(WORD);
}

std::vector<unsigned char> template_with_font_scale(int id, int percent) {
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(id), RT_DIALOG);
    require(resource != nullptr, "Dialog resource missing: " + std::to_string(id));
    const auto size = SizeofResource(instance, resource);
    const auto* source = static_cast<const unsigned char*>(LockResource(LoadResource(instance, resource)));
    require(source != nullptr && size >= 26, "Dialog resource could not be loaded");
    std::vector<unsigned char> result(source, source + size);
    require(read_word(result, 0) == 1 && read_word(result, 2) == 0xffff, "Expected DIALOGEX resource");
    DWORD style{};
    std::memcpy(&style, result.data() + 12, sizeof(style));
    require((style & DS_SETFONT) != 0, "Expected a dialog resource font");
    size_t position = 26; // Fixed DLGTEMPLATEEX header, followed by menu/class/title.
    for (int field = 0; field != 3; ++field) position = skip_string_or_ordinal(result, position);
    WORD points = static_cast<WORD>((read_word(result, position) * percent + 50) / 100);
    std::memcpy(result.data() + position, &points, sizeof(points));
    return result;
}

struct Dialog {
    spatial_ui::DialogScroll scroll;
    HWND window{};

    static INT_PTR CALLBACK procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_GETMINMAXINFO) {
            // The hidden harness must be able to grow a font-scaled popup
            // beyond the CI desktop. Otherwise Windows clamps WS_THICKFRAME
            // windows to the monitor's tracking limit and the "content fits"
            // phase is never reached. Production sizing remains unchanged.
            auto* limits = reinterpret_cast<MINMAXINFO*>(lp);
            limits->ptMaxTrackSize = {32767, 32767};
            return TRUE;
        }
        auto* self = reinterpret_cast<Dialog*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_INITDIALOG) {
            self = reinterpret_cast<Dialog*>(lp);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->window = window;
            return TRUE;
        }
        if (!self) return FALSE;
        switch (message) {
        case WM_SIZE: self->scroll.resize(); return TRUE;
        case WM_HSCROLL:
        case WM_VSCROLL:
            if (lp == 0) {
                self->scroll.on_scroll(message == WM_HSCROLL ? SB_HORZ : SB_VERT, wp);
                return TRUE;
            }
            break;
        case WM_MOUSEWHEEL: return self->scroll.on_mouse_wheel(wp) ? TRUE : FALSE;
        case WM_MOUSEHWHEEL: return self->scroll.on_mouse_wheel(wp, true) ? TRUE : FALSE;
        case WM_DESTROY: self->scroll.detach(); return TRUE;
        }
        return FALSE;
    }

    Dialog(HWND owner, int id, int fontPercent) {
        auto bytes = template_with_font_scale(id, fontPercent);
        window = CreateDialogIndirectParamW(GetModuleHandleW(nullptr),
            reinterpret_cast<const DLGTEMPLATE*>(bytes.data()), owner, procedure, reinterpret_cast<LPARAM>(this));
        require(window != nullptr, "CreateDialogIndirectParam failed: " + std::to_string(GetLastError()));
        scroll.attach(window);
    }

    ~Dialog() { if (window) DestroyWindow(window); }
    Dialog(const Dialog&) = delete;
    Dialog& operator=(const Dialog&) = delete;
};

void resize_client(HWND window, int width, int height) {
    // Scrollbars change the client size while WM_SIZE is handled. The second
    // pass obtains the correct outer size once their visibility has settled.
    for (int pass = 0; pass < 3; ++pass) {
        const RECT current = client(window);
        RECT outer{};
        require(GetWindowRect(window, &outer) != FALSE, "GetWindowRect failed");
        require(SetWindowPos(window, nullptr, 0, 0,
            outer.right - outer.left + width - current.right,
            outer.bottom - outer.top + height - current.bottom,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER) != FALSE, "Dialog resize failed");
    }
    const RECT actual = client(window);
    require(actual.right == width && actual.bottom == height,
        "Requested client " + std::to_string(width) + "x" + std::to_string(height)
        + ", actual " + std::to_string(actual.right) + "x" + std::to_string(actual.bottom));
}

void require_slider(HWND dialog, int id) {
    const HWND slider = GetDlgItem(dialog, id);
    require(slider != nullptr, "Missing slider: " + std::to_string(id));
    wchar_t className[64]{};
    GetClassNameW(slider, className, 64);
    require(std::wstring(className) == TRACKBAR_CLASSW, "Control is not a native trackbar: " + std::to_string(id));
    require((GetWindowLongPtrW(slider, GWL_STYLE) & WS_TABSTOP) != 0, "Slider is not keyboard focusable");
}

struct Page {
    int resource;
    std::vector<int> reach;
    std::vector<int> sliders;
    bool screenshot = false;
};

class CaptureVisibility {
public:
    explicit CaptureVisibility(HWND dialog) {
        // WM_PRINT's PRF_CHILDREN visits visible children. Native single-line
        // Edit controls additionally depend on ancestor visibility. Show only
        // the harness, outside the desktop and without activating it.
        for (HWND window = dialog; window; window = GetParent(window)) {
            State state{window};
            GetWindowRect(window, &state.bounds);
            const auto style = GetWindowLongPtrW(window, GWL_STYLE);
            state.visible = (style & WS_VISIBLE) != 0;
            state.topLevel = (style & WS_CHILD) == 0;
            windows_.push_back(state);
        }
        for (auto it = windows_.rbegin(); it != windows_.rend(); ++it) {
            if (it->topLevel)
                SetWindowPos(it->window, nullptr, -20000, -20000, 0, 0,
                    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            ShowWindow(it->window, SW_SHOWNOACTIVATE);
        }
        RedrawWindow(dialog, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }

    ~CaptureVisibility() {
        for (const auto& state : windows_) {
            if (!state.visible) ShowWindow(state.window, SW_HIDE);
            if (state.topLevel)
                SetWindowPos(state.window, nullptr, state.bounds.left, state.bounds.top, 0, 0,
                    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

private:
    struct State { HWND window; RECT bounds{}; bool visible = false; bool topLevel = false; };
    std::vector<State> windows_;
};

void save_screenshot(HWND dialog, int resource, int percent, const wchar_t* viewport) {
    wchar_t outputDirectory[32768]{};
    const DWORD length = GetEnvironmentVariableW(L"SPATIAL_DIALOG_SCREENSHOT_DIR", outputDirectory, 32768);
    if (length == 0) return; // Rendering artifacts are optional outside CI.
    require(length < 32768, "Screenshot output path is too long");
    std::filesystem::create_directories(outputDirectory);
    const auto path = std::filesystem::path(outputDirectory) /
        (L"dialog-" + std::to_wstring(resource) + L"-font-" + std::to_wstring(percent) + L"-" + viewport + L".png");
    CaptureVisibility visibility(dialog);
    require(IsWindowVisible(dialog) != FALSE, "Screenshot dialog has hidden ancestors");
    for (HWND child = GetWindow(dialog, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        wchar_t className[32]{};
        GetClassNameW(child, className, 32);
        if (std::wstring(className) == L"Edit")
            require(IsWindowVisible(child) != FALSE, "Screenshot edit has hidden ancestors");
    }
    RECT windowBounds{};
    require(GetWindowRect(dialog, &windowBounds) != FALSE, "Screenshot window bounds unavailable");
    const RECT bounds{0, 0, windowBounds.right - windowBounds.left, windowBounds.bottom - windowBounds.top};
    HDC screen = GetWindowDC(dialog);
    HDC canvas = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, bounds.right, bounds.bottom);
    require(screen != nullptr && canvas != nullptr && bitmap != nullptr, "Screenshot bitmap allocation failed");
    const HGDIOBJ previous = SelectObject(canvas, bitmap);
    FillRect(canvas, &bounds, GetSysColorBrush(COLOR_3DFACE));
    // Capture the whole offscreen window, including scrollbar and edit borders;
    // no desktop capture is involved. This is the native resource test harness.
    SendMessageW(dialog, WM_PRINT, reinterpret_cast<WPARAM>(canvas),
        PRF_NONCLIENT | PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN);
    UINT encoderCount = 0, encoderBytes = 0;
    require(Gdiplus::GetImageEncodersSize(&encoderCount, &encoderBytes) == Gdiplus::Ok, "Cannot enumerate image encoders");
    std::vector<unsigned char> encoderStorage(encoderBytes);
    auto* encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoderStorage.data());
    require(Gdiplus::GetImageEncoders(encoderCount, encoderBytes, encoders) == Gdiplus::Ok, "Cannot load image encoders");
    bool saved = false;
    {
        Gdiplus::Bitmap image(bitmap, nullptr);
        for (UINT index = 0; index < encoderCount; ++index) {
            if (std::wstring(encoders[index].MimeType) == L"image/png") {
                require(image.Save(path.c_str(), &encoders[index].Clsid) == Gdiplus::Ok, "PNG screenshot could not be saved");
                saved = true;
                break;
            }
        }
    }
    SelectObject(canvas, previous);
    DeleteObject(bitmap);
    DeleteDC(canvas);
    ReleaseDC(dialog, screen);
    require(saved, "PNG encoder is missing");
}

void test_page(HWND owner, const Page& page, int fontPercent) {
    Dialog dialog(owner, page.resource, fontPercent);
    for (int slider : page.sliders) require_slider(dialog.window, slider);

    // Save the natural control position to detect accumulation/drift after
    // repeated overflow, resize and scroll cycles.
    const HWND anchor = GetDlgItem(dialog.window, page.reach.front());
    require(anchor != nullptr, "Missing anchor control");
    const RECT anchorOrigin = control_rect(dialog.window, anchor);
    int contentWidth = 0, contentHeight = 0, targetWidth = 0, targetHeight = 0;
    for (HWND child = GetWindow(dialog.window, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        const RECT bounds = control_rect(dialog.window, child);
        wchar_t className[32]{};
        GetClassNameW(child, className, 32);
        if (std::wstring(className) == L"Edit") {
            const auto style = GetWindowLongPtrW(child, GWL_STYLE);
            const auto extendedStyle = GetWindowLongPtrW(child, GWL_EXSTYLE);
            require((style & WS_TABSTOP) != 0, "Numeric/report edit is not keyboard focusable");
            require((style & WS_BORDER) != 0 || (extendedStyle & WS_EX_CLIENTEDGE) != 0,
                "Edit field has no visible boundary");
        }
        contentWidth = std::max(contentWidth, static_cast<int>(bounds.right));
        contentHeight = std::max(contentHeight, static_cast<int>(bounds.bottom));
    }
    for (int id : page.reach) {
        const HWND control = GetDlgItem(dialog.window, id);
        require(control != nullptr, "Missing reachable control: " + std::to_string(id));
        const RECT bounds = control_rect(dialog.window, control);
        targetWidth = std::max(targetWidth, static_cast<int>(bounds.right - bounds.left));
        targetHeight = std::max(targetHeight, static_cast<int>(bounds.bottom - bounds.top));
    }
    const int narrowWidth = std::max(targetWidth + 24, contentWidth * 3 / 5);
    const int shortHeight = std::max(targetHeight + 24, contentHeight * 3 / 5);

    for (int cycle = 0; cycle != 2; ++cycle) {
        resize_client(dialog.window, narrowWidth, shortHeight);
        require((GetWindowLongPtrW(dialog.window, GWL_STYLE) & (WS_VSCROLL | WS_HSCROLL)) == (WS_VSCROLL | WS_HSCROLL),
            "Both scrollbars must appear when controls overflow");
        require(limit(scroll_info(dialog.window, SB_VERT)) > 0, "Vertical scroll range is empty");
        require(limit(scroll_info(dialog.window, SB_HORZ)) > 0, "Horizontal scroll range is empty");

        SendMessageW(dialog.window, WM_VSCROLL, SB_BOTTOM, 0);
        SendMessageW(dialog.window, WM_HSCROLL, SB_RIGHT, 0);
        const auto verticalEnd = scroll_info(dialog.window, SB_VERT);
        const auto horizontalEnd = scroll_info(dialog.window, SB_HORZ);
        require(verticalEnd.nPos == limit(verticalEnd), "Vertical end cannot be reached");
        require(horizontalEnd.nPos == limit(horizontalEnd), "Horizontal end cannot be reached");

        for (int id : page.reach) {
            const HWND control = GetDlgItem(dialog.window, id);
            // This is the production focus-reveal operation. The hidden test
            // windows do not take keyboard focus from the runner's desktop.
            dialog.scroll.ensure_visible(control);
            require(inside(dialog.window, control), "Control remains clipped after focus reveal: " + std::to_string(id));
        }

        if (page.screenshot && cycle == 0 && fontPercent != 150)
            save_screenshot(dialog.window, page.resource, fontPercent, L"small-scrolled");

        SendMessageW(dialog.window, WM_VSCROLL, SB_TOP, 0);
        SendMessageW(dialog.window, WM_HSCROLL, SB_LEFT, 0);
        SendMessageW(dialog.window, WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(-WHEEL_DELTA)), 0);
        require(scroll_info(dialog.window, SB_VERT).nPos > 0, "Mouse wheel did not scroll down");
        SendMessageW(dialog.window, WM_MOUSEHWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
        require(scroll_info(dialog.window, SB_HORZ).nPos > 0, "Horizontal wheel did not scroll right");

        // DPI-change handling must preserve a valid coordinate system even
        // when called with a nonzero scroll position.
        if (cycle == 1) dialog.scroll.dpi_changing();
        dialog.scroll.dpi_changed();
        dialog.scroll.ensure_visible(anchor);
        require(inside(dialog.window, anchor), "DPI recalculation lost a reachable control");

        resize_client(dialog.window, contentWidth + 120, contentHeight + 120);
        require((GetWindowLongPtrW(dialog.window, GWL_STYLE) & (WS_VSCROLL | WS_HSCROLL)) == 0,
            "Scrollbars remain after the content fits");
        const RECT restored = control_rect(dialog.window, anchor);
        require(EqualRect(&restored, &anchorOrigin) != FALSE, "Resizing larger failed to restore original control position");
        for (int id : page.reach) require(inside(dialog.window, GetDlgItem(dialog.window, id)), "Control clipped in large viewport");
        if (page.screenshot && cycle == 0 && fontPercent != 150)
            save_screenshot(dialog.window, page.resource, fontPercent, L"large");
    }
    std::cout << "PASS dialog " << page.resource << ", font " << fontPercent << "%\n";
}

std::vector<Page> pages() {
#if defined(TEST_SPATIAL_DSP_DIALOGS)
    return {
        {IDD_DSP_PAGE_UPMIX, {idLayoutMode, idHeightGainSlider, idBeginnerDefaultsButton}, {idHeightGainSlider, idDecorrelationSlider}},
        {IDD_DSP_PAGE_CHANNELS, {idChannelGainEditFrontLeft, idChannelGainSliderTopMiddleLeft, idChannelDelaySliderTopMiddleLeft,
            idChannelGainSliderTopMiddleRight, idChannelDelaySliderTopMiddleRight, idChannelInvertTopMiddleRight},
            {idChannelGainSliderTopMiddleLeft, idChannelGainSliderTopMiddleRight, idChannelDelaySliderTopMiddleLeft, idChannelDelaySliderTopMiddleRight}, true},
        {IDD_DSP_PAGE_MAPPING, {idMap51FrontLeft, idMap51SurroundRight}, {}},
        {IDD_DSP_PAGE_LFE, {idEnableLfe, idLfeLowpassSlider}, {idLfeLowpassSlider}},
        {IDD_DSP_PAGE_LIMITER, {idLimiterEnabled, idLimiterCeilingSlider}, {idLimiterCeilingSlider}},
        {IDD_DSP_PAGE_ABOUT, {idSupportButton, idRepoButton}, {}}
    };
#elif defined(TEST_SPATIAL_OUTPUT_DIALOGS)
    return {
        {IDD_SPATIAL_AUDIO_PAGE_LAYOUT, {idTopMiddleWidth, idTopMiddleHeight, idTopMiddleDepth, idTopMiddleDepthSlider, idProbeEndpoint},
            {idTopMiddleWidthSlider, idTopMiddleHeightSlider, idTopMiddleDepthSlider}, true},
        {IDD_SPATIAL_AUDIO_PAGE_TEST, {idDirectionalTestDynamic, idTestButtonTopMiddleLeft, idTestButtonTopMiddleRight, idTestButtonTopBackRight},
            {idDirectionalTestGainSlider, idDirectionalTestFrequencySlider}},
        {IDD_SPATIAL_AUDIO_PAGE_ABOUT, {idSupportButton, idGitHubButton}, {}}
    };
#elif defined(TEST_HEIGHT_DSP_DIALOGS)
    return {{IDD_HEIGHT_DSP_CONFIG, {IDC_HEIGHT_GAIN, IDC_TOP_MIDDLE_GAIN_SLIDER, IDC_TOP_MIDDLE_GAIN, IDC_MID_FEED_SLIDER, IDOK, IDCANCEL},
        {IDC_HEIGHT_GAIN_SLIDER, IDC_TOP_MIDDLE_GAIN_SLIDER, IDC_FRONT_DIFFERENCE_SLIDER, IDC_SURROUND_FEED_SLIDER, IDC_MID_FEED_SLIDER}, true}};
#endif
}
} // namespace

int main() {
    try {
        Gdiplus::GdiplusStartupInput gdiplusInput;
        ULONG_PTR gdiplusToken = 0;
        require(Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) == Gdiplus::Ok, "GDI+ initialization failed");
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES};
        require(InitCommonControlsEx(&common) != FALSE, "Common controls could not be initialized");
        const HWND owner = CreateWindowExW(0, L"STATIC", L"Dialog test owner", WS_OVERLAPPEDWINDOW,
            0, 0, 1600, 1200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        require(owner != nullptr, "Owner window could not be created");
        for (int percent : {100, 150, 200}) {
            for (const auto& page : pages()) {
                try {
                    test_page(owner, page, percent);
                } catch (const std::exception& error) {
                    throw std::runtime_error("Dialog " + std::to_string(page.resource) + ", font " + std::to_string(percent) + "%: " + error.what());
                }
            }
        }
        DestroyWindow(owner);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
