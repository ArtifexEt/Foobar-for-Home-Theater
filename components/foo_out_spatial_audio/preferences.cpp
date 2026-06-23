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

const uint32_t kProbeSampleRates[] = {44100, 48000, 88200, 96000, 176400, 192000};

const TargetDef* target_def_from_target(int target) {
    for (const auto& def : kTargets) {
        if (def.target == target) return &def;
    }
    return nullptr;
}

bool supports_dynamic_test_target(int target) {
    return target != target_low_frequency;
}

AudioObjectType add_mask(AudioObjectType mask, AudioObjectType value) {
    return static_cast<AudioObjectType>(static_cast<uint32_t>(mask) | static_cast<uint32_t>(value));
}

bool mask_contains(AudioObjectType mask, AudioObjectType value) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(value)) == static_cast<uint32_t>(value);
}

AudioObjectType requested_static_mask(LayoutMode mode, AudioObjectType nativeMask) {
    if (mode == LayoutMode::Auto) return nativeMask;

    AudioObjectType mask = AudioObjectType_None;
    auto include = [&](std::initializer_list<int> targets) {
        for (const int target : targets) {
            const auto* def = target_def_from_target(target);
            if (def != nullptr) mask = add_mask(mask, def->type);
        }
    };

    switch (mode) {
    case LayoutMode::Stereo:
        include({target_front_left, target_front_right});
        break;
    case LayoutMode::FivePointOne:
        include({target_front_left, target_front_right, target_front_center, target_low_frequency, target_side_left, target_side_right});
        break;
    case LayoutMode::SevenPointOne:
        include({target_front_left, target_front_right, target_front_center, target_low_frequency, target_side_left, target_side_right, target_back_left, target_back_right});
        break;
    case LayoutMode::FivePointOneTwo:
        include({target_front_left, target_front_right, target_front_center, target_low_frequency, target_side_left, target_side_right, target_top_front_left, target_top_front_right});
        break;
    case LayoutMode::FivePointOneFour:
        include({target_front_left, target_front_right, target_front_center, target_low_frequency, target_side_left, target_side_right, target_top_front_left, target_top_front_right, target_top_back_left, target_top_back_right});
        break;
    case LayoutMode::SevenPointOneFour:
    default:
        include({target_front_left, target_front_right, target_front_center, target_low_frequency, target_side_left, target_side_right, target_back_left, target_back_right, target_top_front_left, target_top_front_right, target_top_back_left, target_top_back_right});
        break;
    }

    return mask;
}

std::wstring mask_text(AudioObjectType mask) {
    std::wstring text;
    for (const auto& def : kTargets) {
        if (!mask_contains(mask, def.type)) continue;
        if (!text.empty()) text += L", ";
        text += def.label;
    }
    return text.empty() ? L"none" : text;
}

static std::string narrow(const wchar_t* text) {
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

std::string hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) stream << " (" << narrow(message) << ")";
    return stream.str();
}

void throw_if_failed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << hresult_text(hr);
        throw std::runtime_error(stream.str());
    }
}

double db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

