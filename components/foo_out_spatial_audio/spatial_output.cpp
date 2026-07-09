#include "stdafx.h"
#include "spatial_output.h"

using Microsoft::WRL::ComPtr;

namespace spatial_audio {
namespace {

constexpr double kPi = 3.14159265358979323846;

static constexpr GUID guid_output       = { 0xb588936e, 0x8925, 0x4874, { 0x82, 0xf4, 0x6a, 0x1d, 0x71, 0x08, 0x6d, 0xf3 } };
static constexpr GUID guid_device_default = { 0xb5e19e29, 0x189e, 0x4a8b, { 0xad, 0x94, 0x28, 0x7c, 0xba, 0xa8, 0x10, 0x94 } };

struct ChannelDef {
    const char* key;
    AudioObjectType type;
};

struct EndpointInfo {
    GUID guid = {};
    std::wstring id;
    std::string name;
};

const std::vector<ChannelDef>& all_channels() {
    static const std::vector<ChannelDef> channels = {
        {"front_left",       AudioObjectType_FrontLeft},
        {"front_right",      AudioObjectType_FrontRight},
        {"front_center",     AudioObjectType_FrontCenter},
        {"low_frequency",    AudioObjectType_LowFrequency},
        {"side_left",        AudioObjectType_SideLeft},
        {"side_right",       AudioObjectType_SideRight},
        {"back_left",        AudioObjectType_BackLeft},
        {"back_right",       AudioObjectType_BackRight},
        {"top_front_left",   AudioObjectType_TopFrontLeft},
        {"top_front_right",  AudioObjectType_TopFrontRight},
        {"top_back_left",    AudioObjectType_TopBackLeft},
        {"top_back_right",   AudioObjectType_TopBackRight},
    };
    return channels;
}

AudioObjectType add_mask(AudioObjectType mask, AudioObjectType value) {
    return static_cast<AudioObjectType>(static_cast<uint32_t>(mask) | static_cast<uint32_t>(value));
}

bool mask_contains(AudioObjectType mask, AudioObjectType value) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(value)) == static_cast<uint32_t>(value);
}

unsigned target_channel_flag(size_t target) {
    static constexpr unsigned flags[target_count] = {
        audio_chunk::channel_front_left,
        audio_chunk::channel_front_right,
        audio_chunk::channel_front_center,
        audio_chunk::channel_lfe,
        audio_chunk::channel_side_left,
        audio_chunk::channel_side_right,
        audio_chunk::channel_back_left,
        audio_chunk::channel_back_right,
        audio_chunk::channel_top_front_left,
        audio_chunk::channel_top_front_right,
        audio_chunk::channel_top_back_left,
        audio_chunk::channel_top_back_right,
        audio_chunk::channel_front_center_left,
        audio_chunk::channel_front_center_right,
    };
    return target < target_count ? flags[target] : 0;
}

unsigned fallback_channel_mask(unsigned channels) {
    switch (channels) {
    case 1:
        return audio_chunk::channel_config_mono;
    case 2:
        return audio_chunk::channel_config_stereo;
    case 6:
        return audio_chunk::channel_config_5point1_side;
    case 8:
        return audio_chunk::channel_config_7point1;
    case 10:
        return audio_chunk::channel_config_5point1_side
            | audio_chunk::channel_top_front_left
            | audio_chunk::channel_top_front_right
            | audio_chunk::channel_top_back_left
            | audio_chunk::channel_top_back_right;
    case 12:
        return audio_chunk::channel_config_7point1
            | audio_chunk::channel_top_front_left
            | audio_chunk::channel_top_front_right
            | audio_chunk::channel_top_back_left
            | audio_chunk::channel_top_back_right;
    case 14:
        return audio_chunk::channel_config_7point1
            | audio_chunk::channel_front_center_left
            | audio_chunk::channel_front_center_right
            | audio_chunk::channel_top_front_left
            | audio_chunk::channel_top_front_right
            | audio_chunk::channel_top_back_left
            | audio_chunk::channel_top_back_right;
    default:
        return audio_chunk::g_guess_channel_config(channels);
    }
}

unsigned normalized_channel_mask(unsigned channels, unsigned mask) {
    if (channels == 0 || channels > 32) return 0;
    if (mask != 0 && audio_chunk::g_count_channels(mask) == channels) return mask;
    return fallback_channel_mask(channels);
}

