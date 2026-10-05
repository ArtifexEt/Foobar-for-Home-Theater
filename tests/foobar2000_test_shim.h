#pragma once

// Minimal in-memory SDK adapter for executing the production DSP on any host.
// It deliberately does not emulate Windows Spatial Audio or render an audio device.
// The Windows workflow separately compiles the plugins against the real SDK.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct GUID {
    uint32_t data1;
    uint16_t data2, data3;
    uint8_t data4[8];
    bool operator==(const GUID&) const = default;
};

template<class T> class test_cfg {
public:
    test_cfg(GUID, T value) : value_(value) {}
    T get() const { return value_; }
    test_cfg& operator=(T value) { value_ = value; return *this; }
private:
    T value_;
};
using cfg_int = test_cfg<int>;
using cfg_float = test_cfg<double>;
using cfg_bool = test_cfg<bool>;
using audio_sample = float;

class audio_chunk {
public:
    enum : unsigned {
        channel_front_left = 1u << 0, channel_front_right = 1u << 1,
        channel_front_center = 1u << 2, channel_lfe = 1u << 3,
        channel_back_left = 1u << 4, channel_back_right = 1u << 5,
        channel_front_center_left = 1u << 6, channel_front_center_right = 1u << 7,
        channel_back_center = 1u << 8,
        channel_side_left = 1u << 9, channel_side_right = 1u << 10,
        channel_top_center = 1u << 11,
        channel_top_front_left = 1u << 12, channel_top_front_center = 1u << 13,
        channel_top_front_right = 1u << 14,
        channel_top_back_left = 1u << 15, channel_top_back_center = 1u << 16,
        channel_top_back_right = 1u << 17,
        channels_front_left_right = channel_front_left | channel_front_right,
        channels_back_left_right = channel_back_left | channel_back_right,
        channels_side_left_right = channel_side_left | channel_side_right,
        channel_config_mono = channel_front_center,
        channel_config_stereo = channels_front_left_right,
        channel_config_5point1 = channels_front_left_right | channel_front_center | channel_lfe | channels_back_left_right,
        channel_config_5point1_side = channels_front_left_right | channel_front_center | channel_lfe | channels_side_left_right,
        channel_config_7point1 = channel_config_5point1 | channels_side_left_right,
    };
    static constexpr unsigned g_count_channels(unsigned mask) { return std::popcount(mask); }
    static constexpr unsigned g_channel_index_from_flag(unsigned mask, unsigned flag) {
        return (mask & flag) ? g_count_channels(mask & (flag - 1u)) : ~0u;
    }
    static constexpr unsigned g_extract_channel_flag(unsigned mask, unsigned index) {
        while (mask && index--) mask &= mask - 1u;
        return mask & (~mask + 1u);
    }
    static constexpr unsigned g_guess_channel_config(unsigned channels) {
        switch (channels) {
        case 1: return channel_config_mono;
        case 2: return channel_config_stereo;
        case 6: return channel_config_5point1;
        case 8: return channel_config_7point1;
        default: return 0;
        }
    }
    unsigned get_channel_count() const { return channels_; }
    unsigned get_channel_config() const { return mask_; }
    unsigned get_sample_rate() const { return rate_; }
    size_t get_sample_count() const { return frames_; }
    const audio_sample* get_data() const { return samples_.data(); }
    void set_data(const audio_sample* data, size_t frames, unsigned channels, unsigned rate, unsigned mask) {
        samples_.assign(data, data + frames * channels);
        frames_ = frames; channels_ = channels; rate_ = rate; mask_ = mask;
    }
private:
    std::vector<audio_sample> samples_;
    size_t frames_ = 0;
    unsigned channels_ = 0, rate_ = 48000, mask_ = 0;
};

class dsp_preset {
public:
    void set_owner(GUID owner) { owner_ = owner; }
    GUID get_owner() const { return owner_; }
    void set_data(const void* data, size_t count) { bytes_.assign(static_cast<const char*>(data), count); }
    const void* get_data() const { return bytes_.data(); }
    size_t get_data_size() const { return bytes_.size(); }
private:
    GUID owner_{};
    std::string bytes_;
};
using dsp_preset_impl = dsp_preset;
struct dsp_preset_edit_callback { void on_preset_changed(const dsp_preset&) {} };
struct abort_callback {};
class dsp_impl_base {
public:
    virtual ~dsp_impl_base() = default;
    virtual bool on_chunk(audio_chunk*, abort_callback&) = 0;
    virtual void on_endoftrack(abort_callback&) = 0;
    virtual void on_endofplayback(abort_callback&) = 0;
    virtual void flush() = 0;
    virtual double get_latency() = 0;
    virtual bool need_track_change_mark() = 0;
};
template<class T> struct dsp_factory_t {};
namespace pfc { using string_base = std::string; }

// UI entry points are compiled but never exercised in the portable tests.
using HWND = void*;
using CWindow = HWND;
using WORD = unsigned short;
using UINT = unsigned;
using BOOL = int;
using LPARAM = intptr_t;
using LRESULT = intptr_t;
constexpr int TRUE = 1, IDOK = 1, IDCANCEL = 2;
constexpr int BN_CLICKED = 0, CB_ADDSTRING = 1, CB_SETCURSEL = 2, CB_GETCURSEL = 3;
#define BEGIN_MSG_MAP_EX(...)
#define END_MSG_MAP(...)
#define COMMAND_ID_HANDLER(...)
#define COMMAND_HANDLER_EX(...)
#define MSG_WM_INITDIALOG(...)
#define _countof(array) (sizeof(array) / sizeof((array)[0]))
template<class T> class CDialogImpl {
public:
    void DoModal(HWND) {}
    void EndDialog(int) {}
    HWND GetDlgItem(int) { return nullptr; }
    LRESULT SendDlgItemMessage(int, int) { return 0; }
    HWND m_hWnd = nullptr;
};
inline void GetDlgItemTextW(HWND, int, wchar_t*, int) {}
inline void SetDlgItemTextW(HWND, int, const wchar_t*) {}
inline void EnableWindow(HWND, BOOL) {}
inline LRESULT SendMessageW(HWND, int, int, LPARAM) { return 0; }
template<size_t N> int swprintf_s(wchar_t (&buffer)[N], const wchar_t* format, double value) {
    return std::swprintf(buffer, N, format, value);
}
