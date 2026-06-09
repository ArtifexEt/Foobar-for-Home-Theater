#include "stdafx.h"
#include "component_config.h"

using Microsoft::WRL::ComPtr;

namespace spatial_audio {
namespace {

static constexpr GUID guid_preferences = { 0x9a26d4a8, 0x2f0b, 0x47b6, { 0xb5, 0x8d, 0xa0, 0x3c, 0x36, 0x26, 0x8f, 0x91 } };
static constexpr double kPi = 3.14159265358979323846;

enum ControlId {
    idTabs = 1001,
    idLayoutMode,
    idSampleRateMode,
    idProbeEndpoint,
    idEndpointSummary,
    idLimiterEnabled,
    idLimiterMode,
    idSupportButton,
    idRepoButton,
    idEnableLfe,
    idMap51FrontLeft,
    idMap51FrontRight,
    idMap51FrontCenter,
    idMap51Lfe,
    idMap51SurroundLeft,
    idMap51SurroundRight,
    idTestLoopEnabled,
    idTestUseDynamicObject,
    idTestTarget,
    idTestRunSelected,
    idTestButtonBase = 1400,

    idMasterGain = 2000,
    idHeadroom,
    idLimiterCeiling,
    idCenterGain,
    idSurroundGain,
    idRearGain,
    idHeightGain,
    idSideAmount,
    idHeightFromMid,
    idDecorrelation,
    idLfeGain,
    idLfeLowpass,
    idTestGain,
    idTestFrequency,

    idMasterGainSlider = 3000,
    idHeadroomSlider,
    idLimiterCeilingSlider,
    idCenterGainSlider,
    idSurroundGainSlider,
    idRearGainSlider,
    idHeightGainSlider,
    idSideAmountSlider,
    idHeightFromMidSlider,
    idDecorrelationSlider,
    idLfeGainSlider,
    idLfeLowpassSlider,
    idTestGainSlider,
    idTestFrequencySlider,

    idChannelGainEditBase = 4000,
    idChannelGainSliderBase = 4100,
    idChannelDelayEditBase = 4200,
    idChannelDelaySliderBase = 4300,
};

enum class Page {
    Layout = 0,
    Upmix,
    Channels,
    Mapping,
    Test,
    Count,
};

struct TargetDef {
    int target;
    const char* key;
    const wchar_t* label;
    AudioObjectType type;
    double frequencyHz;
    float x;
    float y;
    float z;
};

const TargetDef kTargets[] = {
    {target_front_left, "front_left", L"Front left", AudioObjectType_FrontLeft, 220.0, -1.0f, 0.0f, -1.2f},
    {target_front_right, "front_right", L"Front right", AudioObjectType_FrontRight, 247.0, 1.0f, 0.0f, -1.2f},
    {target_front_center, "front_center", L"Front center", AudioObjectType_FrontCenter, 277.0, 0.0f, 0.0f, -1.3f},
    {target_low_frequency, "low_frequency", L"LFE", AudioObjectType_LowFrequency, 55.0, 0.0f, -0.2f, -0.8f},
    {target_side_left, "side_left", L"Side left", AudioObjectType_SideLeft, 311.0, -1.3f, 0.0f, 0.0f},
    {target_side_right, "side_right", L"Side right", AudioObjectType_SideRight, 349.0, 1.3f, 0.0f, 0.0f},
    {target_back_left, "back_left", L"Back left", AudioObjectType_BackLeft, 392.0, -1.0f, 0.0f, 1.1f},
    {target_back_right, "back_right", L"Back right", AudioObjectType_BackRight, 440.0, 1.0f, 0.0f, 1.1f},
    {target_top_front_left, "top_front_left", L"Top front left", AudioObjectType_TopFrontLeft, 523.25, -0.8f, 1.4f, -0.9f},
    {target_top_front_right, "top_front_right", L"Top front right", AudioObjectType_TopFrontRight, 587.33, 0.8f, 1.4f, -0.9f},
    {target_top_back_left, "top_back_left", L"Top back left", AudioObjectType_TopBackLeft, 659.25, -0.8f, 1.4f, 0.9f},
    {target_top_back_right, "top_back_right", L"Top back right", AudioObjectType_TopBackRight, 739.99, 0.8f, 1.4f, 0.9f},
};

struct MappingOption {
    int target;
    const wchar_t* label;
};

const MappingOption kMappingOptions[] = {
    {target_front_left, L"Front left"},
    {target_front_right, L"Front right"},
    {target_front_center, L"Front center"},
    {target_low_frequency, L"LFE"},
    {target_side_left, L"Side left"},
    {target_side_right, L"Side right"},
    {target_back_left, L"Back left"},
    {target_back_right, L"Back right"},
    {target_top_front_left, L"Top front left"},
    {target_top_front_right, L"Top front right"},
    {target_top_back_left, L"Top back left"},
    {target_top_back_right, L"Top back right"},
    {target_disabled, L"Disabled"},
};

struct LayoutOption {
    LayoutMode mode;
    const wchar_t* label;
};

struct SampleRateOption {
    SampleRateMode mode;
    const wchar_t* label;
};

struct LimiterOption {
    LimiterMode mode;
    const wchar_t* label;
};

const LayoutOption kLayoutOptions[] = {
    {LayoutMode::Auto, L"Auto / endpoint native"},
    {LayoutMode::Stereo, L"Stereo"},
    {LayoutMode::FivePointOne, L"5.1"},
    {LayoutMode::SevenPointOne, L"7.1"},
    {LayoutMode::FivePointOneTwo, L"5.1.2"},
    {LayoutMode::SevenPointOneFour, L"7.1.4"},
};

const SampleRateOption kSampleRateOptions[] = {
    {SampleRateMode::AutoHighest, L"Auto highest supported"},
    {SampleRateMode::SourceIfSupported, L"Source rate if supported"},
    {SampleRateMode::Fixed48000, L"48 kHz compatible"},
    {SampleRateMode::Fixed44100, L"44.1 kHz"},
    {SampleRateMode::Fixed88200, L"88.2 kHz"},
    {SampleRateMode::Fixed96000, L"96 kHz"},
    {SampleRateMode::Fixed176400, L"176.4 kHz"},
    {SampleRateMode::Fixed192000, L"192 kHz"},
};

const LimiterOption kLimiterOptions[] = {
    {LimiterMode::TransparentSoft, L"Transparent soft"},
    {LimiterMode::HardCeiling, L"Hard ceiling"},
};

const uint32_t kProbeSampleRates[] = {44100, 48000, 88200, 96000, 176400, 192000};

struct SliderBinding {
    int editId;
    int sliderId;
    double minValue;
    double maxValue;
    double scale;
    int decimals;
};

const TargetDef* target_def_from_target(int target) {
    for (const auto& def : kTargets) {
        if (def.target == target) {
            return &def;
        }
    }
    return nullptr;
}

AudioObjectType add_mask(AudioObjectType mask, AudioObjectType value) {
    return static_cast<AudioObjectType>(static_cast<uint32_t>(mask) | static_cast<uint32_t>(value));
}

bool mask_contains(AudioObjectType mask, AudioObjectType value) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(value)) == static_cast<uint32_t>(value);
}