AudioObjectType object_type_from_channel_flag(unsigned flag) {
    switch (flag) {
    case audio_chunk::channel_front_left:      return AudioObjectType_FrontLeft;
    case audio_chunk::channel_front_right:     return AudioObjectType_FrontRight;
    case audio_chunk::channel_front_center:    return AudioObjectType_FrontCenter;
    case audio_chunk::channel_lfe:             return AudioObjectType_LowFrequency;
    case audio_chunk::channel_side_left:       return AudioObjectType_SideLeft;
    case audio_chunk::channel_side_right:      return AudioObjectType_SideRight;
    case audio_chunk::channel_back_left:       return AudioObjectType_BackLeft;
    case audio_chunk::channel_back_right:      return AudioObjectType_BackRight;
    case audio_chunk::channel_top_front_left:  return AudioObjectType_TopFrontLeft;
    case audio_chunk::channel_top_front_right: return AudioObjectType_TopFrontRight;
    case audio_chunk::channel_top_back_left:   return AudioObjectType_TopBackLeft;
    case audio_chunk::channel_top_back_right:  return AudioObjectType_TopBackRight;
    default: return AudioObjectType_None;
    }
}

AudioObjectType object_mask_from_channel_mask(unsigned channelMask) {
    AudioObjectType mask = AudioObjectType_None;
    for (unsigned bit = 0; bit < 32; ++bit) {
        const unsigned flag = 1u << bit;
        if ((channelMask & flag) == 0) continue;
        const AudioObjectType type = object_type_from_channel_flag(flag);
        if (type != AudioObjectType_None) mask = add_mask(mask, type);
    }
    return mask;
}

AudioObjectType audio_bed_mask(unsigned channels, unsigned mask) {
    if (channels == 0) return AudioObjectType_None;
    return object_mask_from_channel_mask(normalized_channel_mask(channels, mask));
}

std::string narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string narrowText(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrowText.data(), required, nullptr, nullptr);
    narrowText.resize(static_cast<size_t>(required - 1));
    return narrowText;
}

std::string endpoint_hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) stream << " (" << narrow(message) << ")";
    return stream.str();
}

void endpoint_throw_if_failed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << endpoint_hresult_text(hr);
        throw std::runtime_error(stream.str());
    }
}

uint64_t fnv1a64(const wchar_t* text, uint64_t seed) {
    uint64_t hash = seed;
    while (text != nullptr && *text != L'\0') {
        const wchar_t ch = *text++;
        hash ^= static_cast<uint8_t>(ch & 0xff);
        hash *= 1099511628211ull;
        hash ^= static_cast<uint8_t>((ch >> 8) & 0xff);
        hash *= 1099511628211ull;
    }
    return hash;
}

GUID endpoint_guid_from_id(const wchar_t* endpointId) {
    const uint64_t a = fnv1a64(endpointId, 14695981039346656037ull);
    const uint64_t b = fnv1a64(endpointId, 1099511628211ull);
    GUID guid = {};
    guid.Data1 = static_cast<unsigned long>(a & 0xffffffffu);
    guid.Data2 = static_cast<unsigned short>((a >> 32) & 0xffffu);
    guid.Data3 = static_cast<unsigned short>(((a >> 48) & 0x0fffu) | 0x5000u);
    guid.Data4[0] = static_cast<unsigned char>(((b >> 0) & 0x3fu) | 0x80u);
    guid.Data4[1] = static_cast<unsigned char>((b >> 8) & 0xffu);
    guid.Data4[2] = static_cast<unsigned char>((b >> 16) & 0xffu);
    guid.Data4[3] = static_cast<unsigned char>((b >> 24) & 0xffu);
    guid.Data4[4] = static_cast<unsigned char>((b >> 32) & 0xffu);
    guid.Data4[5] = static_cast<unsigned char>((b >> 40) & 0xffu);
    guid.Data4[6] = static_cast<unsigned char>((b >> 48) & 0xffu);
    guid.Data4[7] = static_cast<unsigned char>((b >> 56) & 0xffu);
    return guid;
}

std::string endpoint_name(IMMDevice* device) {
    ComPtr<IPropertyStore> store;
    if (device == nullptr || FAILED(device->OpenPropertyStore(STGM_READ, &store))) return {};
    PROPVARIANT value;
    PropVariantInit(&value);
    std::string result;
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR)
        result = narrow(value.pwszVal);
    PropVariantClear(&value);
    return result;
}