WAVEFORMATEX make_object_format(uint32_t sampleRate) {
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 1;
    format.nSamplesPerSec = sampleRate;
    format.wBitsPerSample = 32;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

bool is_format_supported(ISpatialAudioClient* spatialClient, uint32_t sampleRate) {
    const WAVEFORMATEX format = make_object_format(sampleRate);
    return SUCCEEDED(spatialClient->IsAudioObjectFormatSupported(&format));
}

uint32_t fixed_sample_rate(SampleRateMode mode) {
    switch (mode) {
    case SampleRateMode::Fixed44100:  return 44100;
    case SampleRateMode::Fixed48000:  return 48000;
    case SampleRateMode::Fixed88200:  return 88200;
    case SampleRateMode::Fixed96000:  return 96000;
    case SampleRateMode::Fixed176400: return 176400;
    case SampleRateMode::Fixed192000: return 192000;
    default: return 0;
    }
}

uint32_t highest_supported_sample_rate(ISpatialAudioClient* spatialClient) {
    const uint32_t descendingRates[] = {192000, 176400, 96000, 88200, 48000, 44100};
    for (const uint32_t sampleRate : descendingRates) {
        if (is_format_supported(spatialClient, sampleRate)) return sampleRate;
    }
    return 48000;
}

uint32_t resolve_test_sample_rate(ISpatialAudioClient* spatialClient, SampleRateMode mode) {
    if (mode == SampleRateMode::AutoHighest) return highest_supported_sample_rate(spatialClient);
    const uint32_t fixedRate = fixed_sample_rate(mode);
    if (fixedRate != 0) return fixedRate;
    return is_format_supported(spatialClient, 48000) ? 48000 : highest_supported_sample_rate(spatialClient);
}

ComPtr<ISpatialAudioClient> create_spatial_client() {
    ComPtr<IMMDeviceEnumerator> enumerator;
    throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

    ComPtr<IMMDevice> device;
    throw_if_failed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");

    ComPtr<ISpatialAudioClient> spatialClient;
    throw_if_failed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");
    return spatialClient;
}

ComPtr<ISpatialAudioObjectRenderStream> activate_stream(
    ISpatialAudioClient* spatialClient,
    const WAVEFORMATEX& format,
    AudioObjectType staticMask,
    UINT32 minDynamicObjects,
    UINT32 maxDynamicObjects,
    HANDLE completionEvent) {
    SpatialAudioObjectRenderStreamActivationParams streamParams = {};
    streamParams.ObjectFormat = const_cast<WAVEFORMATEX*>(&format);
    streamParams.StaticObjectTypeMask = staticMask;
    streamParams.MinDynamicObjectCount = minDynamicObjects;
    streamParams.MaxDynamicObjectCount = maxDynamicObjects;
    streamParams.Category = AudioCategory_Media;
    streamParams.EventHandle = completionEvent;
    streamParams.NotifyObject = nullptr;

    PROPVARIANT activationParams;
    PropVariantInit(&activationParams);
    activationParams.vt = VT_BLOB;
    activationParams.blob.cbSize = sizeof(streamParams);
    activationParams.blob.pBlobData = reinterpret_cast<BYTE*>(&streamParams);

    ComPtr<ISpatialAudioObjectRenderStream> stream;
    throw_if_failed(
        spatialClient->ActivateSpatialAudioStream(&activationParams, __uuidof(ISpatialAudioObjectRenderStream), reinterpret_cast<void**>(stream.GetAddressOf())),
        "Activate spatial stream");
    return stream;
}

void fill_sine(float* samples, UINT32 frames, double& phase, double frequencyHz, double gain, uint32_t sampleRate) {
    const double increment = 2.0 * kPi * frequencyHz / static_cast<double>(sampleRate);
    for (UINT32 i = 0; i < frames; ++i) {
        samples[i] = static_cast<float>(std::sin(phase) * gain);
        phase += increment;
        if (phase >= 2.0 * kPi) phase -= 2.0 * kPi;
    }
}

std::atomic_bool g_directional_test_running = false;

void run_directional_test_worker(int target, bool preferDynamicObject, double gainDb, double frequencyHz, SampleRateMode sampleRateMode) {
    bool expected = false;
    if (!g_directional_test_running.compare_exchange_strong(expected, true)) return;

    try {
        const HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        throw_if_failed(coInit, "Initialize COM");
        const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(reinterpret_cast<void*>(1), [](void*) {
            CoUninitialize();
        });

        const auto* targetDef = target_def_from_target(target);
        if (targetDef == nullptr) throw std::runtime_error("Unknown test target.");

        ComPtr<ISpatialAudioClient> spatialClient = create_spatial_client();
        const uint32_t sampleRate = resolve_test_sample_rate(spatialClient.Get(), sampleRateMode);
        const WAVEFORMATEX format = make_object_format(sampleRate);
        throw_if_failed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

        UINT32 maxDynamicObjectCount = 0;
        throw_if_failed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");

        AudioObjectType nativeMask = AudioObjectType_None;
        throw_if_failed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

        const bool useDynamicObject = preferDynamicObject && supports_dynamic_test_target(target) && maxDynamicObjectCount > 0;
        const AudioObjectType staticMask = useDynamicObject ? AudioObjectType_None : targetDef->type;
        if (!useDynamicObject && !mask_contains(nativeMask, targetDef->type))
            throw std::runtime_error("Selected static direction is not exposed by this endpoint.");

        HANDLE completionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (completionEvent == nullptr) throw_if_failed(HRESULT_FROM_WIN32(GetLastError()), "Create completion event");
        const auto closeEvent = std::unique_ptr<void, void (*)(void*)>(completionEvent, [](void* handle) {
            CloseHandle(reinterpret_cast<HANDLE>(handle));
        });

        ComPtr<ISpatialAudioObjectRenderStream> stream = activate_stream(
            spatialClient.Get(),
            format,
            staticMask,
            useDynamicObject ? 1 : 0,
            useDynamicObject ? 1 : 0,
            completionEvent);

        ComPtr<ISpatialAudioObject> object;
        if (!useDynamicObject)
            throw_if_failed(stream->ActivateSpatialAudioObject(targetDef->type, object.GetAddressOf()), "Activate static test object");

        throw_if_failed(stream->Start(), "Start spatial stream");

        double phase = 0.0;
        double renderedSeconds = 0.0;
        const double durationSeconds = 1.4;
        const double gain = db_to_linear(std::clamp(gainDb, -60.0, 0.0));
        const double requestedFrequency = target == target_low_frequency ? targetDef->frequencyHz : frequencyHz;
        const double frequency = std::clamp(requestedFrequency <= 0.0 ? targetDef->frequencyHz : requestedFrequency, 40.0, 2000.0);

        while (renderedSeconds < durationSeconds) {
            if (WaitForSingleObject(completionEvent, 1000) != WAIT_OBJECT_0)
                throw std::runtime_error("Timed out waiting for spatial audio buffer.");

            UINT32 availableDynamicObjects = 0;
            UINT32 frameCount = 0;
            throw_if_failed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

            if (useDynamicObject && !object)
                throw_if_failed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, object.GetAddressOf()), "Activate dynamic test object");

            BYTE* byteBuffer = nullptr;
            UINT32 bufferLength = 0;
            throw_if_failed(object->GetBuffer(&byteBuffer, &bufferLength), "Get test buffer");

            auto* samples = reinterpret_cast<float*>(byteBuffer);
            const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
            fill_sine(samples, framesToWrite, phase, frequency, gain, sampleRate);

            if (useDynamicObject) {
                throw_if_failed(object->SetPosition(targetDef->x, targetDef->y, targetDef->z), "Set dynamic test position");
                throw_if_failed(object->SetVolume(1.0f), "Set dynamic test volume");
            }

            throw_if_failed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
            renderedSeconds += static_cast<double>(frameCount) / static_cast<double>(sampleRate);
        }

        stream->Stop();
        stream->Reset();
    } catch (const std::exception& error) {
        FB2K_console_formatter() << "foo_out_spatial_audio test: " << error.what();
    }

    g_directional_test_running.store(false);
}

