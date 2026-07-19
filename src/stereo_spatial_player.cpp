#include <windows.h>
#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <functiondiscoverykeys_devpkey.h>
#include <spatialaudioclient.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

constexpr double kPi = 3.14159265358979323846;

struct StereoFrame {
    float left = 0.0f;
    float right = 0.0f;
};

struct WavData {
    uint32_t sampleRate = 0;
    std::vector<StereoFrame> frames;
};

struct ChannelDef {
    std::string key;
    AudioObjectType type;
};

struct CustomObjectConfig {
    std::string name;
    std::string source = "mid";
    double gainDb = -9.0;
    float x = 0.0f;
    float y = 0.0f;
    float z = -1.0f;
    std::string motion = "none";
};

struct UpmixConfig {
    double inputGainDb = 0.0;
    double frontGainDb = 0.0;
    double centerGainDb = -6.0;
    double lfeGainDb = -24.0;
    double surroundGainDb = -9.0;
    double rearGainDb = -12.0;
    double heightGainDb = -12.0;
    double sideAmount = 0.75;
    double heightFromMid = 0.20;
    double lfeLowpassHz = 120.0;
    bool enableLfe = false;
};

struct PlayerConfig {
    uint32_t sampleRate = 48000;
    double masterGainDb = -12.0;
    std::string staticBed = "7.1.4";
    std::map<std::string, double> channelGainsDb;
    std::vector<CustomObjectConfig> customObjects;
    UpmixConfig upmix;
};

struct RuntimeOptions {
    std::string wavPath;
    std::string configPath = "config\\spatial_audio_profile.ini";
    std::string mode = "bed";
    bool listDevices = false;
    std::wstring deviceQuery;
};

struct ObjectState {
    ComPtr<ISpatialAudioObject> object;
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
        {"front_left", AudioObjectType_FrontLeft},
        {"front_right", AudioObjectType_FrontRight},
        {"front_center", AudioObjectType_FrontCenter},
        {"low_frequency", AudioObjectType_LowFrequency},
        {"side_left", AudioObjectType_SideLeft},
        {"side_right", AudioObjectType_SideRight},
        {"back_left", AudioObjectType_BackLeft},
        {"back_right", AudioObjectType_BackRight},
        {"top_front_left", AudioObjectType_TopFrontLeft},
        {"top_front_right", AudioObjectType_TopFrontRight},
        {"top_back_left", AudioObjectType_TopBackLeft},
        {"top_back_right", AudioObjectType_TopBackRight},
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

float ClampSample(double value) {
    return static_cast<float>(std::clamp(value, -1.0, 1.0));
}

std::string Narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }

    std::string narrow(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), required, nullptr, nullptr);
    narrow.resize(static_cast<size_t>(required - 1));
    return narrow;
}

std::wstring WidenArg(const char* text) {
    if (text == nullptr || *text == '\0') {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (required <= 1) {
        return {};
    }

    std::wstring wide(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), required);
    wide.resize(static_cast<size_t>(required - 1));
    return wide;
}

std::wstring ToLowerWide(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(towlower(c));
    });
    return text;
}

void ThrowIfFailed(HRESULT hr, const char* action);

std::wstring DeviceName(IMMDevice* device) {
    ComPtr<IPropertyStore> store;
    if (device == nullptr || FAILED(device->OpenPropertyStore(STGM_READ, &store))) {
        return {};
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring result;
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR && value.pwszVal != nullptr) {
        result = value.pwszVal;
    }
    PropVariantClear(&value);
    return result;
}

struct DeviceInfo {
    std::wstring id;
    std::wstring name;
};