std::vector<EndpointInfo> enumerate_render_endpoints(IMMDeviceEnumerator* enumerator) {
    std::vector<EndpointInfo> endpoints;
    if (enumerator == nullptr) return endpoints;

    ComPtr<IMMDeviceCollection> collection;
    if (FAILED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection))) return endpoints;

    UINT count = 0;
    if (FAILED(collection->GetCount(&count))) return endpoints;
    for (UINT i = 0; i < count; ++i) {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, &device))) continue;

        LPWSTR id = nullptr;
        if (FAILED(device->GetId(&id)) || id == nullptr) continue;

        EndpointInfo info;
        info.id = id;
        info.guid = endpoint_guid_from_id(id);
        info.name = endpoint_name(device.Get());
        if (info.name.empty()) info.name = narrow(id);
        endpoints.push_back(info);
        CoTaskMemFree(id);
    }
    return endpoints;
}

ComPtr<IMMDevice> get_render_endpoint(IMMDeviceEnumerator* enumerator, const GUID& requestedGuid) {
    ComPtr<IMMDevice> device;
    if (enumerator == nullptr) return device;
    if (requestedGuid == guid_device_default || requestedGuid == GUID_NULL) {
        endpoint_throw_if_failed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");
        return device;
    }

    for (const auto& endpoint : enumerate_render_endpoints(enumerator)) {
        if (!(endpoint.guid == requestedGuid)) continue;
        endpoint_throw_if_failed(enumerator->GetDevice(endpoint.id.c_str(), &device), "Get selected render endpoint");
        return device;
    }
    throw exception_output_device_not_found();
}

}  // namespace

spatial_audio_output::spatial_audio_output(const GUID& device, double bufferLength, bool, t_uint32)
    : config_(ReadConfig()), device_(device), bufferLength_(std::max(bufferLength, 0.2)) {
    wakeEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (wakeEvent_ == nullptr) throw exception_output_device_not_found();
}

spatial_audio_output::~spatial_audio_output() {
    stop_stream();
    if (wakeEvent_ != nullptr) { CloseHandle(wakeEvent_); wakeEvent_ = nullptr; }
}

GUID spatial_audio_output::g_get_guid() { return guid_output; }
const char* spatial_audio_output::g_get_name() { return "Spatial Audio Output"; }

void spatial_audio_output::g_enum_devices(output_device_enum_callback& callback) {
    const char name[] = "Default Windows Spatial Audio endpoint";
    callback.on_device(guid_device_default, name, sizeof(name) - 1);

    HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitialize = SUCCEEDED(coInit);
    if (FAILED(coInit) && coInit != RPC_E_CHANGED_MODE) return;
    const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(shouldUninitialize ? reinterpret_cast<void*>(1) : nullptr, [](void* marker) {
        if (marker != nullptr) CoUninitialize();
    });

    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");
        for (const auto& endpoint : enumerate_render_endpoints(enumerator.Get())) {
            callback.on_device(endpoint.guid, endpoint.name.c_str(), static_cast<unsigned>(endpoint.name.size()));
        }
    } catch (...) {
    }
}

bool spatial_audio_output::g_advanced_settings_query()   { return false; }
bool spatial_audio_output::g_needs_bitdepth_config()     { return false; }
bool spatial_audio_output::g_needs_dither_config()       { return false; }
bool spatial_audio_output::g_needs_device_list_prefixes(){ return false; }
bool spatial_audio_output::g_supports_multiple_streams() { return false; }
bool spatial_audio_output::g_is_high_latency()           { return false; }

unsigned spatial_audio_output::get_forced_sample_rate() { return forced_sample_rate(ReadConfig(), device_); }
unsigned spatial_audio_output::get_forced_channel_mask() { return 0; }

void spatial_audio_output::pause(bool state) {
    { std::lock_guard<std::mutex> lock(mutex_); paused_ = state; }
    SetEvent(wakeEvent_);
}

void spatial_audio_output::volume_set(double value) { volumeDb_.store(value); }

bool spatial_audio_output::is_progressing() {
    std::lock_guard<std::mutex> lock(mutex_);
    return started_ && !paused_;
}

pfc::eventHandle_t spatial_audio_output::get_trigger_event() { return wakeEvent_; }

