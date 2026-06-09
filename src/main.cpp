#include <windows.h>
#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <spatialaudioclient.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

constexpr double kPi = 3.14159265358979323846;

struct ChannelDef {
    std::string key;
    std::string label;
    AudioObjectType type;
    double frequencyHz;
};

struct CustomObjectConfig {
    std::string name;
    double frequencyHz = 440.0;
    double gainDb = -6.0;
    float x = 0.0f;
    float y = 0.0f;
    float z = -1.0f;
    std::string motion = "none";
};

struct ExperimentConfig {
    uint32_t sampleRate = 48000;
    double durationSeconds = 10.0;
    double channelTestSeconds = 1.25;
    double masterGainDb = -18.0;
    std::string staticBed = "7.1.4";
    std::map<std::string, double> channelGainsDb;
    std::vector<CustomObjectConfig> customObjects;
};

struct RuntimeOptions {
    bool probe = false;
    bool staticTest = false;
    bool custom = false;
    std::string configPath = "config\\spatial_audio_profile.ini";
};

struct AudioObjectState {
    ComPtr<ISpatialAudioObject> object;
    double phase = 0.0;
};

class HandleCloser {
public:
    void operator()(HANDLE handle) const {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }
};

using UniqueHandle = std::unique_ptr<void, HandleCloser>;

const std::vector<ChannelDef>& AllChannels() {
    static const std::vector<ChannelDef> channels = {
        {"front_left", "Front Left", AudioObjectType_FrontLeft, 220.0},
        {"front_right", "Front Right", AudioObjectType_FrontRight, 247.0},
        {"front_center", "Front Center", AudioObjectType_FrontCenter, 277.0},
        {"low_frequency", "LFE", AudioObjectType_LowFrequency, 55.0},
        {"side_left", "Side Left", AudioObjectType_SideLeft, 311.0},
        {"side_right", "Side Right", AudioObjectType_SideRight, 349.0},
        {"back_left", "Back Left", AudioObjectType_BackLeft, 392.0},
        {"back_right", "Back Right", AudioObjectType_BackRight, 440.0},
        {"top_front_left", "Top Front Left", AudioObjectType_TopFrontLeft, 523.25},
        {"top_front_right", "Top Front Right", AudioObjectType_TopFrontRight, 587.33},
        {"top_back_left", "Top Back Left", AudioObjectType_TopBackLeft, 659.25},
        {"top_back_right", "Top Back Right", AudioObjectType_TopBackRight, 739.99},
    };
    return channels;
}

std::string Trim(std::string text) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [&](char c) { return !isSpace(static_cast<unsigned char>(c)); }));
    text.erase(std::find_if(text.rbegin(), text.rend(), [&](char c) { return !isSpace(static_cast<unsigned char>(c)); }).base(), text.end());
    return text;
}

std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

double DbToLinear(double db) {
    return std::pow(10.0, db / 20.0);
}

std::wstring Widen(std::string_view text) {
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring wide(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), required);
    return wide;
}

std::string Narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }

    std::string narrow(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), required, nullptr, nullptr);
    return narrow;
}

std::string HResultText(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) {
        stream << " (" << Narrow(message) << ")";
    }
    return stream.str();
}

void ThrowIfFailed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << HResultText(hr);
        throw std::runtime_error(stream.str());
    }
}