std::vector<DeviceInfo> EnumerateRenderDevices(IMMDeviceEnumerator* enumerator) {
    std::vector<DeviceInfo> devices;
    ComPtr<IMMDeviceCollection> collection;
    ThrowIfFailed(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection), "Enumerate render endpoints");
    UINT count = 0;
    ThrowIfFailed(collection->GetCount(&count), "Get endpoint count");
    for (UINT i = 0; i < count; ++i) {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, &device))) {
            continue;
        }

        LPWSTR id = nullptr;
        if (FAILED(device->GetId(&id)) || id == nullptr) {
            continue;
        }

        DeviceInfo info;
        info.id = id;
        info.name = DeviceName(device.Get());
        if (info.name.empty()) {
            info.name = id;
        }
        devices.push_back(info);
        CoTaskMemFree(id);
    }
    return devices;
}

bool FileExists(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

std::string JoinPath(const std::string& base, const std::string& relative) {
    if (base.empty()) {
        return relative;
    }

    const char last = base.back();
    if (last == '\\' || last == '/') {
        return base + relative;
    }
    return base + "\\" + relative;
}

std::string ExecutableDirectory() {
    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }

    std::wstring path(modulePath, length);
    const auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return {};
    }

    return Narrow(path.substr(0, slash).c_str());
}

std::string ResolveConfigPath(const std::string& requestedPath) {
    if (FileExists(requestedPath)) {
        return requestedPath;
    }

    const std::string exeDirectory = ExecutableDirectory();
    const std::vector<std::string> candidates = {
        "config\\spatial_audio_profile.ini",
        "spatial_audio_profile.ini",
        JoinPath(exeDirectory, "config\\spatial_audio_profile.ini"),
        JoinPath(exeDirectory, "spatial_audio_profile.ini"),
    };

    for (const auto& candidate : candidates) {
        if (candidate != requestedPath && FileExists(candidate)) {
            return candidate;
        }
    }

    return requestedPath;
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

uint16_t ReadU16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

uint32_t ReadU32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

std::vector<uint8_t> ReadFileBytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open WAV file: " + path);
    }

    file.seekg(0, std::ios::end);
    const auto length = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

float ReadPcmSample(const uint8_t* data, uint16_t bitsPerSample, uint16_t formatTag) {
    if (formatTag == 3 && bitsPerSample == 32) {
        float value = 0.0f;
        std::memcpy(&value, data, sizeof(float));
        return std::clamp(value, -1.0f, 1.0f);
    }

    if (formatTag != 1) {
        throw std::runtime_error("Only PCM int and IEEE float WAV files are supported.");
    }

    if (bitsPerSample == 16) {
        const int16_t sample = static_cast<int16_t>(ReadU16(data));
        return static_cast<float>(sample / 32768.0);
    }
    if (bitsPerSample == 24) {
        int32_t sample = static_cast<int32_t>(data[0] | (data[1] << 8) | (data[2] << 16));
        if ((sample & 0x800000) != 0) {
            sample |= ~0xFFFFFF;
        }
        return static_cast<float>(sample / 8388608.0);
    }
    if (bitsPerSample == 32) {
        const int32_t sample = static_cast<int32_t>(ReadU32(data));
        return static_cast<float>(sample / 2147483648.0);
    }

    throw std::runtime_error("Unsupported PCM bit depth.");
}