void spatial_audio_output::on_update() {
    const OutputConfig next = ReadConfig();
    bool needsReopen = false;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        needsReopen = stream_shape_changed(config_, next);
        config_ = next;
    }
    bool renderStopped = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        renderStopped = renderThread_.joinable() && !started_;
    }
    if (needsReopen || renderStopped) {
        on_need_reopen();
        SetEvent(wakeEvent_);
    }
}

void spatial_audio_output::open(audio_chunk::spec_t const& spec) {
    const OutputConfig nextConfig = ReadConfig();
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config_ = nextConfig;
    }
    const uint32_t configuredRate = forced_sample_rate(nextConfig, device_);
    const bool rateMatchesConfig  = configuredRate == 0 || spec.sampleRate == configuredRate;
    if (!rateMatchesConfig || !spatial_sample_rate_supported(device_, spec.sampleRate) || spec.chanCount < 2) {
        throw exception_output_unsupported_stream_format();
    }
    stop_stream();
    sampleRate_     = spec.sampleRate;
    capacityFrames_ = static_cast<size_t>(std::max(0.2, bufferLength_) * static_cast<double>(sampleRate_));
    clear_queue();
    const unsigned detectedChannelMask = normalized_channel_mask(spec.chanCount, spec.chanMask);
    const AudioObjectType detectedBedMask = object_mask_from_channel_mask(detectedChannelMask);
    start_stream(sampleRate_, detectedBedMask, detectedChannelMask);
}

void spatial_audio_output::write(const audio_chunk& data) {
    const size_t frameCount = data.get_sample_count();
    const unsigned channels = data.get_channel_count();
    const unsigned mask     = data.get_channel_config();
    const audio_sample* samples = data.get_data();
    if (samples == nullptr || channels < 2 || frameCount == 0) return;

    std::lock_guard<std::mutex> lock(mutex_);
    const size_t free    = capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
    const size_t toCopy  = std::min(frameCount, free);
    const unsigned sourceMask = normalized_channel_mask(channels, mask);
    const bool useMaskedChannelOrder = sourceMask != 0
        && audio_chunk::g_count_channels(sourceMask) == channels;
    for (size_t i = 0; i < toCopy; ++i) {
        std::array<float, target_count> frame = {};
        if (useMaskedChannelOrder) {
            for (size_t ch = 0; ch < target_count; ++ch) {
                const unsigned index = audio_chunk::g_channel_index_from_flag(sourceMask, target_channel_flag(ch));
                if (index != static_cast<unsigned>(-1) && index < channels)
                    frame[ch] = static_cast<float>(samples[i * channels + index]);
            }
        } else if (channels == target_count) {
            for (size_t ch = 0; ch < target_count; ++ch) {
                frame[ch] = static_cast<float>(samples[i * channels + ch]);
            }
        } else if (channels >= 2) {
            frame[0] = static_cast<float>(samples[i * channels + 0]);
            frame[1] = static_cast<float>(samples[i * channels + 1]);
        }
        queue_.push_back(frame);
    }
    queuedFrames_.store(queue_.size());
    SetEvent(wakeEvent_);
}

t_size spatial_audio_output::can_write_samples() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) return 0;
    return capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
}

t_size spatial_audio_output::get_latency_samples() {
    return queuedFrames_.load();
}

void spatial_audio_output::on_flush()      { clear_queue(); SetEvent(wakeEvent_); }
void spatial_audio_output::on_force_play() { SetEvent(wakeEvent_); }

void spatial_audio_output::start_stream(uint32_t sampleRate, AudioObjectType audioBedMask, unsigned audioChannelMask) {
    spatialEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (spatialEvent_ == nullptr) throw_if_failed(HRESULT_FROM_WIN32(GetLastError()), "Create spatial completion event");
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = false; paused_ = false; started_ = true; }
    renderThread_ = std::thread([this, sampleRate, audioBedMask, audioChannelMask]() { render_loop(sampleRate, audioBedMask, audioChannelMask); });
}

void spatial_audio_output::stop_stream() {
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; started_ = false; }
    SetEvent(wakeEvent_);
    if (spatialEvent_ != nullptr) SetEvent(spatialEvent_);
    if (renderThread_.joinable()) renderThread_.join();
    if (spatialEvent_ != nullptr) { CloseHandle(spatialEvent_); spatialEvent_ = nullptr; }
    clear_queue();
}

void spatial_audio_output::clear_queue() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    queuedFrames_.store(0);
    lastRenderFrames_.store(0);
}