ExperimentConfig LoadConfig(const std::string& path) {
    ExperimentConfig config;

    for (const auto& channel : AllChannels()) {
        config.channelGainsDb[channel.key] = 0.0;
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    std::map<std::string, std::map<std::string, std::string>> values;
    std::string section;
    std::string line;
    while (std::getline(file, line)) {
        const auto commentPos = line.find_first_of("#;");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = ToLower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }

        const auto equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        auto key = ToLower(Trim(line.substr(0, equalsPos)));
        auto value = Trim(line.substr(equalsPos + 1));
        values[section][key] = value;
    }

    auto getString = [&](const std::string& sectionName, const std::string& key, const std::string& fallback) {
        const auto sectionIt = values.find(sectionName);
        if (sectionIt == values.end()) {
            return fallback;
        }
        const auto keyIt = sectionIt->second.find(key);
        return keyIt == sectionIt->second.end() ? fallback : keyIt->second;
    };

    auto getDouble = [&](const std::string& sectionName, const std::string& key, double fallback) {
        const auto text = getString(sectionName, key, "");
        if (text.empty()) {
            return fallback;
        }
        return std::stod(text);
    };

    config.sampleRate = static_cast<uint32_t>(getDouble("engine", "sample_rate", config.sampleRate));
    config.durationSeconds = getDouble("engine", "duration_seconds", config.durationSeconds);
    config.channelTestSeconds = getDouble("engine", "channel_test_seconds", config.channelTestSeconds);
    config.masterGainDb = getDouble("engine", "master_gain_db", config.masterGainDb);
    config.staticBed = ToLower(getString("layout", "static_bed", config.staticBed));

    for (const auto& channel : AllChannels()) {
        config.channelGainsDb[channel.key] = getDouble("channel_gains_db", channel.key, 0.0);
    }

    const int objectCount = static_cast<int>(getDouble("custom", "object_count", 0.0));
    for (int i = 1; i <= objectCount; ++i) {
        const std::string sectionName = "custom_object." + std::to_string(i);
        CustomObjectConfig object;
        object.name = getString(sectionName, "name", "object_" + std::to_string(i));
        object.frequencyHz = getDouble(sectionName, "frequency_hz", object.frequencyHz);
        object.gainDb = getDouble(sectionName, "gain_db", object.gainDb);
        object.x = static_cast<float>(getDouble(sectionName, "x", object.x));
        object.y = static_cast<float>(getDouble(sectionName, "y", object.y));
        object.z = static_cast<float>(getDouble(sectionName, "z", object.z));
        object.motion = ToLower(getString(sectionName, "motion", object.motion));
        config.customObjects.push_back(object);
    }

    return config;
}

AudioObjectType AddMask(AudioObjectType mask, AudioObjectType value) {
    return static_cast<AudioObjectType>(static_cast<uint32_t>(mask) | static_cast<uint32_t>(value));
}

bool MaskContains(AudioObjectType mask, AudioObjectType value) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(value)) == static_cast<uint32_t>(value);
}

AudioObjectType RequestedStaticMask(const ExperimentConfig& config) {
    AudioObjectType mask = AudioObjectType_None;

    auto include = [&](std::initializer_list<std::string_view> keys) {
        for (const auto key : keys) {
            const auto it = std::find_if(AllChannels().begin(), AllChannels().end(), [&](const ChannelDef& channel) {
                return channel.key == key;
            });
            if (it != AllChannels().end()) {
                mask = AddMask(mask, it->type);
            }
        }
    };

    const std::string bed = ToLower(config.staticBed);
    if (bed == "stereo") {
        include({"front_left", "front_right"});
    } else if (bed == "5.1") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right"});
    } else if (bed == "7.1") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "back_left", "back_right"});
    } else if (bed == "5.1.2") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "top_front_left", "top_front_right"});
    } else {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "back_left", "back_right", "top_front_left", "top_front_right", "top_back_left", "top_back_right"});
    }

    return mask;
}

std::string ChannelMaskText(AudioObjectType mask) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& channel : AllChannels()) {
        if (!MaskContains(mask, channel.type)) {
            continue;
        }
        if (!first) {
            stream << ", ";
        }
        stream << channel.key;
        first = false;
    }
    if (first) {
        return "none";
    }
    return stream.str();
}