WavData LoadStereoWav(const std::string& path) {
    const std::vector<uint8_t> bytes = ReadFileBytes(path);
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        throw std::runtime_error("Not a RIFF/WAVE file.");
    }

    const uint8_t* fmt = nullptr;
    uint32_t fmtSize = 0;
    const uint8_t* data = nullptr;
    uint32_t dataSize = 0;

    size_t offset = 12;
    while (offset + 8 <= bytes.size()) {
        const uint8_t* chunk = bytes.data() + offset;
        const uint32_t chunkSize = ReadU32(chunk + 4);
        const size_t payload = offset + 8;
        if (payload + chunkSize > bytes.size()) {
            break;
        }

        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            fmt = bytes.data() + payload;
            fmtSize = chunkSize;
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            data = bytes.data() + payload;
            dataSize = chunkSize;
        }

        offset = payload + chunkSize + (chunkSize % 2);
    }

    if (fmt == nullptr || data == nullptr || fmtSize < 16) {
        throw std::runtime_error("WAV is missing fmt or data chunk.");
    }

    uint16_t formatTag = ReadU16(fmt);
    const uint16_t channels = ReadU16(fmt + 2);
    const uint32_t sampleRate = ReadU32(fmt + 4);
    const uint16_t blockAlign = ReadU16(fmt + 12);
    const uint16_t bitsPerSample = ReadU16(fmt + 14);

    if (formatTag == 0xFFFE && fmtSize >= 40) {
        formatTag = ReadU16(fmt + 24);
    }

    if (channels != 2) {
        throw std::runtime_error("Expected a stereo WAV file.");
    }

    const uint16_t bytesPerSample = static_cast<uint16_t>((bitsPerSample + 7) / 8);
    if (blockAlign < channels * bytesPerSample || blockAlign == 0) {
        throw std::runtime_error("Invalid WAV block alignment.");
    }

    WavData wav;
    wav.sampleRate = sampleRate;
    const size_t frameCount = dataSize / blockAlign;
    wav.frames.reserve(frameCount);

    for (size_t i = 0; i < frameCount; ++i) {
        const uint8_t* frame = data + (i * blockAlign);
        wav.frames.push_back({
            ReadPcmSample(frame, bitsPerSample, formatTag),
            ReadPcmSample(frame + bytesPerSample, bitsPerSample, formatTag),
        });
    }

    return wav;
}

std::map<std::string, std::map<std::string, std::string>> LoadIniValues(const std::string& path) {
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

        const auto key = ToLower(Trim(line.substr(0, equalsPos)));
        const auto value = Trim(line.substr(equalsPos + 1));
        values[section][key] = value;
    }

    return values;
}

std::string GetString(const std::map<std::string, std::map<std::string, std::string>>& values, const std::string& section, const std::string& key, const std::string& fallback) {
    const auto sectionIt = values.find(section);
    if (sectionIt == values.end()) {
        return fallback;
    }
    const auto keyIt = sectionIt->second.find(key);
    return keyIt == sectionIt->second.end() ? fallback : keyIt->second;
}

double GetDouble(const std::map<std::string, std::map<std::string, std::string>>& values, const std::string& section, const std::string& key, double fallback) {
    const auto text = GetString(values, section, key, "");
    if (text.empty()) {
        return fallback;
    }
    return std::stod(text);
}

bool GetBool(const std::map<std::string, std::map<std::string, std::string>>& values, const std::string& section, const std::string& key, bool fallback) {
    const auto text = ToLower(GetString(values, section, key, fallback ? "true" : "false"));
    return text == "1" || text == "true" || text == "yes" || text == "on";
}

std::string InferSource(const std::string& name) {
    const auto lower = ToLower(name);
    if (lower.find("left") != std::string::npos) {
        return "left";
    }
    if (lower.find("right") != std::string::npos) {
        return "right";
    }
    if (lower.find("ceiling") != std::string::npos || lower.find("height") != std::string::npos || lower.find("top") != std::string::npos) {
        return "ambience";
    }
    return "mid";
}