WAVEFORMATEX spatial_audio_output::make_object_format(uint32_t sampleRate) {
    WAVEFORMATEX format = {};
    format.wFormatTag      = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels       = 1;
    format.nSamplesPerSec  = sampleRate;
    format.wBitsPerSample  = 32;
    format.nBlockAlign     = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

std::string spatial_audio_output::hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) stream << " (" << narrow(message) << ")";
    return stream.str();
}

void spatial_audio_output::throw_if_failed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << hresult_text(hr);
        throw std::runtime_error(stream.str());
    }
}

void spatial_audio_output::render_loop(uint32_t sampleRate, AudioObjectType audioBedMask, unsigned audioChannelMask) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInit)) {
        FB2K_console_formatter() << "foo_out_spatial_audio: COM init failed: " << hresult_text(coInit).c_str();
        return;
    }

    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

        ComPtr<IMMDevice> device = get_render_endpoint(enumerator.Get(), device_);

        ComPtr<ISpatialAudioClient> spatialClient;
        throw_if_failed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");

        const WAVEFORMATEX format = make_object_format(sampleRate);
        throw_if_failed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

        AudioObjectType nativeMask = AudioObjectType_None;
        throw_if_failed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

        UINT32 maxDynamicObjectCount = 0;
        throw_if_failed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");

        OutputConfig streamConfig = current_config();
        const AudioObjectType requestedMask = requested_static_mask(streamConfig, nativeMask, audioBedMask);
        const std::vector<int> dynamicTargets = requested_dynamic_targets(streamConfig, audioChannelMask);
        if (dynamicTargets.size() > maxDynamicObjectCount)
            throw std::runtime_error("Endpoint does not expose enough dynamic Spatial Audio objects for the requested 9.x layout.");

        AudioObjectType activeMask = AudioObjectType_None;
        std::vector<ChannelState> channels;
        for (const auto& ch : all_channels()) {
            if (mask_contains(nativeMask, ch.type) && mask_contains(requestedMask, ch.type)) {
                activeMask = add_mask(activeMask, ch.type);
                channels.push_back({ch.key, ch.type, {}});
            }
        }

        if (channels.empty()) throw std::runtime_error("Endpoint exposes no compatible static Spatial Audio bed channels.");
        if (spatialEvent_ == nullptr) throw std::runtime_error("Spatial completion event is not available.");

        SpatialAudioObjectRenderStreamActivationParams streamParams = {};
        streamParams.ObjectFormat          = const_cast<WAVEFORMATEX*>(&format);
        streamParams.StaticObjectTypeMask  = activeMask;
        streamParams.MinDynamicObjectCount = 0;
        streamParams.MaxDynamicObjectCount = static_cast<UINT32>(dynamicTargets.size());
        streamParams.Category    = AudioCategory_Media;
        streamParams.EventHandle = spatialEvent_;
        streamParams.NotifyObject= nullptr;

        PROPVARIANT activationParams;
        PropVariantInit(&activationParams);
        activationParams.vt = VT_BLOB;
        activationParams.blob.cbSize   = sizeof(streamParams);
        activationParams.blob.pBlobData= reinterpret_cast<BYTE*>(&streamParams);

        ComPtr<ISpatialAudioObjectRenderStream> stream;
        throw_if_failed(spatialClient->ActivateSpatialAudioStream(&activationParams, __uuidof(ISpatialAudioObjectRenderStream), reinterpret_cast<void**>(stream.GetAddressOf())), "Activate spatial stream");

        for (auto& ch : channels)
            throw_if_failed(stream->ActivateSpatialAudioObject(ch.type, ch.object.GetAddressOf()), ("Activate " + ch.key).c_str());

        throw_if_failed(stream->Start(), "Start spatial stream");

        double testPhase = 0.0;
        ComPtr<ISpatialAudioObject> dynamicTestObject;
        std::vector<DynamicChannelState> dynamicChannels;
        for (const int target : dynamicTargets) {
            float x = 0.0f, y = 0.0f, z = -1.0f;
            if (!target_coordinates(target, x, y, z)) continue;
            dynamicChannels.push_back({target, x, y, z, {}});
        }
        while (true) {
            { std::lock_guard<std::mutex> lock(mutex_); if (stopping_) break; }

            const DWORD waitResult = WaitForSingleObject(spatialEvent_, 100);
            if (waitResult != WAIT_OBJECT_0) continue;

            UINT32 availableDynamicObjects = 0;
            UINT32 frameCount = 0;
            throw_if_failed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");
            lastRenderFrames_.store(frameCount);

            std::vector<std::array<float, target_count>> input(frameCount);
            bool paused = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                paused = paused_;
                if (!paused) {
                    for (UINT32 i = 0; i < frameCount && !queue_.empty(); ++i) {
                        input[i] = queue_.front();
                        queue_.pop_front();
                    }
                    queuedFrames_.store(queue_.size());
                }
            }

            const double vol = db_to_linear(volumeDb_.load());
            const OutputConfig frameConfig = current_config();
            const bool dynamicTestActive = frameConfig.directionalTestEnabled
                && frameConfig.directionalTestUseDynamicObject
                && frameConfig.directionalTestTarget != target_low_frequency
                && maxDynamicObjectCount > 0;
            const bool staticTestActive = frameConfig.directionalTestEnabled && !dynamicTestActive;

            for (auto& channel : channels) {
                BYTE* byteBuffer = nullptr;
                UINT32 bufferLength = 0;
                throw_if_failed(channel.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + channel.key).c_str());
                auto* samples = reinterpret_cast<float*>(byteBuffer);
                const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
                const int idx    = channel_index(channel.key);
                const int target = target_from_key(channel.key);

                for (UINT32 i = 0; i < framesToWrite; ++i) {
                    double value = (!paused && idx >= 0 && i < input.size())
                        ? static_cast<double>(input[i][static_cast<size_t>(idx)])
                        : 0.0;
                    if (!paused && staticTestActive && target == frameConfig.directionalTestTarget) {
                        value += test_signal_value(testPhase, frameConfig);
                    }
                    value *= vol;
                    samples[i] = clamp_sample(value);
                }
            }

            if (!paused && dynamicTestActive) {
                if (!dynamicTestObject) {
                    throw_if_failed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, dynamicTestObject.GetAddressOf()), "Activate dynamic test object");
                }

                BYTE* byteBuffer = nullptr;
                UINT32 bufferLength = 0;
                throw_if_failed(dynamicTestObject->GetBuffer(&byteBuffer, &bufferLength), "Get dynamic test buffer");
                auto* samples = reinterpret_cast<float*>(byteBuffer);
                const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
                for (UINT32 i = 0; i < framesToWrite; ++i)
                    samples[i] = clamp_sample(test_signal_value(testPhase, frameConfig));

                float x = 0.0f, y = 0.0f, z = -1.0f;
                if (!target_coordinates(frameConfig.directionalTestTarget, x, y, z))
                    throw std::runtime_error("Selected test target cannot be positioned as a dynamic object.");
                throw_if_failed(dynamicTestObject->SetPosition(x, y, z), "Set dynamic test position");
                throw_if_failed(dynamicTestObject->SetVolume(1.0f), "Set dynamic test volume");
            }

            for (auto& channel : dynamicChannels) {
                if (!channel.object) {
                    throw_if_failed(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic, channel.object.GetAddressOf()), "Activate front wide dynamic object");
                }

                BYTE* byteBuffer = nullptr;
                UINT32 bufferLength = 0;
                throw_if_failed(channel.object->GetBuffer(&byteBuffer, &bufferLength), "Get front wide dynamic buffer");
                auto* samples = reinterpret_cast<float*>(byteBuffer);
                const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
                for (UINT32 i = 0; i < framesToWrite; ++i) {
                    const double value = !paused ? static_cast<double>(input[i][static_cast<size_t>(channel.target)]) * vol : 0.0;
                    samples[i] = clamp_sample(value);
                }
                throw_if_failed(channel.object->SetPosition(channel.x, channel.y, channel.z), "Set front wide dynamic position");
                throw_if_failed(channel.object->SetVolume(1.0f), "Set front wide dynamic volume");
            }

            throw_if_failed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
            SetEvent(wakeEvent_);
        }

        stream->Stop();
        stream->Reset();
    } catch (const std::exception& error) {
        FB2K_console_formatter() << "foo_out_spatial_audio: " << error.what();
    }

    { std::lock_guard<std::mutex> lock(mutex_); started_ = false; }
    lastRenderFrames_.store(0);
    SetEvent(wakeEvent_);
    CoUninitialize();
}