AudioObjectType requested_static_mask(LayoutMode mode, AudioObjectType nativeMask) {
    if (mode == LayoutMode::Auto) {
        return nativeMask;
    }

    AudioObjectType mask = AudioObjectType_None;
    auto include = [&](std::initializer_list<int> targets) {
        for (const int target : targets) {
            const auto* def = target_def_from_target(target);
            if (def != nullptr) {
                mask = add_mask(mask, def->type);
            }
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
        if (!mask_contains(mask, def.type)) {
            continue;
        }
        if (!text.empty()) {
            text += L", ";
        }
        text += def.label;
    }
    return text.empty() ? L"none" : text;
}

std::string narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }

    std::string result(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    return result;
}

std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), required);
    return result;
}

std::string hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) {
        stream << " (" << narrow(message) << ")";
    }
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
    case SampleRateMode::Fixed44100:
        return 44100;
    case SampleRateMode::Fixed48000:
        return 48000;
    case SampleRateMode::Fixed88200:
        return 88200;
    case SampleRateMode::Fixed96000:
        return 96000;
    case SampleRateMode::Fixed176400:
        return 176400;
    case SampleRateMode::Fixed192000:
        return 192000;
    default:
        return 0;
    }
}

uint32_t highest_supported_sample_rate(ISpatialAudioClient* spatialClient) {
    const uint32_t descendingRates[] = {192000, 176400, 96000, 88200, 48000, 44100};
    for (const uint32_t sampleRate : descendingRates) {
        if (is_format_supported(spatialClient, sampleRate)) {
            return sampleRate;
        }
    }
    return 48000;
}

uint32_t resolve_test_sample_rate(ISpatialAudioClient* spatialClient, SampleRateMode mode) {
    if (mode == SampleRateMode::AutoHighest) {
        return highest_supported_sample_rate(spatialClient);
    }

    const uint32_t fixedRate = fixed_sample_rate(mode);
    if (fixedRate != 0) {
        return fixedRate;
    }

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
        if (phase >= 2.0 * kPi) {
            phase -= 2.0 * kPi;
        }
    }
}

std::atomic_bool g_directional_test_running = false;