RuntimeOptions ParseOptions(int argc, char** argv) {
    RuntimeOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--probe") {
            options.probe = true;
        } else if (arg == "--static-test") {
            options.staticTest = true;
        } else if (arg == "--custom") {
            options.custom = true;
        } else if (arg == "--config" && i + 1 < argc) {
            options.configPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  SpatialAudioHomeTheaterExperiment --probe [--config path]\n"
                << "  SpatialAudioHomeTheaterExperiment --static-test [--config path]\n"
                << "  SpatialAudioHomeTheaterExperiment --custom [--config path]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (!options.probe && !options.staticTest && !options.custom) {
        options.probe = true;
    }
    return options;
}

WAVEFORMATEX MakeObjectFormat(uint32_t sampleRate) {
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 1;
    format.nSamplesPerSec = sampleRate;
    format.wBitsPerSample = 32;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;
    return format;
}

ComPtr<ISpatialAudioClient> CreateSpatialClient() {
    ComPtr<IMMDeviceEnumerator> enumerator;
    ThrowIfFailed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

    ComPtr<IMMDevice> device;
    ThrowIfFailed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");

    ComPtr<ISpatialAudioClient> spatialClient;
    ThrowIfFailed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");

    return spatialClient;
}

void ProbeSpatialAudio(ISpatialAudioClient* spatialClient, const ExperimentConfig& config) {
    WAVEFORMATEX format = MakeObjectFormat(config.sampleRate);

    std::cout << "Object format: float32 mono " << config.sampleRate << " Hz\n";

    const HRESULT formatHr = spatialClient->IsAudioObjectFormatSupported(&format);
    std::cout << "Format supported: " << (SUCCEEDED(formatHr) ? "yes" : "no") << " - " << HResultText(formatHr) << "\n";

    AudioObjectType nativeMask = AudioObjectType_None;
    ThrowIfFailed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");
    std::cout << "Native static bed: " << ChannelMaskText(nativeMask) << "\n";

    UINT32 maxDynamicObjectCount = 0;
    ThrowIfFailed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");
    std::cout << "Max dynamic objects: " << maxDynamicObjectCount << "\n";

    const AudioObjectType requestedMask = RequestedStaticMask(config);
    std::cout << "Requested static bed: " << ChannelMaskText(requestedMask) << "\n";

    AudioObjectType missingMask = AudioObjectType_None;
    for (const auto& channel : AllChannels()) {
        if (MaskContains(requestedMask, channel.type) && !MaskContains(nativeMask, channel.type)) {
            missingMask = AddMask(missingMask, channel.type);
        }
    }

    if (missingMask != AudioObjectType_None) {
        std::cout << "Missing from native bed: " << ChannelMaskText(missingMask) << "\n";
    }
}

ComPtr<ISpatialAudioObjectRenderStream> ActivateStream(
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
    ThrowIfFailed(
        spatialClient->ActivateSpatialAudioStream(&activationParams, __uuidof(ISpatialAudioObjectRenderStream), reinterpret_cast<void**>(stream.GetAddressOf())),
        "Activate spatial audio stream");

    return stream;
}

void FillSine(float* buffer, UINT32 frames, double& phase, double frequencyHz, uint32_t sampleRate, double gain) {
    const double increment = 2.0 * kPi * frequencyHz / static_cast<double>(sampleRate);
    for (UINT32 i = 0; i < frames; ++i) {
        buffer[i] = static_cast<float>(std::sin(phase) * gain);
        phase += increment;
        if (phase >= 2.0 * kPi) {
            phase -= 2.0 * kPi;
        }
    }
}

void FillSilence(float* buffer, UINT32 frames) {
    std::fill(buffer, buffer + frames, 0.0f);
}