OutputConfig spatial_audio_output::current_config() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

double spatial_audio_output::test_signal_value(double& phase, const OutputConfig& config) const {
    const double frequency = test_frequency_hz(config);
    const double gain  = db_to_linear(std::clamp(config.directionalTestGainDb, -60.0, 0.0));
    const double value = std::sin(phase) * gain;
    phase += 2.0 * kPi * frequency / static_cast<double>(sampleRate_);
    if (phase >= 2.0 * kPi) phase -= 2.0 * kPi;
    return value;
}

double spatial_audio_output::test_frequency_hz(const OutputConfig& config) const {
    if (config.directionalTestTarget == target_low_frequency) return 55.0;
    return std::clamp(config.directionalTestFrequencyHz, 40.0, 2000.0);
}

bool spatial_audio_output::is_5point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront  = (mask & front) == front;
    const bool hasBack   = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide   = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && (hasBack || hasSide) && audio_chunk::g_count_channels(mask) == 6;
}

bool spatial_audio_output::is_7point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront  = (mask & front) == front;
    const bool hasBack   = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide   = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && hasBack && hasSide && audio_chunk::g_count_channels(mask) == 8;
}

uint32_t spatial_audio_output::fixed_sample_rate(SampleRateMode mode) {
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

uint32_t spatial_audio_output::highest_supported_sample_rate(const GUID& device) {
    const uint32_t descendingRates[] = {192000, 176400, 96000, 88200, 48000, 44100};
    for (const uint32_t sr : descendingRates) {
        if (spatial_sample_rate_supported(device, sr)) return sr;
    }
    return 48000;
}

bool spatial_audio_output::spatial_sample_rate_supported(const GUID& deviceGuid, uint32_t sampleRate) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitialize = SUCCEEDED(coInit);
    if (FAILED(coInit) && coInit != RPC_E_CHANGED_MODE) return sampleRate == 48000;
    const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(shouldUninitialize ? reinterpret_cast<void*>(1) : nullptr, [](void* marker) {
        if (marker != nullptr) CoUninitialize();
    });
    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");
        ComPtr<IMMDevice> device = get_render_endpoint(enumerator.Get(), deviceGuid);
        ComPtr<ISpatialAudioClient> spatialClient;
        throw_if_failed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");
        const WAVEFORMATEX format = make_object_format(sampleRate);
        return SUCCEEDED(spatialClient->IsAudioObjectFormatSupported(&format));
    } catch (...) {
        return sampleRate == 48000;
    }
}

