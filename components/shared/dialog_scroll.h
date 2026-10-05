#pragma once

// Scrolling for fixed-layout Win32 dialog pages. Keep the controls at their
// natural size: a narrow preferences window must not hide the last controls.
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <vector>

namespace spatial_ui {

class DialogScroll {
public:
    DialogScroll() = default;
    DialogScroll(const DialogScroll&) = delete;
    DialogScroll& operator=(const DialogScroll&) = delete;
    ~DialogScroll() { detach(); }

    void attach(HWND window) {
        detach();
        window_ = window;
        dpi_ = window_dpi();
        if (window_ == nullptr) return;
        ::EnumChildWindows(window_, attach_child, reinterpret_cast<LPARAM>(this));
        resize();
    }

    void detach() {
        for (HWND child : children_) {
            if (::IsWindow(child))
                ::RemoveWindowSubclass(child, child_proc, reinterpret_cast<UINT_PTR>(this));
        }
        children_.clear();
        if (::IsWindow(window_)) move_to(0, 0);
        window_ = nullptr;
        x_ = y_ = 0;
        wheelVertical_ = wheelHorizontal_ = 0;
        pendingDpi_ = false;
    }

    void resize() {
        if (!::IsWindow(window_) || changing_) return;
        // WM_SIZE may arrive while the dialog manager is still applying a
        // monitor DPI change. Its AFTERPARENT (or deferred top-level) handler
        // will remeasure once child geometry is final.
        if (window_dpi() != dpi_) return;
        changing_ = true;
        contentWidth_ = contentHeight_ = 0;
        ::EnumChildWindows(window_, measure_child, reinterpret_cast<LPARAM>(this));
        const int padding = scaled(8);
        if (contentWidth_ > 0) contentWidth_ += padding;
        if (contentHeight_ > 0) contentHeight_ += padding;

        RECT client = {};
        ::GetClientRect(window_, &client);
        const LONG_PTR style = ::GetWindowLongPtrW(window_, GWL_STYLE);
        const int verticalWidth = scrollbar_metric(SM_CXVSCROLL);
        const int horizontalHeight = scrollbar_metric(SM_CYHSCROLL);
        const int fullWidth = client.right + ((style & WS_VSCROLL) != 0 ? verticalWidth : 0);
        const int fullHeight = client.bottom + ((style & WS_HSCROLL) != 0 ? horizontalHeight : 0);
        bool horizontal = false, vertical = false;
        // Each bar reduces the other axis's viewport. Solve both together so
        // they disappear again when the dialog grows.
        for (int i = 0; i != 3; ++i) {
            horizontal = contentWidth_ > fullWidth - (vertical ? verticalWidth : 0);
            vertical = contentHeight_ > fullHeight - (horizontal ? horizontalHeight : 0);
        }
        ::ShowScrollBar(window_, SB_HORZ, horizontal);
        ::ShowScrollBar(window_, SB_VERT, vertical);
        ::GetClientRect(window_, &client);
        viewportWidth_ = (std::max)(1, static_cast<int>(client.right));
        viewportHeight_ = (std::max)(1, static_cast<int>(client.bottom));
        set_range(SB_HORZ, contentWidth_, viewportWidth_, x_);
        set_range(SB_VERT, contentHeight_, viewportHeight_, y_);
        move_to(scroll_position(SB_HORZ), scroll_position(SB_VERT));
        changing_ = false;
    }

    void on_scroll(int bar, WPARAM wp) {
        if (!::IsWindow(window_)) return;
        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL;
        ::GetScrollInfo(window_, bar, &info);
        int next = info.nPos;
        switch (LOWORD(wp)) {
        case SB_LINEUP: next -= scaled(20); break;
        case SB_LINEDOWN: next += scaled(20); break;
        case SB_PAGEUP: next -= static_cast<int>(info.nPage); break;
        case SB_PAGEDOWN: next += static_cast<int>(info.nPage); break;
        case SB_TOP: next = info.nMin; break;
        case SB_BOTTOM: next = info.nMax; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: next = info.nTrackPos; break;
        default: return;
        }
        scroll_to(bar, next);
    }