void run_directional_test(int target, bool preferDynamicObject, double gainDb, double frequencyHz, SampleRateMode sampleRateMode) {
    std::thread(run_directional_test_worker, target, preferDynamicObject, gainDb, frequencyHz, sampleRateMode).detach();
}

std::wstring query_endpoint_summary(LayoutMode layoutMode, SampleRateMode sampleRateMode) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitialize = SUCCEEDED(coInit);
    if (FAILED(coInit) && coInit != RPC_E_CHANGED_MODE) {
        std::wostringstream text;
        text << L"COM init failed: " << widen(hresult_text(coInit));
        return text.str();
    }
    const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(shouldUninitialize ? reinterpret_cast<void*>(1) : nullptr, [](void* marker) {
        if (marker != nullptr) CoUninitialize();
    });

    try {
        ComPtr<ISpatialAudioClient> spatialClient = create_spatial_client();
        const uint32_t selectedSampleRate = resolve_test_sample_rate(spatialClient.Get(), sampleRateMode);
        const WAVEFORMATEX format = make_object_format(selectedSampleRate);
        const HRESULT formatHr = spatialClient->IsAudioObjectFormatSupported(&format);

        AudioObjectType nativeMask = AudioObjectType_None;
        throw_if_failed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

        UINT32 maxDynamicObjectCount = 0;
        throw_if_failed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");

        const AudioObjectType requestedMask = requested_static_mask(layoutMode, nativeMask);
        AudioObjectType activeMask = AudioObjectType_None;
        AudioObjectType missingMask = AudioObjectType_None;
        for (const auto& target : kTargets) {
            if (!mask_contains(requestedMask, target.type)) continue;
            if (mask_contains(nativeMask, target.type)) activeMask = add_mask(activeMask, target.type);
            else missingMask = add_mask(missingMask, target.type);
        }

        std::wostringstream text;
        text << L"Object format: float32 mono " << selectedSampleRate << L" Hz\r\n";
        text << L"Format supported: " << (SUCCEEDED(formatHr) ? L"yes" : L"no") << L" - " << widen(hresult_text(formatHr)) << L"\r\n";
        text << L"Supported rates: ";
        bool firstRate = true;
        for (const uint32_t sampleRate : kProbeSampleRates) {
            if (!is_format_supported(spatialClient.Get(), sampleRate)) continue;
            if (!firstRate) text << L", ";
            text << sampleRate;
            firstRate = false;
        }
        if (firstRate) text << L"none";
        text << L"\r\n";
        text << L"Native static bed: " << mask_text(nativeMask) << L"\r\n";
        text << L"Requested bed: " << mask_text(requestedMask) << L"\r\n";
        text << L"Active bed after fallback: " << mask_text(activeMask) << L"\r\n";
        text << L"Missing channels: " << mask_text(missingMask) << L"\r\n";
        text << L"Max dynamic objects: " << maxDynamicObjectCount;
        return text.str();
    } catch (const std::exception& error) {
        return L"Endpoint probe failed: " + widen(error.what());
    }
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
        const WORD id = LOWORD(wp);
        const WORD code = HIWORD(wp);
        if (id == idProbeEndpoint && code == BN_CLICKED) {
            const std::wstring summary = query_endpoint_summary(read_layout_mode(), read_sample_rate_mode());
            HWND endpointSummary = find_dlg_item(wnd_, idEndpointSummary);
            if (endpointSummary != nullptr) SetWindowTextW(endpointSummary, summary.c_str());
            return 0;
        }
        if (id == idDirectionalTestRunSelected && code == BN_CLICKED) {
            run_selected_test();
            return 0;
        }
        if (id >= idTestButtonBase && id < idTestButtonBase + static_cast<int>(target_count) && code == BN_CLICKED) {
            const int target = static_cast<int>(id - idTestButtonBase);
            set_test_target(target);
            run_directional_test(target, read_check(wnd_, idDirectionalTestDynamic),
                read_double(wnd_, idDirectionalTestGain, -18.0),
                read_double(wnd_, idDirectionalTestFrequency, 660.0),
                read_sample_rate_mode());
            callback_->on_state_changed();
            return 0;
        }
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
        add_tooltip(find_dlg_item(wnd_, idProbeEndpoint), L"Ask Windows which Spatial Audio channels, objects, and sample rates are available.");
        add_tooltip(find_dlg_item(wnd_, idEndpointSummary), L"Endpoint diagnostics from Windows Spatial Audio.");
    }

    void populate_test_page() {
        HWND testTargetCombo = find_dlg_item(wnd_, idDirectionalTestTarget);
        for (const auto& target : kTargets)
            add_combo_item(testTargetCombo, target.label, target.target);
        add_tooltip(find_dlg_item(wnd_, idDirectionalTestEnabled), L"Play a test tone through a single speaker to verify routing.");
        add_tooltip(find_dlg_item(wnd_, idDirectionalTestDynamic), L"Use Windows dynamic spatial object instead of a static bed channel. May be more accurate on some hardware.");
        add_tooltip(find_dlg_item(wnd_, idDirectionalTestRunSelected), L"Play a short tone in the selected direction without changing playback.");
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
            ::SetWindowTextW(versionLabel, versionText.c_str());
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

    void run_selected_test() const {
        const int target = read_test_target();
        run_directional_test(target, read_check(wnd_, idDirectionalTestDynamic),
            read_double(wnd_, idDirectionalTestGain, -18.0),
            read_double(wnd_, idDirectionalTestFrequency, 660.0),
            read_sample_rate_mode());
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