PlayerConfig LoadConfig(const std::string& path) {
    const auto values = LoadIniValues(path);
    PlayerConfig config;

    for (const auto& channel : AllChannels()) {
        config.channelGainsDb[channel.key] = 0.0;
    }

    config.sampleRate = static_cast<uint32_t>(GetDouble(values, "engine", "sample_rate", config.sampleRate));
    config.masterGainDb = GetDouble(values, "engine", "master_gain_db", config.masterGainDb);
    config.staticBed = ToLower(GetString(values, "layout", "static_bed", config.staticBed));

    for (const auto& channel : AllChannels()) {
        config.channelGainsDb[channel.key] = GetDouble(values, "channel_gains_db", channel.key, config.channelGainsDb[channel.key]);
    }

    config.upmix.inputGainDb = GetDouble(values, "stereo_upmix", "input_gain_db", config.upmix.inputGainDb);
    config.upmix.frontGainDb = GetDouble(values, "stereo_upmix", "front_gain_db", config.upmix.frontGainDb);
    config.upmix.centerGainDb = GetDouble(values, "stereo_upmix", "center_gain_db", config.upmix.centerGainDb);
    config.upmix.lfeGainDb = GetDouble(values, "stereo_upmix", "lfe_gain_db", config.upmix.lfeGainDb);
    config.upmix.surroundGainDb = GetDouble(values, "stereo_upmix", "surround_gain_db", config.upmix.surroundGainDb);
    config.upmix.rearGainDb = GetDouble(values, "stereo_upmix", "rear_gain_db", config.upmix.rearGainDb);
    config.upmix.heightGainDb = GetDouble(values, "stereo_upmix", "height_gain_db", config.upmix.heightGainDb);
    config.upmix.sideAmount = GetDouble(values, "stereo_upmix", "side_amount", config.upmix.sideAmount);
    config.upmix.heightFromMid = GetDouble(values, "stereo_upmix", "height_from_mid", config.upmix.heightFromMid);
    config.upmix.lfeLowpassHz = GetDouble(values, "stereo_upmix", "lfe_lowpass_hz", config.upmix.lfeLowpassHz);
    config.upmix.enableLfe = GetBool(values, "stereo_upmix", "enable_lfe", config.upmix.enableLfe);

    const int objectCount = static_cast<int>(GetDouble(values, "custom", "object_count", 0.0));
    for (int i = 1; i <= objectCount; ++i) {
        const std::string section = "custom_object." + std::to_string(i);
        CustomObjectConfig object;
        object.name = GetString(values, section, "name", "object_" + std::to_string(i));
        object.source = ToLower(GetString(values, section, "source", InferSource(object.name)));
        object.gainDb = GetDouble(values, section, "gain_db", object.gainDb);
        object.x = static_cast<float>(GetDouble(values, section, "x", object.x));
        object.y = static_cast<float>(GetDouble(values, section, "y", object.y));
        object.z = static_cast<float>(GetDouble(values, section, "z", object.z));
        object.motion = ToLower(GetString(values, section, "motion", object.motion));
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

AudioObjectType RequestedStaticMask(const PlayerConfig& config) {
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

    if (config.staticBed == "stereo") {
        include({"front_left", "front_right"});
    } else if (config.staticBed == "5.1") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right"});
    } else if (config.staticBed == "7.1") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "back_left", "back_right"});
    } else if (config.staticBed == "5.1.2") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "top_front_left", "top_front_right"});
    } else if (config.staticBed == "5.1.4") {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "top_front_left", "top_front_right", "top_back_left", "top_back_right"});
    } else {
        include({"front_left", "front_right", "front_center", "low_frequency", "side_left", "side_right", "back_left", "back_right", "top_front_left", "top_front_right", "top_back_left", "top_back_right"});
    }

    return mask;
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

ComPtr<IMMDevice> SelectRenderDevice(IMMDeviceEnumerator* enumerator, const std::wstring& query) {
    ComPtr<IMMDevice> device;
    if (query.empty()) {
        ThrowIfFailed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");
        return device;
    }

    const std::wstring queryLower = ToLowerWide(query);
    for (const auto& item : EnumerateRenderDevices(enumerator)) {
        const std::wstring nameLower = ToLowerWide(item.name);
        const std::wstring idLower = ToLowerWide(item.id);
        if (nameLower.find(queryLower) == std::wstring::npos && idLower.find(queryLower) == std::wstring::npos) {
            continue;
        }

        ThrowIfFailed(enumerator->GetDevice(item.id.c_str(), &device), "Get selected render endpoint");
        std::cout << "Selected endpoint: " << Narrow(item.name.c_str()) << "\n";
        return device;
    }
    throw std::runtime_error("No active render endpoint matches --device.");
}

void PrintRenderDevices(IMMDeviceEnumerator* enumerator) {
    std::cout << "Active render endpoints:\n";
    for (const auto& item : EnumerateRenderDevices(enumerator)) {
        std::cout << "  " << Narrow(item.name.c_str()) << "\n";
        std::cout << "    id: " << Narrow(item.id.c_str()) << "\n";
    }
}