    bool on_mouse_wheel(WPARAM wp, bool horizontal = false) {
        if (!::IsWindow(window_)) return false;
        const bool nativeHorizontal = horizontal;
        horizontal = horizontal || ((LOWORD(wp) & MK_SHIFT) != 0);
        if (horizontal ? contentWidth_ <= viewportWidth_ : contentHeight_ <= viewportHeight_)
            return false;
        int& accumulated = horizontal ? wheelHorizontal_ : wheelVertical_;
        accumulated += static_cast<short>(HIWORD(wp));
        const int notches = accumulated / WHEEL_DELTA;
        accumulated %= WHEEL_DELTA;
        if (notches == 0) return true;
        UINT lines = 3;
        ::SystemParametersInfoW(horizontal ? SPI_GETWHEELSCROLLCHARS : SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
        const int page = horizontal ? viewportWidth_ : viewportHeight_;
        const int distance = lines == WHEEL_PAGESCROLL ? page : scaled(20) * static_cast<int>(lines);
        const int bar = horizontal ? SB_HORZ : SB_VERT;
        // WM_MOUSEHWHEEL positive means right; a vertical wheel with Shift
        // keeps the usual positive/up direction (left).
        scroll_to(bar, (horizontal ? x_ : y_) + (nativeHorizontal ? 1 : -1) * notches * distance);
        return true;
    }

    void ensure_visible(HWND control) {
        if (!::IsWindow(window_) || !::IsChild(window_, control) || changing_) return;
        RECT bounds = visible_rect(control);
        const int margin = scaled(4);
        int nextX = x_, nextY = y_;
        if (bounds.right > viewportWidth_ - margin) nextX += bounds.right - viewportWidth_ + margin;
        if (bounds.left < margin || bounds.right - bounds.left > viewportWidth_ - 2 * margin)
            nextX = x_ + bounds.left - margin;
        if (bounds.bottom > viewportHeight_ - margin) nextY += bounds.bottom - viewportHeight_ + margin;
        if (bounds.top < margin || bounds.bottom - bounds.top > viewportHeight_ - 2 * margin)
            nextY = y_ + bounds.top - margin;
        scroll_to(SB_HORZ, nextX);
        scroll_to(SB_VERT, nextY);
    }

    // Child dialogs receive BEFOREPARENT before Windows rescales their child
    // positions. Undo scrolling first so DPI scaling never scales an offset
    // twice. Call dpi_changed() after the parent's DPI change is complete.
    void dpi_changing() {
        if (!::IsWindow(window_) || pendingDpi_) return;
        pendingDpi_ = true;
        dpiX_ = x_;
        dpiY_ = y_;
        move_to(0, 0);
    }

    void dpi_changed() {
        if (!::IsWindow(window_)) return;
        const UINT nextDpi = window_dpi();
        const int restoreX = ::MulDiv(pendingDpi_ ? dpiX_ : x_, nextDpi, dpi_);
        const int restoreY = ::MulDiv(pendingDpi_ ? dpiY_ : y_, nextDpi, dpi_);
        if (!pendingDpi_) {
            // Fallback for hosts without BEFOREPARENT: Windows has scaled the
            // currently displayed child coordinates, including this offset.
            x_ = restoreX;
            y_ = restoreY;
        }
        pendingDpi_ = false;
        dpi_ = nextDpi;
        resize();
        scroll_to(SB_HORZ, restoreX);
        scroll_to(SB_VERT, restoreY);
        ensure_visible(::GetFocus());
    }

private:
    static BOOL CALLBACK attach_child(HWND child, LPARAM context) {
        auto* self = reinterpret_cast<DialogScroll*>(context);
        if (::SetWindowSubclass(child, child_proc, reinterpret_cast<UINT_PTR>(self), reinterpret_cast<DWORD_PTR>(self)))
            self->children_.push_back(child);
        return TRUE;
    }

    static LRESULT CALLBACK child_proc(HWND child, UINT message, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR context) {
        auto* self = reinterpret_cast<DialogScroll*>(context);
        if (message == WM_SETFOCUS) self->ensure_visible(child);
        if ((message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) && ::GetFocus() != child) {
            if (self->on_mouse_wheel(wp, message == WM_MOUSEHWHEEL)) return 0;
        }
        if (message == WM_NCDESTROY) {
            ::RemoveWindowSubclass(child, child_proc, id);
            auto it = std::find(self->children_.begin(), self->children_.end(), child);
            if (it != self->children_.end()) *it = nullptr;
        }
        return ::DefSubclassProc(child, message, wp, lp);
    }

    RECT visible_rect(HWND control) const {
        RECT rect = {};
        ::GetWindowRect(control, &rect);
        ::MapWindowPoints(nullptr, window_, reinterpret_cast<POINT*>(&rect), 2);
        wchar_t className[16] = {};
        ::GetClassNameW(control, className, 16);
        if (::lstrcmpiW(className, L"ComboBox") == 0
            && (::GetWindowLongPtrW(control, GWL_STYLE) & 3) != CBS_SIMPLE) {
            COMBOBOXINFO info = {};
            info.cbSize = sizeof(info);
            if (::GetComboBoxInfo(control, &info)) {
                const int closedHeight = (std::max)(info.rcItem.bottom, info.rcButton.bottom) + 2;
                rect.bottom = rect.top + (std::min)(static_cast<int>(rect.bottom - rect.top), closedHeight);
            }
        }
        return rect;
    }

    static BOOL CALLBACK measure_child(HWND child, LPARAM context) {
        auto* self = reinterpret_cast<DialogScroll*>(context);
        if (::GetParent(child) != self->window_) return TRUE;
        const RECT rect = self->visible_rect(child);
        self->contentWidth_ = (std::max)(self->contentWidth_, static_cast<int>(rect.right) + self->x_);
        self->contentHeight_ = (std::max)(self->contentHeight_, static_cast<int>(rect.bottom) + self->y_);
        return TRUE;
    }

    UINT window_dpi() const {
        using GetDpi = UINT(WINAPI*)(HWND);
        static auto getDpi = reinterpret_cast<GetDpi>(::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
        if (getDpi != nullptr && window_ != nullptr) {
            const UINT dpi = getDpi(window_);
            if (dpi != 0) return dpi;
        }
        HDC dc = ::GetDC(window_);
        const int dpi = dc != nullptr ? ::GetDeviceCaps(dc, LOGPIXELSY) : 96;
        if (dc != nullptr) ::ReleaseDC(window_, dc);
        return dpi > 0 ? static_cast<UINT>(dpi) : 96;
    }

    int scaled(int value) const { return ::MulDiv(value, dpi_, 96); }

    int scrollbar_metric(int metric) const {
        using GetMetric = int(WINAPI*)(int, UINT);
        static auto getMetric = reinterpret_cast<GetMetric>(::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi"));
        return getMetric != nullptr ? getMetric(metric, dpi_) : ::GetSystemMetrics(metric);
    }

    void set_range(int bar, int extent, int page, int position) {
        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;
        info.nMax = (std::max)(0, extent - 1);
        info.nPage = static_cast<UINT>(page);
        info.nPos = position;
        ::SetScrollInfo(window_, bar, &info, TRUE);
    }

    int scroll_position(int bar) const {
        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS;
        ::GetScrollInfo(window_, bar, &info);
        return info.nPos;
    }

    void scroll_to(int bar, int position) {
        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS;
        info.nPos = position;
        ::SetScrollInfo(window_, bar, &info, TRUE);
        move_to(bar == SB_HORZ ? scroll_position(bar) : x_, bar == SB_VERT ? scroll_position(bar) : y_);
    }

    void move_to(int nextX, int nextY) {
        const int dx = x_ - nextX, dy = y_ - nextY;
        x_ = nextX;
        y_ = nextY;
        if (dx == 0 && dy == 0) return;
        // ScrollWindowEx(SW_SCROLLCHILDREN) can leave partly clipped children
        // behind. Move every direct child, even those completely offscreen.
        for (HWND child = ::GetWindow(window_, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT)) {
            RECT rect = {};
            ::GetWindowRect(child, &rect);
            ::MapWindowPoints(nullptr, window_, reinterpret_cast<POINT*>(&rect), 2);
            ::SetWindowPos(child, nullptr, rect.left + dx, rect.top + dy, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        ::RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }

    HWND window_ = nullptr;
    std::vector<HWND> children_;
    UINT dpi_ = 96;
    int x_ = 0, y_ = 0;
    int contentWidth_ = 0, contentHeight_ = 0;
    int viewportWidth_ = 1, viewportHeight_ = 1;
    int wheelVertical_ = 0, wheelHorizontal_ = 0;
    int dpiX_ = 0, dpiY_ = 0;
    bool changing_ = false, pendingDpi_ = false;
};

} // namespace spatial_ui
