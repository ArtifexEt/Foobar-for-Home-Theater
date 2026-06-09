#include "stdafx.h"
#include "spatial_output.h"

using Microsoft::WRL::ComPtr;

namespace spatial_atmos {
namespace {

constexpr double kPi = 3.14159265358979323846;

static constexpr GUID guid_output = { 0xb588936e, 0x8925, 0x4874, { 0x82, 0xf4, 0x6a, 0x1d, 0x71, 0x08, 0x6d, 0xf3 } };
static constexpr GUID guid_device_default = { 0xb5e19e29, 0x189e, 0x4a8b, { 0xad, 0x94, 0x28, 0x7c, 0xba, 0xa8, 0x10, 0x94 } };

struct ChannelDef {
    const char* key;
    AudioObjectType type;
};

const std::vector<ChannelDef>& all_channels() {
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

AudioObjectType add_mask(AudioObjectType mask, AudioObjectType value) {
    return static_cast<AudioObjectType>(static_cast<uint32_t>(mask) | static_cast<uint32_t>(value));
}

bool mask_contains(AudioObjectType mask, AudioObjectType value) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(value)) == static_cast<uint32_t>(value);
}

std::string narrow(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }

    std::string narrowText(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrowText.data(), required, nullptr, nullptr);
    return narrowText;
}

}  // namespace

spatial_atmos_output::spatial_atmos_output(const GUID& device, double bufferLength, bool, t_uint32)
    : config_(ReadConfig()), device_(device), bufferLength_(std::max(bufferLength, 0.2)) {
    wakeEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (wakeEvent_ == nullptr) {
        throw exception_output_device_not_found();
    }
}

spatial_atmos_output::~spatial_atmos_output() {
    stop_stream();
    if (wakeEvent_ != nullptr) {
        CloseHandle(wakeEvent_);
        wakeEvent_ = nullptr;
    }
}

GUID spatial_atmos_output::g_get_guid() {
    return guid_output;
}

const char* spatial_atmos_output::g_get_name() {
    return "Spatial Atmos for Home Theater";
}

void spatial_atmos_output::g_enum_devices(output_device_enum_callback& callback) {
    const char name[] = "Default Windows Spatial Audio endpoint";
    callback.on_device(guid_device_default, name, sizeof(name) - 1);
}

bool spatial_atmos_output::g_advanced_settings_query() {
    return false;
}

bool spatial_atmos_output::g_needs_bitdepth_config() {
    return false;
}

bool spatial_atmos_output::g_needs_dither_config() {
    return false;
}

bool spatial_atmos_output::g_needs_device_list_prefixes() {
    return false;
}

bool spatial_atmos_output::g_supports_multiple_streams() {
    return false;
}

bool spatial_atmos_output::g_is_high_latency() {
    return false;
}

unsigned spatial_atmos_output::get_forced_sample_rate() {
    return 48000;
}

unsigned spatial_atmos_output::get_forced_channel_mask() {
    return audio_chunk::channel_config_stereo;
}

void spatial_atmos_output::pause(bool state) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        paused_ = state;
    }
    SetEvent(wakeEvent_);
}

void spatial_atmos_output::volume_set(double value) {
    volumeDb_.store(value);
}

bool spatial_atmos_output::is_progressing() {
    std::lock_guard<std::mutex> lock(mutex_);
    return started_ && !paused_;
}

pfc::eventHandle_t spatial_atmos_output::get_trigger_event() {
    return wakeEvent_;
}

void spatial_atmos_output::on_update() {
}

void spatial_atmos_output::open(audio_chunk::spec_t const& spec) {
    if (spec.sampleRate != 48000 || spec.chanCount != 2) {
        throw exception_output_unsupported_stream_format();
    }

    stop_stream();
    config_ = ReadConfig();
    sampleRate_ = spec.sampleRate;
    capacityFrames_ = static_cast<size_t>(std::max(0.2, bufferLength_) * static_cast<double>(sampleRate_));
    clear_queue();
    start_stream(sampleRate_);
}