ComPtr<ISpatialAudioClient> CreateSpatialClient(const std::wstring& deviceQuery) {
    ComPtr<IMMDeviceEnumerator> enumerator;
    ThrowIfFailed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

    ComPtr<IMMDevice> device = SelectRenderDevice(enumerator.Get(), deviceQuery);

    ComPtr<ISpatialAudioClient> spatialClient;
    ThrowIfFailed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");

    return spatialClient;
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

double SourceValue(const std::string& source, const StereoFrame& frame) {
    const double left = frame.left;
    const double right = frame.right;
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;

    if (source == "left") {
        return left;
    }
    if (source == "right") {
        return right;
    }
    if (source == "side") {
        return side;
    }
    if (source == "ambience" || source == "height") {
        return side;
    }
    return mid;
}

double BedValue(const std::string& key, const StereoFrame& frame, const UpmixConfig& upmix, uint32_t sampleRate, double& lfeState) {
    const double inputGain = DbToLinear(upmix.inputGainDb);
    const double left = frame.left * inputGain;
    const double right = frame.right * inputGain;
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5 * upmix.sideAmount;

    if (key == "front_left") {
        return left * DbToLinear(upmix.frontGainDb);
    }
    if (key == "front_right") {
        return right * DbToLinear(upmix.frontGainDb);
    }
    if (key == "front_center") {
        return mid * DbToLinear(upmix.centerGainDb);
    }
    if (key == "low_frequency") {
        if (!upmix.enableLfe) {
            return 0.0;
        }
        const double cutoffHz = std::clamp(upmix.lfeLowpassHz, 20.0, 250.0);
        const double alpha = 1.0 - std::exp((-2.0 * kPi * cutoffHz) / static_cast<double>(sampleRate));
        lfeState += alpha * (mid - lfeState);
        return lfeState * DbToLinear(upmix.lfeGainDb);
    }
    if (key == "side_left") {
        return side * DbToLinear(upmix.surroundGainDb);
    }
    if (key == "side_right") {
        return -side * DbToLinear(upmix.surroundGainDb);
    }
    if (key == "back_left") {
        return (side + left * 0.10) * DbToLinear(upmix.rearGainDb);
    }
    if (key == "back_right") {
        return (-side + right * 0.10) * DbToLinear(upmix.rearGainDb);
    }
    if (key == "top_front_left") {
        return (side + mid * upmix.heightFromMid) * DbToLinear(upmix.heightGainDb);
    }
    if (key == "top_front_right") {
        return (-side + mid * upmix.heightFromMid) * DbToLinear(upmix.heightGainDb);
    }
    if (key == "top_back_left") {
        return (side * 0.7 + mid * upmix.heightFromMid) * DbToLinear(upmix.heightGainDb);
    }
    if (key == "top_back_right") {
        return (-side * 0.7 + mid * upmix.heightFromMid) * DbToLinear(upmix.heightGainDb);
    }

    return 0.0;
}

void PlayBed(ISpatialAudioClient* spatialClient, const PlayerConfig& config, const WavData& wav) {
    const WAVEFORMATEX format = MakeObjectFormat(config.sampleRate);
    ThrowIfFailed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial audio object format");

    AudioObjectType nativeMask = AudioObjectType_None;
    ThrowIfFailed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

    const AudioObjectType requestedMask = RequestedStaticMask(config);
    AudioObjectType activeMask = AudioObjectType_None;
    std::vector<const ChannelDef*> activeChannels;

    for (const auto& channel : AllChannels()) {
        if (MaskContains(requestedMask, channel.type) && MaskContains(nativeMask, channel.type)) {
            activeMask = AddMask(activeMask, channel.type);
            activeChannels.push_back(&channel);
        }
    }

    if (activeChannels.empty()) {
        throw std::runtime_error("No requested static bed channels are supported by the current endpoint.");
    }

    UniqueHandle completionEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!completionEvent) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Create completion event");
    }

    auto stream = ActivateStream(spatialClient, format, activeMask, 0, 0, completionEvent.get());
    std::map<std::string, ObjectState> states;
    for (const auto* channel : activeChannels) {
        ThrowIfFailed(stream->ActivateSpatialAudioObject(channel->type, states[channel->key].object.GetAddressOf()), ("Activate " + channel->key).c_str());
    }

    ThrowIfFailed(stream->Start(), "Start spatial stream");

    size_t cursor = 0;
    double lfeState = 0.0;
    const double masterGain = DbToLinear(config.masterGainDb);

    while (cursor < wav.frames.size()) {
        if (WaitForSingleObject(completionEvent.get(), 1000) != WAIT_OBJECT_0) {
            throw std::runtime_error("Timed out waiting for the spatial audio buffer event.");
        }

        UINT32 availableDynamicObjects = 0;
        UINT32 frameCount = 0;
        ThrowIfFailed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

        for (const auto* channel : activeChannels) {
            auto& object = states[channel->key].object;
            BYTE* byteBuffer = nullptr;
            UINT32 bufferLength = 0;
            ThrowIfFailed(object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + channel->key).c_str());

            auto* samples = reinterpret_cast<float*>(byteBuffer);
            const UINT32 bufferFrames = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
            const double channelGain = DbToLinear(config.channelGainsDb.at(channel->key));

            for (UINT32 frameIndex = 0; frameIndex < bufferFrames; ++frameIndex) {
                const size_t sampleIndex = cursor + frameIndex;
                const StereoFrame frame = sampleIndex < wav.frames.size() ? wav.frames[sampleIndex] : StereoFrame{};
                samples[frameIndex] = ClampSample(BedValue(channel->key, frame, config.upmix, config.sampleRate, lfeState) * masterGain * channelGain);
            }
        }

        ThrowIfFailed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
        cursor += frameCount;
    }

    ThrowIfFailed(stream->Stop(), "Stop spatial stream");
    ThrowIfFailed(stream->Reset(), "Reset spatial stream");
}