void run_directional_test_worker(int target, bool preferDynamicObject, double gainDb, double frequencyHz, SampleRateMode sampleRateMode) {
    bool expected = false;
    if (!g_directional_test_running.compare_exchange_strong(expected, true)) {
        return;
    }

    try {
        const HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        throw_if_failed(coInit, "Initialize COM");
        const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(reinterpret_cast<void*>(1), [](void*) {
            CoUninitialize();
        });

        const auto* targetDef = target_def_from_target(target);
        if (targetDef == nullptr) {
            throw std::runtime_error("Unknown test target.");
        }

        ComPtr<ISpatialAudioClient> spatialClient = create_spatial_client();
        const uint32_t sampleRate = resolve_test_sample_rate(spatialClient.Get(), sampleRateMode);
        const WAVEFORMATEX format = make_object_format(sampleRate);
        throw_if_failed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

        UINT32 maxDynamicObjectCount = 0;
        throw_if_failed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");

        AudioObjectType nativeMask = AudioObjectType_None;
        throw_if_failed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

        const bool useDynamicObject = preferDynamicObject && maxDynamicObjectCount > 0;
        const AudioObjectType staticMask = useDynamicObject ? AudioObjectType_None : targetDef->type;
        if (!useDynamicObject && !mask_contains(nativeMask, targetDef->type)) {
            throw std::runtime_error("Selected static direction is not exposed by this endpoint.");
        }

        HANDLE completionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (completionEvent == nullptr) {
            throw_if_failed(HRESULT_FROM_WIN32(GetLastError()), "Create completion event");
        }
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
        if (!useDynamicObject) {
            throw_if_failed(stream->ActivateSpatialAudioObject(targetDef->type, object.GetAddressOf()), "Activate static test object");
        }

        throw_if_failed(stream->Start(), "Start spatial stream");

        double phase = 0.0;
        double renderedSeconds = 0.0;
        const double durationSeconds = 1.4;
        const double gain = db_to_linear(std::clamp(gainDb, -60.0, 0.0));
        const double frequency = std::clamp(frequencyHz <= 0.0 ? targetDef->frequencyHz : frequencyHz, 40.0, 2000.0);

        while (renderedSeconds < durationSeconds) {
            if (WaitForSingleObject(completionEvent, 1000) != WAIT_OBJECT_0) {
                throw std::runtime_error("Timed out waiting for spatial audio buffer.");
            }

            UINT32 availableDynamicObjects = 0;
            UINT32 frameCount = 0;
            throw_if_failed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

            if (useDynamicObject && !object) {
                throw_if_failed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, object.GetAddressOf()), "Activate dynamic test object");
            }

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
        if (marker == nullptr) {
            return;
        }
        CoUninitialize();
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
            if (!mask_contains(requestedMask, target.type)) {
                continue;
            }
            if (mask_contains(nativeMask, target.type)) {
                activeMask = add_mask(activeMask, target.type);
            } else {
                missingMask = add_mask(missingMask, target.type);
            }
        }

        std::wostringstream text;
        text << L"Object format: float32 mono " << selectedSampleRate << L" Hz\r\n";
        text << L"Format supported: " << (SUCCEEDED(formatHr) ? L"yes" : L"no") << L" - " << widen(hresult_text(formatHr)) << L"\r\n";
        text << L"Supported rates: ";
        bool firstRate = true;
        for (const uint32_t sampleRate : kProbeSampleRates) {
            if (!is_format_supported(spatialClient.Get(), sampleRate)) {
                continue;
            }
            if (!firstRate) {
                text << L", ";
            }
            text << sampleRate;
            firstRate = false;
        }
        if (firstRate) {
            text << L"none";
        }
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

double read_double(HWND wnd, int id, double fallback) {
    wchar_t buffer[64] = {};
    GetDlgItemTextW(wnd, id, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    const double value = wcstod(buffer, &end);
    return end != buffer ? value : fallback;
}

void set_double_text(HWND wnd, int id, double value, int decimals) {
    wchar_t buffer[64] = {};
    swprintf_s(buffer, decimals <= 0 ? L"%.0f" : decimals == 1 ? L"%.1f" : L"%.2f", value);
    SetDlgItemTextW(wnd, id, buffer);
}

HWND create_label(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y + 3, w, h, parent, nullptr, core_api::get_my_instance(), nullptr);
}

HWND create_edit(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr);
}

HWND create_combo(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr);
}

HWND create_button(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr);
}