uint32_t spatial_audio_output::forced_sample_rate(const OutputConfig& config, const GUID& device) {
    if (config.sampleRateMode == SampleRateMode::SourceIfSupported) return 0;
    if (config.sampleRateMode == SampleRateMode::AutoHighest) return highest_supported_sample_rate(device);
    return fixed_sample_rate(config.sampleRateMode);
}

int spatial_audio_output::target_from_key(const std::string& key) {
    if (key == "front_left")      return target_front_left;
    if (key == "front_right")     return target_front_right;
    if (key == "front_center")    return target_front_center;
    if (key == "low_frequency")   return target_low_frequency;
    if (key == "side_left")       return target_side_left;
    if (key == "side_right")      return target_side_right;
    if (key == "back_left")       return target_back_left;
    if (key == "back_right")      return target_back_right;
    if (key == "top_front_left")  return target_top_front_left;
    if (key == "top_front_right") return target_top_front_right;
    if (key == "top_back_left")   return target_top_back_left;
    if (key == "top_back_right")  return target_top_back_right;
    if (key == "front_wide_left") return target_front_wide_left;
    if (key == "front_wide_right")return target_front_wide_right;
    return target_disabled;
}

int spatial_audio_output::channel_index(const std::string& key) {
    const auto& channels = all_channels();
    for (int i = 0; i < static_cast<int>(channels.size()); ++i) {
        if (key == channels[static_cast<size_t>(i)].key) return i;
    }
    return -1;
}