void RunStaticBedTest(ISpatialAudioClient* spatialClient, const ExperimentConfig& config) {
    const WAVEFORMATEX format = MakeObjectFormat(config.sampleRate);
    ThrowIfFailed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

    AudioObjectType nativeMask = AudioObjectType_None;
    ThrowIfFailed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

    const AudioObjectType requestedMask = RequestedStaticMask(config);
    AudioObjectType activeMask = AudioObjectType_None;
    for (const auto& channel : AllChannels()) {
        if (MaskContains(requestedMask, channel.type) && MaskContains(nativeMask, channel.type)) {
            activeMask = AddMask(activeMask, channel.type);
        }
    }

    if (activeMask == AudioObjectType_None) {
        throw std::runtime_error("No requested static channels are supported by the current endpoint.");
    }

    UniqueHandle completionEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!completionEvent) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Create buffer completion event");
    }

    auto stream = ActivateStream(spatialClient, format, activeMask, 0, 0, completionEvent.get());

    std::vector<const ChannelDef*> activeChannels;
    std::map<std::string, AudioObjectState> states;
    for (const auto& channel : AllChannels()) {
        if (!MaskContains(activeMask, channel.type)) {
            continue;
        }

        states[channel.key] = AudioObjectState{};
        activeChannels.push_back(&channel);
    }

    std::cout << "Static test channels: " << ChannelMaskText(activeMask) << "\n";
    std::cout << "Listen for each named channel, especially top_front/top_back speakers.\n";

    ThrowIfFailed(stream->Start(), "Start spatial stream");

    const double totalSeconds = std::max(config.durationSeconds, config.channelTestSeconds * activeChannels.size());
    double renderedSeconds = 0.0;

    while (renderedSeconds < totalSeconds) {
        if (WaitForSingleObject(completionEvent.get(), 1000) != WAIT_OBJECT_0) {
            throw std::runtime_error("Timed out waiting for the spatial audio buffer event.");
        }

        UINT32 availableDynamicObjects = 0;
        UINT32 frameCount = 0;
        ThrowIfFailed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

        const size_t activeIndex = std::min(static_cast<size_t>(renderedSeconds / config.channelTestSeconds), activeChannels.size() - 1);
        const auto* activeChannel = activeChannels[activeIndex];

        for (const auto* channel : activeChannels) {
            auto& state = states[channel->key];

            if (!state.object) {
                ThrowIfFailed(stream->ActivateSpatialAudioObject(channel->type, state.object.GetAddressOf()), ("Activate static channel " + channel->key).c_str());
            }

            BYTE* byteBuffer = nullptr;
            UINT32 bufferLength = 0;
            ThrowIfFailed(state.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + channel->key).c_str());

            auto* sampleBuffer = reinterpret_cast<float*>(byteBuffer);
            const UINT32 bufferFrames = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));

            if (channel == activeChannel) {
                const double channelGainDb = config.channelGainsDb.at(channel->key);
                FillSine(sampleBuffer, bufferFrames, state.phase, channel->frequencyHz, config.sampleRate, DbToLinear(config.masterGainDb + channelGainDb));
            } else {
                FillSilence(sampleBuffer, bufferFrames);
            }
        }

        ThrowIfFailed(stream->EndUpdatingAudioObjects(), "End updating audio objects");

        const double nextRenderedSeconds = renderedSeconds + (static_cast<double>(frameCount) / static_cast<double>(config.sampleRate));
        const auto previousIndex = static_cast<size_t>(renderedSeconds / config.channelTestSeconds);
        const auto nextIndex = static_cast<size_t>(nextRenderedSeconds / config.channelTestSeconds);
        if (nextIndex != previousIndex && nextIndex < activeChannels.size()) {
            std::cout << "Now testing: " << activeChannels[nextIndex]->key << "\n";
        }
        renderedSeconds = nextRenderedSeconds;
    }

    ThrowIfFailed(stream->Stop(), "Stop spatial stream");
    ThrowIfFailed(stream->Reset(), "Reset spatial stream");
}