void open_url(const wchar_t* url) {
    ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

int read_combo_target(HWND wnd, int id, int fallback) {
    HWND combo = GetDlgItem(wnd, id);
    const int index = ComboBox_GetCurSel(combo);
    if (index == CB_ERR) {
        return fallback;
    }
    return static_cast<int>(ComboBox_GetItemData(combo, index));
}

void set_combo_target(HWND wnd, int id, int target) {
    HWND combo = GetDlgItem(wnd, id);
    for (int i = 0; i < ComboBox_GetCount(combo); ++i) {
        if (static_cast<int>(ComboBox_GetItemData(combo, i)) == target) {
            ComboBox_SetCurSel(combo, i);
            return;
        }
    }
    ComboBox_SetCurSel(combo, 0);
}

LayoutMode read_layout_mode(HWND wnd) {
    HWND combo = GetDlgItem(wnd, idLayoutMode);
    const int index = ComboBox_GetCurSel(combo);
    if (index == CB_ERR) {
        return LayoutMode::Auto;
    }
    return static_cast<LayoutMode>(ComboBox_GetItemData(combo, index));
}

void set_layout_mode(HWND wnd, LayoutMode mode) {
    HWND combo = GetDlgItem(wnd, idLayoutMode);
    for (int i = 0; i < ComboBox_GetCount(combo); ++i) {
        if (static_cast<LayoutMode>(ComboBox_GetItemData(combo, i)) == mode) {
            ComboBox_SetCurSel(combo, i);
            return;
        }
    }
    ComboBox_SetCurSel(combo, 0);
}

SampleRateMode read_sample_rate_mode(HWND wnd) {
    HWND combo = GetDlgItem(wnd, idSampleRateMode);
    const int index = ComboBox_GetCurSel(combo);
    if (index == CB_ERR) {
        return SampleRateMode::AutoHighest;
    }
    return static_cast<SampleRateMode>(ComboBox_GetItemData(combo, index));
}

void set_sample_rate_mode(HWND wnd, SampleRateMode mode) {
    HWND combo = GetDlgItem(wnd, idSampleRateMode);
    for (int i = 0; i < ComboBox_GetCount(combo); ++i) {
        if (static_cast<SampleRateMode>(ComboBox_GetItemData(combo, i)) == mode) {
            ComboBox_SetCurSel(combo, i);
            return;
        }
    }
    ComboBox_SetCurSel(combo, 0);
}

LimiterMode read_limiter_mode(HWND wnd) {
    HWND combo = GetDlgItem(wnd, idLimiterMode);
    const int index = ComboBox_GetCurSel(combo);
    if (index == CB_ERR) {
        return LimiterMode::TransparentSoft;
    }
    return static_cast<LimiterMode>(ComboBox_GetItemData(combo, index));
}

void set_limiter_mode(HWND wnd, LimiterMode mode) {
    HWND combo = GetDlgItem(wnd, idLimiterMode);
    for (int i = 0; i < ComboBox_GetCount(combo); ++i) {
        if (static_cast<LimiterMode>(ComboBox_GetItemData(combo, i)) == mode) {
            ComboBox_SetCurSel(combo, i);
            return;
        }
    }
    ComboBox_SetCurSel(combo, 0);
}

class preferences_instance : public preferences_page_instance {
public:
    preferences_instance(HWND parent, preferences_page_callback::ptr callback) : callback_(callback), initial_(ReadConfig()) {
        INITCOMMONCONTROLSEX commonControls = {};
        commonControls.dwSize = sizeof(commonControls);
        commonControls.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES;
        InitCommonControlsEx(&commonControls);

        register_class();
        wnd_ = CreateWindowExW(0, class_name(), L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 760, 620, parent, nullptr, core_api::get_my_instance(), this);
        populate();
        dark_.AddDialogWithControls(wnd_);
    }

    ~preferences_instance() {
        if (wnd_ != nullptr && IsWindow(wnd_)) {
            DestroyWindow(wnd_);
        }
    }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
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
        return L"foo_out_spatial_audio_preferences";
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

        if (self == nullptr) {
            return DefWindowProcW(wnd, msg, wp, lp);
        }

        switch (msg) {
        case WM_COMMAND:
            return self->on_command(wp, lp);
        case WM_HSCROLL:
            return self->on_scroll(reinterpret_cast<HWND>(lp));
        case WM_NOTIFY:
            return self->on_notify(reinterpret_cast<NMHDR*>(lp));
        default:
            break;
        }

        return DefWindowProcW(wnd, msg, wp, lp);
    }

    LRESULT on_command(WPARAM wp, LPARAM) {
        const WORD id = LOWORD(wp);
        const WORD code = HIWORD(wp);
        if (id == idSupportButton && code == BN_CLICKED) {
            open_url(L"https://buymeacoffee.com/szymonrybka");
            return 0;
        }
        if (id == idRepoButton && code == BN_CLICKED) {
            open_url(L"https://github.com/ArtifexEt/Foobar-for-Home-Theater");
            return 0;
        }
        if (id == idProbeEndpoint && code == BN_CLICKED) {
            const std::wstring summary = query_endpoint_summary(read_layout_mode(wnd_), read_sample_rate_mode(wnd_));
            SetDlgItemTextW(wnd_, idEndpointSummary, summary.c_str());
            return 0;
        }
        if (id == idTestRunSelected && code == BN_CLICKED) {
            run_selected_test();
            return 0;
        }
        if (id >= idTestButtonBase && id < idTestButtonBase + static_cast<int>(target_count) && code == BN_CLICKED) {
            const int target = static_cast<int>(id - idTestButtonBase);
            set_combo_target(wnd_, idTestTarget, target);
            run_directional_test(target, Button_GetCheck(GetDlgItem(wnd_, idTestUseDynamicObject)) == BST_CHECKED, read_double(wnd_, idTestGain, -18.0), read_double(wnd_, idTestFrequency, 660.0), read_sample_rate_mode(wnd_));
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
        if (source == nullptr || updatingControls_) {
            return 0;
        }
        const int sliderId = GetDlgCtrlID(source);
        if (sync_edit_from_slider(sliderId)) {
            callback_->on_state_changed();
        }
        return 0;
    }

    LRESULT on_notify(NMHDR* header) {
        if (header != nullptr && header->idFrom == idTabs && header->code == TCN_SELCHANGE) {
            selectedPage_ = TabCtrl_GetCurSel(GetDlgItem(wnd_, idTabs));
            show_selected_page();
        }
        return 0;
    }

    void populate() {
        HWND tabs = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS, 8, 8, 744, 536, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(idTabs)), core_api::get_my_instance(), nullptr);
        add_tab(tabs, 0, L"Layout");
        add_tab(tabs, 1, L"Upmix");
        add_tab(tabs, 2, L"Channels");
        add_tab(tabs, 3, L"5.1 map");
        add_tab(tabs, 4, L"Test");

        populate_layout_page();
        populate_upmix_page();
        populate_channels_page();
        populate_mapping_page();
        populate_test_page();

        create_button(wnd_, idSupportButton, L"Support: Buy me a coffee", 12, 562, 220, 28);
        create_button(wnd_, idRepoButton, L"GitHub repo", 248, 562, 140, 28);

        write_to_controls(initial_);
        selectedPage_ = 0;
        show_selected_page();
    }

    void add_tab(HWND tabs, int index, const wchar_t* label) {
        TCITEMW item = {};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t*>(label);
        TabCtrl_InsertItem(tabs, index, &item);
    }

    HWND add_page_control(Page page, HWND control) {
        pageControls_[static_cast<size_t>(page)].push_back(control);
        return control;
    }

    HWND add_label(Page page, const wchar_t* text, int x, int y, int w, int h) {
        return add_page_control(page, create_label(wnd_, text, x, y, w, h));
    }

    HWND add_button(Page page, int id, const wchar_t* text, int x, int y, int w, int h) {
        return add_page_control(page, create_button(wnd_, id, text, x, y, w, h));
    }

    HWND add_checkbox(Page page, int id, const wchar_t* text, int x, int y, int w, int h) {
        return add_page_control(page, CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr));
    }

    HWND add_combo(Page page, int id, int x, int y, int w, int h) {
        return add_page_control(page, create_combo(wnd_, id, x, y, w, h));
    }

    void add_slider_row(Page page, const wchar_t* label, int editId, int sliderId, int x, int y, double minValue, double maxValue, double scale, int decimals) {
        add_label(page, label, x, y, 150, 24);
        add_page_control(page, create_edit(wnd_, editId, x + 160, y, 64, 24));
        HWND slider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, x + 236, y - 2, 300, 28, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(sliderId)), core_api::get_my_instance(), nullptr);
        SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELPARAM(static_cast<int>(std::round(minValue * scale)), static_cast<int>(std::round(maxValue * scale))));
        SendMessageW(slider, TBM_SETPAGESIZE, 0, static_cast<LPARAM>(std::max(1.0, scale)));
        add_page_control(page, slider);
        sliders_.push_back({editId, sliderId, minValue, maxValue, scale, decimals});
    }

    void populate_layout_page() {
        const Page page = Page::Layout;
        add_label(page, L"Output bed", 28, 54, 120, 24);
        HWND layoutCombo = add_combo(page, idLayoutMode, 188, 52, 220, 160);
        for (const auto& option : kLayoutOptions) {
            const auto index = ComboBox_AddString(layoutCombo, option.label);
            ComboBox_SetItemData(layoutCombo, index, static_cast<int>(option.mode));
        }

        add_label(page, L"Render rate", 28, 94, 120, 24);
        HWND sampleRateCombo = add_combo(page, idSampleRateMode, 188, 92, 220, 220);
        for (const auto& option : kSampleRateOptions) {
            const auto index = ComboBox_AddString(sampleRateCombo, option.label);
            ComboBox_SetItemData(sampleRateCombo, index, static_cast<int>(option.mode));
        }

        add_checkbox(page, idLimiterEnabled, L"Limiter", 28, 134, 120, 24);
        add_label(page, L"Limiter mode", 188, 134, 110, 24);
        HWND limiterCombo = add_combo(page, idLimiterMode, 318, 132, 180, 120);
        for (const auto& option : kLimiterOptions) {
            const auto index = ComboBox_AddString(limiterCombo, option.label);
            ComboBox_SetItemData(limiterCombo, index, static_cast<int>(option.mode));
        }

        add_slider_row(page, L"Limiter ceiling (dB)", idLimiterCeiling, idLimiterCeilingSlider, 28, 174, -12.0, 0.0, 10.0, 1);

        add_button(page, idProbeEndpoint, L"Probe endpoint", 28, 222, 150, 28);
        HWND summary = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 28, 264, 680, 186, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(idEndpointSummary)), core_api::get_my_instance(), nullptr);
        add_page_control(page, summary);
    }

    void populate_upmix_page() {
        const Page page = Page::Upmix;
        int y = 52;
        add_slider_row(page, L"Master gain (dB)", idMasterGain, idMasterGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Headroom (dB)", idHeadroom, idHeadroomSlider, 28, y, -24.0, 6.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Center gain (dB)", idCenterGain, idCenterGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Surround gain (dB)", idSurroundGain, idSurroundGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Rear gain (dB)", idRearGain, idRearGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Height gain (dB)", idHeightGain, idHeightGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"Side amount", idSideAmount, idSideAmountSlider, 28, y, 0.0, 2.0, 100.0, 2); y += 32;
        add_slider_row(page, L"Height from mid", idHeightFromMid, idHeightFromMidSlider, 28, y, 0.0, 1.0, 100.0, 2); y += 32;
        add_slider_row(page, L"Decorrelate", idDecorrelation, idDecorrelationSlider, 28, y, 0.0, 1.0, 100.0, 2); y += 40;
        add_checkbox(page, idEnableLfe, L"Enable LFE extraction", 28, y, 200, 24); y += 34;
        add_slider_row(page, L"LFE gain (dB)", idLfeGain, idLfeGainSlider, 28, y, -60.0, 12.0, 10.0, 1); y += 32;
        add_slider_row(page, L"LFE low-pass (Hz)", idLfeLowpass, idLfeLowpassSlider, 28, y, 40.0, 250.0, 1.0, 0);
    }

    void populate_channels_page() {
        const Page page = Page::Channels;
        add_label(page, L"Channel", 28, 54, 120, 24);
        add_label(page, L"Gain", 178, 54, 64, 24);
        add_label(page, L"Delay ms", 470, 54, 80, 24);

        int y = 84;
        for (size_t i = 0; i < target_count; ++i) {
            const auto& target = kTargets[i];
            add_label(page, target.label, 28, y, 140, 24);
            add_page_control(page, create_edit(wnd_, idChannelGainEditBase + static_cast<int>(i), 178, y, 56, 24));
            HWND gainSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 242, y - 2, 190, 28, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(idChannelGainSliderBase + i)), core_api::get_my_instance(), nullptr);
            SendMessageW(gainSlider, TBM_SETRANGE, TRUE, MAKELPARAM(-240, 120));
            SendMessageW(gainSlider, TBM_SETPAGESIZE, 0, 10);
            add_page_control(page, gainSlider);
            sliders_.push_back({idChannelGainEditBase + static_cast<int>(i), idChannelGainSliderBase + static_cast<int>(i), -24.0, 12.0, 10.0, 1});

            add_page_control(page, create_edit(wnd_, idChannelDelayEditBase + static_cast<int>(i), 470, y, 56, 24));
            HWND delaySlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 536, y - 2, 170, 28, wnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(idChannelDelaySliderBase + i)), core_api::get_my_instance(), nullptr);
            SendMessageW(delaySlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 80));
            SendMessageW(delaySlider, TBM_SETPAGESIZE, 0, 5);
            add_page_control(page, delaySlider);
            sliders_.push_back({idChannelDelayEditBase + static_cast<int>(i), idChannelDelaySliderBase + static_cast<int>(i), 0.0, 80.0, 1.0, 0});
            y += 30;
        }
    }

    void populate_mapping_page() {
        const Page page = Page::Mapping;
        add_label(page, L"5.1 FL", 28, 58, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51FrontLeft, 128, 56, 180, 220));
        add_label(page, L"5.1 FR", 370, 58, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51FrontRight, 470, 56, 180, 220));

        add_label(page, L"5.1 FC", 28, 98, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51FrontCenter, 128, 96, 180, 220));
        add_label(page, L"5.1 LFE", 370, 98, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51Lfe, 470, 96, 180, 220));

        add_label(page, L"5.1 SL/BL", 28, 138, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51SurroundLeft, 128, 136, 180, 220));
        add_label(page, L"5.1 SR/BR", 370, 138, 80, 24);
        populate_mapping_combo(add_combo(page, idMap51SurroundRight, 470, 136, 180, 220));
    }

    void populate_test_page() {
        const Page page = Page::Test;
        add_checkbox(page, idTestLoopEnabled, L"Loop test while playing", 28, 54, 190, 24);
        add_checkbox(page, idTestUseDynamicObject, L"Prefer dynamic object", 240, 54, 190, 24);

        add_label(page, L"Direction", 28, 92, 100, 24);
        HWND combo = add_combo(page, idTestTarget, 128, 90, 190, 220);
        populate_target_combo(combo);
        add_button(page, idTestRunSelected, L"Run selected", 340, 88, 120, 28);

        add_slider_row(page, L"Test gain (dB)", idTestGain, idTestGainSlider, 28, 132, -60.0, 0.0, 10.0, 1);
        add_slider_row(page, L"Frequency (Hz)", idTestFrequency, idTestFrequencySlider, 28, 164, 40.0, 2000.0, 1.0, 0);

        add_button(page, idTestButtonBase + target_top_front_left, L"Top FL", 182, 222, 90, 30);
        add_button(page, idTestButtonBase + target_top_front_right, L"Top FR", 382, 222, 90, 30);
        add_button(page, idTestButtonBase + target_front_left, L"Front L", 132, 268, 90, 30);
        add_button(page, idTestButtonBase + target_front_center, L"Center", 282, 268, 90, 30);
        add_button(page, idTestButtonBase + target_front_right, L"Front R", 432, 268, 90, 30);
        add_button(page, idTestButtonBase + target_side_left, L"Side L", 82, 318, 90, 30);
        add_label(page, L"Listener", 292, 322, 80, 24);
        add_button(page, idTestButtonBase + target_side_right, L"Side R", 482, 318, 90, 30);
        add_button(page, idTestButtonBase + target_back_left, L"Back L", 132, 368, 90, 30);
        add_button(page, idTestButtonBase + target_low_frequency, L"LFE", 282, 368, 90, 30);
        add_button(page, idTestButtonBase + target_back_right, L"Back R", 432, 368, 90, 30);
        add_button(page, idTestButtonBase + target_top_back_left, L"Top BL", 182, 414, 90, 30);
        add_button(page, idTestButtonBase + target_top_back_right, L"Top BR", 382, 414, 90, 30);
    }

    void populate_mapping_combo(HWND combo) {
        for (const auto& option : kMappingOptions) {
            const auto index = ComboBox_AddString(combo, option.label);
            ComboBox_SetItemData(combo, index, option.target);
        }
    }

    void populate_target_combo(HWND combo) {
        for (const auto& target : kTargets) {
            const auto index = ComboBox_AddString(combo, target.label);
            ComboBox_SetItemData(combo, index, target.target);
        }
    }

    void show_selected_page() {
        for (size_t page = 0; page < pageControls_.size(); ++page) {
            const int command = page == static_cast<size_t>(selectedPage_) ? SW_SHOW : SW_HIDE;
            for (HWND control : pageControls_[page]) {
                ShowWindow(control, command);
            }
        }
    }

    const SliderBinding* slider_from_edit(int editId) const {
        for (const auto& binding : sliders_) {
            if (binding.editId == editId) {
                return &binding;
            }
        }
        return nullptr;
    }

    const SliderBinding* slider_from_slider(int sliderId) const {
        for (const auto& binding : sliders_) {
            if (binding.sliderId == sliderId) {
                return &binding;
            }
        }
        return nullptr;
    }

    int slider_pos_from_value(const SliderBinding& binding, double value) const {
        const double clamped = std::clamp(value, binding.minValue, binding.maxValue);
        return static_cast<int>(std::round(clamped * binding.scale));
    }

    double value_from_slider_pos(const SliderBinding& binding, int pos) const {
        return std::clamp(static_cast<double>(pos) / binding.scale, binding.minValue, binding.maxValue);
    }

    void set_numeric(int editId, double value) {
        const SliderBinding* binding = slider_from_edit(editId);
        const int decimals = binding != nullptr ? binding->decimals : 2;
        set_double_text(wnd_, editId, value, decimals);
        sync_slider_from_edit(editId);
    }

    void sync_slider_from_edit(int editId) {
        const SliderBinding* binding = slider_from_edit(editId);
        if (binding == nullptr) {
            return;
        }

        const double value = read_double(wnd_, editId, 0.0);
        HWND slider = GetDlgItem(wnd_, binding->sliderId);
        if (slider != nullptr) {
            SendMessageW(slider, TBM_SETPOS, TRUE, slider_pos_from_value(*binding, value));
        }
    }

    bool sync_edit_from_slider(int sliderId) {
        const SliderBinding* binding = slider_from_slider(sliderId);
        if (binding == nullptr) {
            return false;
        }

        HWND slider = GetDlgItem(wnd_, sliderId);
        if (slider == nullptr) {
            return false;
        }

        updatingControls_ = true;
        const int pos = static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0));
        set_double_text(wnd_, binding->editId, value_from_slider_pos(*binding, pos), binding->decimals);
        updatingControls_ = false;
        return true;
    }

    RuntimeConfig read_from_controls() const {
        RuntimeConfig config;
        config.layoutMode = read_layout_mode(wnd_);
        config.sampleRateMode = read_sample_rate_mode(wnd_);
        config.masterGainDb = read_double(wnd_, idMasterGain, config.masterGainDb);
        config.headroomDb = read_double(wnd_, idHeadroom, config.headroomDb);
        config.limiterEnabled = Button_GetCheck(GetDlgItem(wnd_, idLimiterEnabled)) == BST_CHECKED;
        config.limiterMode = read_limiter_mode(wnd_);
        config.limiterCeilingDb = read_double(wnd_, idLimiterCeiling, config.limiterCeilingDb);
        config.centerGainDb = read_double(wnd_, idCenterGain, config.centerGainDb);
        config.surroundGainDb = read_double(wnd_, idSurroundGain, config.surroundGainDb);
        config.rearGainDb = read_double(wnd_, idRearGain, config.rearGainDb);
        config.heightGainDb = read_double(wnd_, idHeightGain, config.heightGainDb);
        config.sideAmount = read_double(wnd_, idSideAmount, config.sideAmount);
        config.heightFromMid = read_double(wnd_, idHeightFromMid, config.heightFromMid);
        config.decorrelationAmount = read_double(wnd_, idDecorrelation, config.decorrelationAmount);
        config.enableLfe = Button_GetCheck(GetDlgItem(wnd_, idEnableLfe)) == BST_CHECKED;
        config.lfeGainDb = read_double(wnd_, idLfeGain, config.lfeGainDb);
        config.lfeLowpassHz = read_double(wnd_, idLfeLowpass, config.lfeLowpassHz);
        for (size_t i = 0; i < target_count; ++i) {
            config.channelGainDb[i] = read_double(wnd_, idChannelGainEditBase + static_cast<int>(i), config.channelGainDb[i]);
            config.channelDelayMs[i] = read_double(wnd_, idChannelDelayEditBase + static_cast<int>(i), config.channelDelayMs[i]);
        }
        config.map51FrontLeft = read_combo_target(wnd_, idMap51FrontLeft, config.map51FrontLeft);
        config.map51FrontRight = read_combo_target(wnd_, idMap51FrontRight, config.map51FrontRight);
        config.map51FrontCenter = read_combo_target(wnd_, idMap51FrontCenter, config.map51FrontCenter);
        config.map51Lfe = read_combo_target(wnd_, idMap51Lfe, config.map51Lfe);
        config.map51SurroundLeft = read_combo_target(wnd_, idMap51SurroundLeft, config.map51SurroundLeft);
        config.map51SurroundRight = read_combo_target(wnd_, idMap51SurroundRight, config.map51SurroundRight);
        config.directionalTestEnabled = Button_GetCheck(GetDlgItem(wnd_, idTestLoopEnabled)) == BST_CHECKED;
        config.directionalTestUseDynamicObject = Button_GetCheck(GetDlgItem(wnd_, idTestUseDynamicObject)) == BST_CHECKED;
        config.directionalTestTarget = read_combo_target(wnd_, idTestTarget, config.directionalTestTarget);
        config.directionalTestGainDb = read_double(wnd_, idTestGain, config.directionalTestGainDb);
        config.directionalTestFrequencyHz = read_double(wnd_, idTestFrequency, config.directionalTestFrequencyHz);
        return config;
    }

    void write_to_controls(const RuntimeConfig& config) {
        updatingControls_ = true;
        set_layout_mode(wnd_, config.layoutMode);
        set_sample_rate_mode(wnd_, config.sampleRateMode);
        set_numeric(idMasterGain, config.masterGainDb);
        set_numeric(idHeadroom, config.headroomDb);
        Button_SetCheck(GetDlgItem(wnd_, idLimiterEnabled), config.limiterEnabled ? BST_CHECKED : BST_UNCHECKED);
        set_limiter_mode(wnd_, config.limiterMode);
        set_numeric(idLimiterCeiling, config.limiterCeilingDb);
        set_numeric(idCenterGain, config.centerGainDb);
        set_numeric(idSurroundGain, config.surroundGainDb);
        set_numeric(idRearGain, config.rearGainDb);
        set_numeric(idHeightGain, config.heightGainDb);
        set_numeric(idSideAmount, config.sideAmount);
        set_numeric(idHeightFromMid, config.heightFromMid);
        set_numeric(idDecorrelation, config.decorrelationAmount);
        Button_SetCheck(GetDlgItem(wnd_, idEnableLfe), config.enableLfe ? BST_CHECKED : BST_UNCHECKED);
        set_numeric(idLfeGain, config.lfeGainDb);
        set_numeric(idLfeLowpass, config.lfeLowpassHz);
        for (size_t i = 0; i < target_count; ++i) {
            set_numeric(idChannelGainEditBase + static_cast<int>(i), config.channelGainDb[i]);
            set_numeric(idChannelDelayEditBase + static_cast<int>(i), config.channelDelayMs[i]);
        }
        set_combo_target(wnd_, idMap51FrontLeft, config.map51FrontLeft);
        set_combo_target(wnd_, idMap51FrontRight, config.map51FrontRight);
        set_combo_target(wnd_, idMap51FrontCenter, config.map51FrontCenter);
        set_combo_target(wnd_, idMap51Lfe, config.map51Lfe);
        set_combo_target(wnd_, idMap51SurroundLeft, config.map51SurroundLeft);
        set_combo_target(wnd_, idMap51SurroundRight, config.map51SurroundRight);
        Button_SetCheck(GetDlgItem(wnd_, idTestLoopEnabled), config.directionalTestEnabled ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(wnd_, idTestUseDynamicObject), config.directionalTestUseDynamicObject ? BST_CHECKED : BST_UNCHECKED);
        set_combo_target(wnd_, idTestTarget, config.directionalTestTarget);
        set_numeric(idTestGain, config.directionalTestGainDb);
        set_numeric(idTestFrequency, config.directionalTestFrequencyHz);
        updatingControls_ = false;
    }

    void run_selected_test() {
        const int target = read_combo_target(wnd_, idTestTarget, target_front_center);
        run_directional_test(target, Button_GetCheck(GetDlgItem(wnd_, idTestUseDynamicObject)) == BST_CHECKED, read_double(wnd_, idTestGain, -18.0), read_double(wnd_, idTestFrequency, 660.0), read_sample_rate_mode(wnd_));
    }

    static bool different(double a, double b) {
        return std::fabs(a - b) > 0.0001;
    }

    bool has_changed() const {
        const RuntimeConfig current = read_from_controls();
        if (current.layoutMode != initial_.layoutMode
            || current.sampleRateMode != initial_.sampleRateMode
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
            || different(current.lfeLowpassHz, initial_.lfeLowpassHz)) {
            return true;
        }

        for (size_t i = 0; i < target_count; ++i) {
            if (different(current.channelGainDb[i], initial_.channelGainDb[i])
                || different(current.channelDelayMs[i], initial_.channelDelayMs[i])) {
                return true;
            }
        }

        return current.map51FrontLeft != initial_.map51FrontLeft
            || current.map51FrontRight != initial_.map51FrontRight
            || current.map51FrontCenter != initial_.map51FrontCenter
            || current.map51Lfe != initial_.map51Lfe
            || current.map51SurroundLeft != initial_.map51SurroundLeft
            || current.map51SurroundRight != initial_.map51SurroundRight
            || current.directionalTestEnabled != initial_.directionalTestEnabled
            || current.directionalTestUseDynamicObject != initial_.directionalTestUseDynamicObject
            || current.directionalTestTarget != initial_.directionalTestTarget
            || different(current.directionalTestGainDb, initial_.directionalTestGainDb)
            || different(current.directionalTestFrequencyHz, initial_.directionalTestFrequencyHz);
    }

    HWND wnd_ = nullptr;
    preferences_page_callback::ptr callback_;
    RuntimeConfig initial_;
    fb2k::CCoreDarkModeHooks dark_;
    std::array<std::vector<HWND>, static_cast<size_t>(Page::Count)> pageControls_;
    std::vector<SliderBinding> sliders_;
    int selectedPage_ = 0;
    bool updatingControls_ = false;
};

class preferences_page_spatial_audio : public preferences_page_v3 {
public:
    const char* get_name() override {
        return "Spatial Audio";
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

static preferences_page_factory_t<preferences_page_spatial_audio> g_preferences_page_factory;

}  // namespace
}  // namespace spatial_audio