void PlayObjects(ISpatialAudioClient* spatialClient, const PlayerConfig& config, const WavData& wav) {
    if (config.customObjects.empty()) {
        throw std::runtime_error("No custom objects are configured.");
    }

    const WAVEFORMATEX format = MakeObjectFormat(config.sampleRate);
    ThrowIfFailed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial audio object format");

    UINT32 maxDynamicObjectCount = 0;
    ThrowIfFailed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");
    if (maxDynamicObjectCount < config.customObjects.size()) {
        std::ostringstream stream;
        stream << "Endpoint exposes " << maxDynamicObjectCount << " dynamic objects, config requests " << config.customObjects.size() << ".";
        throw std::runtime_error(stream.str());
    }

    UniqueHandle completionEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!completionEvent) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Create completion event");
    }

    const UINT32 objectCount = static_cast<UINT32>(config.customObjects.size());
    auto stream = ActivateStream(spatialClient, format, AudioObjectType_None, objectCount, objectCount, completionEvent.get());
    std::vector<ObjectState> states(config.customObjects.size());

    ThrowIfFailed(stream->Start(), "Start spatial stream");

    size_t cursor = 0;
    const double masterGain = DbToLinear(config.masterGainDb + config.upmix.inputGainDb);

    while (cursor < wav.frames.size()) {
        if (WaitForSingleObject(completionEvent.get(), 1000) != WAIT_OBJECT_0) {
            throw std::runtime_error("Timed out waiting for the spatial audio buffer event.");
        }

        UINT32 availableDynamicObjects = 0;
        UINT32 frameCount = 0;
        ThrowIfFailed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");

        for (size_t i = 0; i < config.customObjects.size(); ++i) {
            const auto& objectConfig = config.customObjects[i];
            auto& state = states[i];
            if (!state.object) {
                ThrowIfFailed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, state.object.GetAddressOf()), ("Activate dynamic object " + objectConfig.name).c_str());
            }

            BYTE* byteBuffer = nullptr;
            UINT32 bufferLength = 0;
            ThrowIfFailed(state.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + objectConfig.name).c_str());

            auto* samples = reinterpret_cast<float*>(byteBuffer);
            const UINT32 bufferFrames = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
            const double gain = masterGain * DbToLinear(objectConfig.gainDb);

            for (UINT32 frameIndex = 0; frameIndex < bufferFrames; ++frameIndex) {
                const size_t sampleIndex = cursor + frameIndex;
                const StereoFrame frame = sampleIndex < wav.frames.size() ? wav.frames[sampleIndex] : StereoFrame{};
                samples[frameIndex] = ClampSample(SourceValue(objectConfig.source, frame) * gain);
            }

            float x = objectConfig.x;
            float y = objectConfig.y;
            float z = objectConfig.z;
            if (objectConfig.motion == "orbit") {
                const float radius = std::max(0.2f, static_cast<float>(std::sqrt((objectConfig.x * objectConfig.x) + (objectConfig.z * objectConfig.z))));
                const float angle = static_cast<float>((static_cast<double>(cursor) / config.sampleRate) * 0.45);
                x = std::cos(angle) * radius;
                z = std::sin(angle) * radius;
            }

            ThrowIfFailed(state.object->SetPosition(x, y, z), ("Set position for " + objectConfig.name).c_str());
            ThrowIfFailed(state.object->SetVolume(1.0f), ("Set volume for " + objectConfig.name).c_str());
        }

        ThrowIfFailed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
        cursor += frameCount;
    }

    ThrowIfFailed(stream->Stop(), "Stop spatial stream");
    ThrowIfFailed(stream->Reset(), "Reset spatial stream");
}