void spatial_atmos_output::write(const audio_chunk& data) {
    const auto sampleCount = data.get_sample_count();
    const auto channels = data.get_channel_count();
    const audio_sample* samples = data.get_data();
    if (samples == nullptr || channels < 2 || sampleCount == 0) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t freeFrames = capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
        const size_t framesToCopy = std::min<size_t>(sampleCount, freeFrames);
        for (size_t i = 0; i < framesToCopy; ++i) {
            queue_.push_back({
                static_cast<float>(samples[(i * channels) + 0]),
                static_cast<float>(samples[(i * channels) + 1]),
            });
        }
        queuedFrames_.store(queue_.size());
    }

    SetEvent(wakeEvent_);
}

t_size spatial_atmos_output::can_write_samples() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) {
        return 0;
    }
    return capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
}

t_size spatial_atmos_output::get_latency_samples() {
    return queuedFrames_.load() + lastRenderFrames_.load();
}

void spatial_atmos_output::on_flush() {
    clear_queue();
    SetEvent(wakeEvent_);
}

void spatial_atmos_output::on_force_play() {
    SetEvent(wakeEvent_);
}

void spatial_atmos_output::start_stream(uint32_t sampleRate) {
    spatialEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (spatialEvent_ == nullptr) {
        throw_if_failed(HRESULT_FROM_WIN32(GetLastError()), "Create spatial completion event");
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = false;
        paused_ = false;
        started_ = true;
    }

    renderThread_ = std::thread([this, sampleRate]() {
        render_loop(sampleRate);
    });
}

void spatial_atmos_output::stop_stream() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        started_ = false;
    }

    SetEvent(wakeEvent_);
    if (spatialEvent_ != nullptr) {
        SetEvent(spatialEvent_);
    }

    if (renderThread_.joinable()) {
        renderThread_.join();
    }

    if (spatialEvent_ != nullptr) {
        CloseHandle(spatialEvent_);
        spatialEvent_ = nullptr;
    }

    clear_queue();
}

void spatial_atmos_output::clear_queue() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    queuedFrames_.store(0);
    lastRenderFrames_.store(0);
}

WAVEFORMATEX spatial_atmos_output::make_object_format(uint32_t sampleRate) {
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 1;
    format.nSamplesPerSec = sampleRate;
    format.wBitsPerSample = 32;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

std::string spatial_atmos_output::hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) {
        stream << " (" << narrow(message) << ")";
    }
    return stream.str();
}

void spatial_atmos_output::throw_if_failed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << hresult_text(hr);
        throw std::runtime_error(stream.str());
    }
}