void RunCustomObjectTest(ISpatialAudioClient* spatialClient, const ExperimentConfig& config) {
    if (config.customObjects.empty()) {
        throw std::runtime_error("No custom objects are configured.");
    }

    const WAVEFORMATEX format = MakeObjectFormat(config.sampleRate);
    ThrowIfFailed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

    UINT32 maxDynamicObjectCount = 0;
    ThrowIfFailed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");
    if (maxDynamicObjectCount < config.customObjects.size()) {
        std::ostringstream stream;
        stream << "The endpoint exposes only " << maxDynamicObjectCount
               << " dynamic objects, but the profile requests " << config.customObjects.size() << ".";
        throw std::runtime_error(stream.str());
    }

    UniqueHandle completionEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!completionEvent) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Create buffer completion event");
    }

    const auto requestedObjects = static_cast<UINT32>(config.customObjects.size());
    auto stream = ActivateStream(spatialClient, format, AudioObjectType_None, requestedObjects, requestedObjects, completionEvent.get());

    std::vector<AudioObjectState> states(config.customObjects.size());

    std::cout << "Custom dynamic objects:\n";
    for (const auto& object : config.customObjects) {
        std::cout << "  " << object.name << " at x=" << object.x << ", y=" << object.y << ", z=" << object.z << ", gain=" << object.gainDb << " dB\n";
    }

    ThrowIfFailed(stream->Start(), "Start spatial stream");

    double renderedSeconds = 0.0;
    while (renderedSeconds < config.durationSeconds) {
        if (WaitForSingleObject(completionEvent.get(), 1000) != WAIT_OBJECT_0) {
            throw std::runtime_error("Timed out waiting for the spatial audio buffer event.");
        }

        UINT32 availableDynamicObjects = 0;
        UINT32 frameCount = 0;
        ThrowIfFailed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

        for (size_t i = 0; i < config.customObjects.size(); ++i) {
            auto& state = states[i];
            const auto& objectConfig = config.customObjects[i];

            if (!state.object) {
                ThrowIfFailed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, state.object.GetAddressOf()), ("Activate dynamic object " + objectConfig.name).c_str());
            }

            BYTE* byteBuffer = nullptr;
            UINT32 bufferLength = 0;
            ThrowIfFailed(state.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + objectConfig.name).c_str());

            auto* sampleBuffer = reinterpret_cast<float*>(byteBuffer);
            const UINT32 bufferFrames = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
            FillSine(sampleBuffer, bufferFrames, state.phase, objectConfig.frequencyHz, config.sampleRate, DbToLinear(config.masterGainDb + objectConfig.gainDb));

            float x = objectConfig.x;
            float y = objectConfig.y;
            float z = objectConfig.z;
            if (objectConfig.motion == "orbit") {
                const float radius = std::max(0.2f, static_cast<float>(std::sqrt((objectConfig.x * objectConfig.x) + (objectConfig.z * objectConfig.z))));
                const float angle = static_cast<float>(renderedSeconds * 0.65);
                x = std::cos(angle) * radius;
                z = std::sin(angle) * radius;
            }

            ThrowIfFailed(state.object->SetPosition(x, y, z), ("Set position for " + objectConfig.name).c_str());
            ThrowIfFailed(state.object->SetVolume(1.0f), ("Set volume for " + objectConfig.name).c_str());
        }

        ThrowIfFailed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
        renderedSeconds += static_cast<double>(frameCount) / static_cast<double>(config.sampleRate);
    }

    ThrowIfFailed(stream->Stop(), "Stop spatial stream");
    ThrowIfFailed(stream->Reset(), "Reset spatial stream");
}

} // namespace

int main(int argc, char** argv) {
    try {
        const RuntimeOptions options = ParseOptions(argc, argv);
        const ExperimentConfig config = LoadConfig(options.configPath);

        ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "Initialize COM");
        const auto uninitializeCom = std::unique_ptr<void, void (*)(void*)>(reinterpret_cast<void*>(1), [](void*) {
            CoUninitialize();
        });

        auto spatialClient = CreateSpatialClient();

        if (options.probe) {
            ProbeSpatialAudio(spatialClient.Get(), config);
        }
        if (options.staticTest) {
            RunStaticBedTest(spatialClient.Get(), config);
        }
        if (options.custom) {
            RunCustomObjectTest(spatialClient.Get(), config);
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