RuntimeOptions ParseOptions(int argc, char** argv) {
    RuntimeOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--wav" && i + 1 < argc) {
            options.wavPath = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            options.configPath = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            options.mode = ToLower(argv[++i]);
        } else if (arg == "--list-devices") {
            options.listDevices = true;
        } else if (arg == "--device" && i + 1 < argc) {
            options.deviceQuery = WidenArg(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  StereoSpatialPlayer --list-devices\n"
                << "  StereoSpatialPlayer --wav path --mode bed [--config path]\n"
                << "  StereoSpatialPlayer --wav path --mode objects [--config path]\n"
                << "  Add --device name-or-id to target a specific render endpoint.\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (!options.listDevices && options.wavPath.empty()) {
        throw std::runtime_error("--wav path is required.");
    }
    if (options.mode != "bed" && options.mode != "objects") {
        throw std::runtime_error("--mode must be bed or objects.");
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const RuntimeOptions options = ParseOptions(argc, argv);

        ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "Initialize COM");
        const auto uninitializeCom = std::unique_ptr<void, void (*)(void*)>(reinterpret_cast<void*>(1), [](void*) {
            CoUninitialize();
        });

        if (options.listDevices) {
            ComPtr<IMMDeviceEnumerator> enumerator;
            ThrowIfFailed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");
            PrintRenderDevices(enumerator.Get());
        }

        if (options.wavPath.empty()) return 0;

        const PlayerConfig config = LoadConfig(ResolveConfigPath(options.configPath));
        const WavData wav = LoadStereoWav(options.wavPath);
        if (wav.sampleRate != config.sampleRate) {
            std::ostringstream stream;
            stream << "WAV sample rate is " << wav.sampleRate << " Hz, config expects " << config.sampleRate << " Hz. Resampling is not implemented yet.";
            throw std::runtime_error(stream.str());
        }

        std::cout << "Loaded " << wav.frames.size() << " stereo frames at " << wav.sampleRate << " Hz.\n";
        std::cout << "Mode: " << options.mode << "\n";

        auto spatialClient = CreateSpatialClient(options.deviceQuery);
        if (options.mode == "bed") {
            PlayBed(spatialClient.Get(), config, wav);
        } else {
            PlayObjects(spatialClient.Get(), config, wav);
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