AudioObjectType spatial_audio_output::requested_static_mask(const OutputConfig& config, AudioObjectType nativeMask, AudioObjectType audioBedMask) {
    if (config.layoutMode == LayoutMode::Auto) {
        const auto detected = static_cast<AudioObjectType>(static_cast<uint32_t>(audioBedMask) & static_cast<uint32_t>(nativeMask));
        return detected != AudioObjectType_None ? detected : nativeMask;
    }

    AudioObjectType mask = AudioObjectType_None;
    auto include = [&](AudioObjectType type) { mask = add_mask(mask, type); };

    switch (config.layoutMode) {
    case LayoutMode::Stereo:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        break;
    case LayoutMode::FivePointOne:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        break;
    case LayoutMode::SevenPointOne:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);   include(AudioObjectType_BackRight);
        break;
    case LayoutMode::FivePointOneTwo:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_TopFrontLeft);include(AudioObjectType_TopFrontRight);
        break;
    case LayoutMode::FivePointOneFour:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_TopFrontLeft);include(AudioObjectType_TopFrontRight);
        include(AudioObjectType_TopBackLeft); include(AudioObjectType_TopBackRight);
        break;
    case LayoutMode::SevenPointOneFour:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);   include(AudioObjectType_BackRight);
        include(AudioObjectType_TopFrontLeft);include(AudioObjectType_TopFrontRight);
        include(AudioObjectType_TopBackLeft); include(AudioObjectType_TopBackRight);
        break;
    case LayoutMode::NinePointOne:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);   include(AudioObjectType_BackRight);
        break;
    case LayoutMode::NinePointOneTwo:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);   include(AudioObjectType_BackRight);
        include(AudioObjectType_TopFrontLeft);include(AudioObjectType_TopFrontRight);
        break;
    case LayoutMode::NinePointOneFour:
    default:
        include(AudioObjectType_FrontLeft);  include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);   include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);   include(AudioObjectType_BackRight);
        include(AudioObjectType_TopFrontLeft);include(AudioObjectType_TopFrontRight);
        include(AudioObjectType_TopBackLeft); include(AudioObjectType_TopBackRight);
        break;
    }
    return mask;
}

std::vector<int> spatial_audio_output::requested_dynamic_targets(const OutputConfig& config, unsigned audioChannelMask) {
    switch (config.layoutMode) {
    case LayoutMode::NinePointOne:
    case LayoutMode::NinePointOneTwo:
    case LayoutMode::NinePointOneFour:
        return {target_front_wide_left, target_front_wide_right};
    case LayoutMode::Auto: {
        std::vector<int> targets;
        if ((audioChannelMask & audio_chunk::channel_front_center_left) != 0)
            targets.push_back(target_front_wide_left);
        if ((audioChannelMask & audio_chunk::channel_front_center_right) != 0)
            targets.push_back(target_front_wide_right);
        return targets;
    }
    default:
        return {};
    }
}

bool spatial_audio_output::target_coordinates(int target, float& x, float& y, float& z) {
    switch (target) {
    case target_front_left:       x = -1.0f; y = 0.0f; z = -1.2f; return true;
    case target_front_right:      x =  1.0f; y = 0.0f; z = -1.2f; return true;
    case target_front_center:     x =  0.0f; y = 0.0f; z = -1.3f; return true;
    case target_low_frequency:    return false;
    case target_side_left:        x = -1.3f; y = 0.0f; z =  0.0f; return true;
    case target_side_right:       x =  1.3f; y = 0.0f; z =  0.0f; return true;
    case target_back_left:        x = -1.0f; y = 0.0f; z =  1.1f; return true;
    case target_back_right:       x =  1.0f; y = 0.0f; z =  1.1f; return true;
    case target_top_front_left:   x = -0.8f; y = 1.4f; z = -0.9f; return true;
    case target_top_front_right:  x =  0.8f; y = 1.4f; z = -0.9f; return true;
    case target_top_back_left:    x = -0.8f; y = 1.4f; z =  0.9f; return true;
    case target_top_back_right:   x =  0.8f; y = 1.4f; z =  0.9f; return true;
    case target_front_wide_left:  x = -1.2f; y = 0.0f; z = -0.65f; return true;
    case target_front_wide_right: x =  1.2f; y = 0.0f; z = -0.65f; return true;
    default: return false;
    }
}

float spatial_audio_output::clamp_sample(double value) {
    return static_cast<float>(std::clamp(value, -1.0, 1.0));
}

double spatial_audio_output::db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

bool spatial_audio_output::stream_shape_changed(const OutputConfig& previous, const OutputConfig& next) {
    return previous.layoutMode != next.layoutMode
        || previous.sampleRateMode != next.sampleRateMode;
}

static output_factory_t<spatial_audio_output> g_spatial_audio_output_factory;

}  // namespace spatial_audio