void spatial_atmos_output::render_loop(uint32_t sampleRate) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInit)) {
        FB2K_console_formatter() << "foo_out_spatial_atmos: COM init failed: " << hresult_text(coInit).c_str();
        return;
    }

    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

        ComPtr<IMMDevice> device;
        throw_if_failed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");

        ComPtr<ISpatialAudioClient> spatialClient;
        throw_if_failed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");

        const WAVEFORMATEX format = make_object_format(sampleRate);
        throw_if_failed(spatialClient->IsAudioObjectFormatSupported(&format), "Check spatial object format");

        AudioObjectType nativeMask = AudioObjectType_None;
        throw_if_failed(spatialClient->GetNativeStaticObjectTypeMask(&nativeMask), "Get native static object mask");

        AudioObjectType activeMask = AudioObjectType_None;
        std::vector<ChannelState> channels;
        for (const auto& channel : all_channels()) {
            if (mask_contains(nativeMask, channel.type)) {
                activeMask = add_mask(activeMask, channel.type);
                channels.push_back({channel.key, channel.type, {}});
            }
        }

        if (channels.empty()) {
            throw std::runtime_error("Endpoint exposes no compatible static Spatial Audio bed channels.");
        }

        if (spatialEvent_ == nullptr) {
            throw std::runtime_error("Spatial completion event is not available.");
        }

        SpatialAudioObjectRenderStreamActivationParams streamParams = {};
        streamParams.ObjectFormat = const_cast<WAVEFORMATEX*>(&format);
        streamParams.StaticObjectTypeMask = activeMask;
        streamParams.MinDynamicObjectCount = 0;
        streamParams.MaxDynamicObjectCount = 0;
        streamParams.Category = AudioCategory_Media;
        streamParams.EventHandle = spatialEvent_;
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

        for (auto& channel : channels) {
            throw_if_failed(stream->ActivateSpatialAudioObject(channel.type, channel.object.GetAddressOf()), ("Activate " + channel.key).c_str());
        }

        throw_if_failed(stream->Start(), "Start spatial stream");

        double lfeState = 0.0;
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stopping_) {
                    break;
                }
            }

            const DWORD waitResult = WaitForSingleObject(spatialEvent_, 100);
            if (waitResult != WAIT_OBJECT_0) {
                continue;
            }

            UINT32 availableDynamicObjects = 0;
            UINT32 frameCount = 0;
            throw_if_failed(stream->BeginUpdatingAudioObjects(&availableDynamicObjects, &frameCount), "Begin updating audio objects");
            lastRenderFrames_.store(frameCount);

            std::vector<StereoFrame> input(frameCount);
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

            const double masterGain = db_to_linear(config_.masterGainDb + volumeDb_.load());
            for (auto& channel : channels) {
                BYTE* byteBuffer = nullptr;
                UINT32 bufferLength = 0;
                throw_if_failed(channel.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + channel.key).c_str());
                auto* samples = reinterpret_cast<float*>(byteBuffer);
                const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));

                for (UINT32 i = 0; i < framesToWrite; ++i) {
                    const double value = paused ? 0.0 : bed_value(channel.key, input[i], lfeState);
                    samples[i] = clamp_sample(value * masterGain);
                }
            }

            throw_if_failed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
            SetEvent(wakeEvent_);
        }

        stream->Stop();
        stream->Reset();
    } catch (const std::exception& error) {
        FB2K_console_formatter() << "foo_out_spatial_atmos: " << error.what();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
    }
    lastRenderFrames_.store(0);
    SetEvent(wakeEvent_);
    CoUninitialize();
}

double spatial_atmos_output::bed_value(const std::string& key, const StereoFrame& frame, double& lfeState) const {
    const double left = frame.left;
    const double right = frame.right;
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5 * config_.sideAmount;

    if (key == "front_left") return left;
    if (key == "front_right") return right;
    if (key == "front_center") return mid * db_to_linear(config_.centerGainDb);
    if (key == "side_left") return side * db_to_linear(config_.surroundGainDb);
    if (key == "side_right") return -side * db_to_linear(config_.surroundGainDb);
    if (key == "back_left") return (side + left * 0.10) * db_to_linear(config_.rearGainDb);
    if (key == "back_right") return (-side + right * 0.10) * db_to_linear(config_.rearGainDb);
    if (key == "top_front_left") return (side + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_front_right") return (-side + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_back_left") return (side * 0.7 + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_back_right") return (-side * 0.7 + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "low_frequency") {
        if (!config_.enableLfe) return 0.0;
        const double cutoffHz = std::clamp(config_.lfeLowpassHz, 20.0, 250.0);
        const double alpha = 1.0 - std::exp((-2.0 * kPi * cutoffHz) / static_cast<double>(sampleRate_));
        lfeState += alpha * (mid - lfeState);
        return lfeState * db_to_linear(config_.lfeGainDb);
    }
    return 0.0;
}

float spatial_atmos_output::clamp_sample(double value) {
    return static_cast<float>(std::clamp(value, -1.0, 1.0));
}

double spatial_atmos_output::db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

static output_factory_t<spatial_atmos_output> g_spatial_atmos_output_factory;

}  // namespace spatial_atmos
