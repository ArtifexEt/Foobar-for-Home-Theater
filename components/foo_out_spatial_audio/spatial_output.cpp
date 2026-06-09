#include "stdafx.h"
#include "spatial_output.h"

using Microsoft::WRL::ComPtr;

namespace spatial_audio {
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

spatial_audio_output::spatial_audio_output(const GUID& device, double bufferLength, bool, t_uint32)
    : config_(ReadConfig()), device_(device), bufferLength_(std::max(bufferLength, 0.2)) {
    wakeEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (wakeEvent_ == nullptr) {
        throw exception_output_device_not_found();
    }
}

spatial_audio_output::~spatial_audio_output() {
    stop_stream();
    if (wakeEvent_ != nullptr) {
        CloseHandle(wakeEvent_);
        wakeEvent_ = nullptr;
    }
}

GUID spatial_audio_output::g_get_guid() {
    return guid_output;
}

const char* spatial_audio_output::g_get_name() {
    return "Spatial Audio for Home Theater";
}

void spatial_audio_output::g_enum_devices(output_device_enum_callback& callback) {
    const char name[] = "Default Windows Spatial Audio endpoint";
    callback.on_device(guid_device_default, name, sizeof(name) - 1);
}

bool spatial_audio_output::g_advanced_settings_query() {
    return false;
}

bool spatial_audio_output::g_needs_bitdepth_config() {
    return false;
}

bool spatial_audio_output::g_needs_dither_config() {
    return false;
}

bool spatial_audio_output::g_needs_device_list_prefixes() {
    return false;
}

bool spatial_audio_output::g_supports_multiple_streams() {
    return false;
}

bool spatial_audio_output::g_is_high_latency() {
    return false;
}

unsigned spatial_audio_output::get_forced_sample_rate() {
    return forced_sample_rate(ReadConfig());
}

unsigned spatial_audio_output::get_forced_channel_mask() {
    return 0;
}

void spatial_audio_output::pause(bool state) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        paused_ = state;
    }
    SetEvent(wakeEvent_);
}

void spatial_audio_output::volume_set(double value) {
    volumeDb_.store(value);
}

bool spatial_audio_output::is_progressing() {
    std::lock_guard<std::mutex> lock(mutex_);
    return started_ && !paused_;
}

pfc::eventHandle_t spatial_audio_output::get_trigger_event() {
    return wakeEvent_;
}

void spatial_audio_output::on_update() {
}

void spatial_audio_output::open(audio_chunk::spec_t const& spec) {
    config_ = ReadConfig();
    const uint32_t configuredRate = forced_sample_rate(config_);
    const bool rateMatchesConfig = configuredRate == 0 || spec.sampleRate == configuredRate;
    const bool stereo = spec.chanCount == 2;
    const bool fivePointOne = spec.chanCount == 6 && (spec.chanMask == 0 || is_5point1_mask(spec.chanMask));
    const bool sevenPointOne = spec.chanCount == 8 && (spec.chanMask == 0 || is_7point1_mask(spec.chanMask));
    if (!rateMatchesConfig || !spatial_sample_rate_supported(spec.sampleRate) || (!stereo && !fivePointOne && !sevenPointOne)) {
        throw exception_output_unsupported_stream_format();
    }

    stop_stream();
    sampleRate_ = spec.sampleRate;
    inputLayout_ = sevenPointOne ? InputLayout::SevenPointOne : fivePointOne ? InputLayout::FivePointOne : InputLayout::Stereo;
    capacityFrames_ = static_cast<size_t>(std::max(0.2, bufferLength_) * static_cast<double>(sampleRate_));
    clear_queue();
    start_stream(sampleRate_);
}

void spatial_audio_output::write(const audio_chunk& data) {
    const auto sampleCount = data.get_sample_count();
    const auto channels = data.get_channel_count();
    const auto mask = data.get_channel_config();
    const audio_sample* samples = data.get_data();
    if (samples == nullptr || channels < 2 || sampleCount == 0) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const size_t freeFrames = capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
        const size_t framesToCopy = std::min<size_t>(sampleCount, freeFrames);
        for (size_t i = 0; i < framesToCopy; ++i) {
            InputFrame frame;
            if (inputLayout_ == InputLayout::FivePointOne || inputLayout_ == InputLayout::SevenPointOne) {
                frame.frontLeft = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_front_left, 0);
                frame.frontRight = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_front_right, 1);
                frame.frontCenter = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_front_center, 2);
                frame.lfe = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_lfe, 3);
                if (inputLayout_ == InputLayout::SevenPointOne) {
                    frame.backLeft = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_back_left, 4);
                    frame.backRight = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_back_right, 5);
                    frame.surroundLeft = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_side_left, 6);
                    frame.surroundRight = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_side_right, 7);
                } else {
                    const bool usesSideSurround = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
                    frame.surroundLeft = sample_by_flag(samples, i, channels, mask, usesSideSurround ? audio_chunk::channel_side_left : audio_chunk::channel_back_left, 4);
                    frame.surroundRight = sample_by_flag(samples, i, channels, mask, usesSideSurround ? audio_chunk::channel_side_right : audio_chunk::channel_back_right, 5);
                }
            } else {
                frame.frontLeft = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_front_left, 0);
                frame.frontRight = sample_by_flag(samples, i, channels, mask, audio_chunk::channel_front_right, 1);
            }
            queue_.push_back(frame);
        }
        queuedFrames_.store(queue_.size());
    }

    SetEvent(wakeEvent_);
}

t_size spatial_audio_output::can_write_samples() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ || stopping_) {
        return 0;
    }
    return capacityFrames_ > queue_.size() ? capacityFrames_ - queue_.size() : 0;
}

t_size spatial_audio_output::get_latency_samples() {
    return queuedFrames_.load() + lastRenderFrames_.load();
}

void spatial_audio_output::on_flush() {
    clear_queue();
    SetEvent(wakeEvent_);
}

void spatial_audio_output::on_force_play() {
    SetEvent(wakeEvent_);
}

void spatial_audio_output::start_stream(uint32_t sampleRate) {
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

void spatial_audio_output::stop_stream() {
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

void spatial_audio_output::clear_queue() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    queuedFrames_.store(0);
    lastRenderFrames_.store(0);
}

WAVEFORMATEX spatial_audio_output::make_object_format(uint32_t sampleRate) {
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 1;
    format.nSamplesPerSec = sampleRate;
    format.wBitsPerSample = 32;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

std::string spatial_audio_output::hresult_text(HRESULT hr) {
    _com_error error(hr);
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(hr);
    const wchar_t* message = error.ErrorMessage();
    if (message != nullptr) {
        stream << " (" << narrow(message) << ")";
    }
    return stream.str();
}

void spatial_audio_output::throw_if_failed(HRESULT hr, const char* action) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << action << " failed: " << hresult_text(hr);
        throw std::runtime_error(stream.str());
    }
}

void spatial_audio_output::render_loop(uint32_t sampleRate) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInit)) {
        FB2K_console_formatter() << "foo_out_spatial_audio: COM init failed: " << hresult_text(coInit).c_str();
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

        UINT32 maxDynamicObjectCount = 0;
        throw_if_failed(spatialClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount), "Get max dynamic object count");

        const AudioObjectType requestedMask = requested_static_mask(config_, nativeMask);
        AudioObjectType activeMask = AudioObjectType_None;
        std::vector<ChannelState> channels;
        for (const auto& channel : all_channels()) {
            if (mask_contains(nativeMask, channel.type) && mask_contains(requestedMask, channel.type)) {
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
        streamParams.MaxDynamicObjectCount = maxDynamicObjectCount > 0 ? 1 : 0;
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
        double testPhase = 0.0;
        ComPtr<ISpatialAudioObject> dynamicTestObject;
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

            std::vector<InputFrame> input(frameCount);
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

            const double masterGain = db_to_linear(config_.masterGainDb + config_.headroomDb + volumeDb_.load());
            const bool dynamicTestActive = config_.directionalTestEnabled && config_.directionalTestUseDynamicObject && maxDynamicObjectCount > 0;
            const bool staticTestActive = config_.directionalTestEnabled && !dynamicTestActive;
            for (auto& channel : channels) {
                BYTE* byteBuffer = nullptr;
                UINT32 bufferLength = 0;
                throw_if_failed(channel.object->GetBuffer(&byteBuffer, &bufferLength), ("Get buffer for " + channel.key).c_str());
                auto* samples = reinterpret_cast<float*>(byteBuffer);
                const UINT32 framesToWrite = std::min(frameCount, bufferLength / static_cast<UINT32>(sizeof(float)));
                const int target = target_from_key(channel.key);
                const double channelGain = target >= 0 && target < static_cast<int>(target_count) ? db_to_linear(config_.channelGainDb[static_cast<size_t>(target)]) : 1.0;

                for (UINT32 i = 0; i < framesToWrite; ++i) {
                    double value = paused ? 0.0 : bed_value(channel.key, input[i], lfeState);
                    if (!paused && staticTestActive && target == config_.directionalTestTarget) {
                        value += test_signal_value(testPhase);
                    }
                    value = apply_channel_delay(channel, value * masterGain * channelGain);
                    samples[i] = clamp_sample(apply_limiter(value));
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
                for (UINT32 i = 0; i < framesToWrite; ++i) {
                    samples[i] = clamp_sample(apply_limiter(test_signal_value(testPhase)));
                }

                float x = 0.0f;
                float y = 0.0f;
                float z = -1.0f;
                target_coordinates(config_.directionalTestTarget, x, y, z);
                throw_if_failed(dynamicTestObject->SetPosition(x, y, z), "Set dynamic test position");
                throw_if_failed(dynamicTestObject->SetVolume(1.0f), "Set dynamic test volume");
            }

            throw_if_failed(stream->EndUpdatingAudioObjects(), "End updating audio objects");
            SetEvent(wakeEvent_);
        }

        stream->Stop();
        stream->Reset();
    } catch (const std::exception& error) {
        FB2K_console_formatter() << "foo_out_spatial_audio: " << error.what();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = false;
    }
    lastRenderFrames_.store(0);
    SetEvent(wakeEvent_);
    CoUninitialize();
}

double spatial_audio_output::bed_value(const std::string& key, const InputFrame& frame, double& lfeState) const {
    if (inputLayout_ == InputLayout::SevenPointOne) {
        return mapped_7point1_value(key, frame);
    }
    if (inputLayout_ == InputLayout::FivePointOne) {
        return mapped_5point1_value(key, frame);
    }
    return stereo_bed_value(key, frame, lfeState);
}

double spatial_audio_output::stereo_bed_value(const std::string& key, const InputFrame& frame, double& lfeState) const {
    const double left = frame.frontLeft;
    const double right = frame.frontRight;
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5 * config_.sideAmount;

    if (key == "front_left") return left;
    if (key == "front_right") return right;
    if (key == "front_center") return mid * db_to_linear(config_.centerGainDb);
    if (key == "side_left") return side * db_to_linear(config_.surroundGainDb);
    if (key == "side_right") return -side * db_to_linear(config_.surroundGainDb);
    const double decorrelation = std::clamp(config_.decorrelationAmount, 0.0, 1.0);
    if (key == "back_left") return (side * (1.0 + decorrelation * 0.35) + left * 0.10) * db_to_linear(config_.rearGainDb);
    if (key == "back_right") return (-side * (1.0 - decorrelation * 0.35) + right * 0.10) * db_to_linear(config_.rearGainDb);
    if (key == "top_front_left") return (side * (1.0 - decorrelation * 0.20) + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_front_right") return (-side * (1.0 + decorrelation * 0.20) + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_back_left") return (side * (0.7 + decorrelation * 0.25) + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "top_back_right") return (-side * (0.7 - decorrelation * 0.25) + mid * config_.heightFromMid) * db_to_linear(config_.heightGainDb);
    if (key == "low_frequency") {
        if (!config_.enableLfe) return 0.0;
        const double cutoffHz = std::clamp(config_.lfeLowpassHz, 20.0, 250.0);
        const double alpha = 1.0 - std::exp((-2.0 * kPi * cutoffHz) / static_cast<double>(sampleRate_));
        lfeState += alpha * (mid - lfeState);
        return lfeState * db_to_linear(config_.lfeGainDb);
    }
    return 0.0;
}

double spatial_audio_output::mapped_5point1_value(const std::string& key, const InputFrame& frame) const {
    const int target = target_from_key(key);
    double value = 0.0;
    if (config_.map51FrontLeft == target) value += frame.frontLeft;
    if (config_.map51FrontRight == target) value += frame.frontRight;
    if (config_.map51FrontCenter == target) value += frame.frontCenter;
    if (config_.map51Lfe == target) value += frame.lfe;
    if (config_.map51SurroundLeft == target) value += frame.surroundLeft;
    if (config_.map51SurroundRight == target) value += frame.surroundRight;
    return value;
}

double spatial_audio_output::mapped_7point1_value(const std::string& key, const InputFrame& frame) const {
    if (key == "front_left") return frame.frontLeft;
    if (key == "front_right") return frame.frontRight;
    if (key == "front_center") return frame.frontCenter;
    if (key == "low_frequency") return frame.lfe;
    if (key == "side_left") return frame.surroundLeft;
    if (key == "side_right") return frame.surroundRight;
    if (key == "back_left") return frame.backLeft;
    if (key == "back_right") return frame.backRight;
    return 0.0;
}

double spatial_audio_output::test_signal_value(double& phase) const {
    const double frequency = std::clamp(config_.directionalTestFrequencyHz, 40.0, 2000.0);
    const double gain = db_to_linear(std::clamp(config_.directionalTestGainDb, -60.0, 0.0));
    const double value = std::sin(phase) * gain;
    phase += 2.0 * kPi * frequency / static_cast<double>(sampleRate_);
    if (phase >= 2.0 * kPi) {
        phase -= 2.0 * kPi;
    }
    return value;
}

double spatial_audio_output::apply_limiter(double value) const {
    if (!config_.limiterEnabled) {
        return value;
    }

    const double ceiling = db_to_linear(std::clamp(config_.limiterCeilingDb, -24.0, 0.0));
    if (config_.limiterMode == LimiterMode::HardCeiling) {
        return std::clamp(value, -ceiling, ceiling);
    }

    const double magnitude = std::fabs(value);
    const double kneeStart = ceiling * 0.85;
    if (magnitude <= kneeStart) {
        return value;
    }

    const double range = std::max(ceiling - kneeStart, 0.000001);
    const double over = magnitude - kneeStart;
    const double limited = kneeStart + range * std::tanh(over / range);
    return std::copysign(std::min(limited, ceiling), value);
}

double spatial_audio_output::apply_channel_delay(ChannelState& channel, double value) const {
    const int target = target_from_key(channel.key);
    if (target < 0 || target >= static_cast<int>(target_count)) {
        return value;
    }

    const double delayMs = std::clamp(config_.channelDelayMs[static_cast<size_t>(target)], 0.0, 80.0);
    const size_t delaySamples = static_cast<size_t>(std::round((delayMs / 1000.0) * static_cast<double>(sampleRate_)));
    if (delaySamples == 0) {
        channel.delayBuffer.clear();
        channel.delayIndex = 0;
        return value;
    }

    const size_t requiredSize = delaySamples + 1;
    if (channel.delayBuffer.size() != requiredSize) {
        channel.delayBuffer.assign(requiredSize, 0.0f);
        channel.delayIndex = 0;
    }

    const double delayed = channel.delayBuffer[channel.delayIndex];
    channel.delayBuffer[channel.delayIndex] = static_cast<float>(value);
    channel.delayIndex = (channel.delayIndex + 1) % channel.delayBuffer.size();
    return delayed;
}

bool spatial_audio_output::is_5point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront = (mask & front) == front;
    const bool hasBack = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && (hasBack || hasSide) && audio_chunk::g_count_channels(mask) == 6;
}

bool spatial_audio_output::is_7point1_mask(unsigned mask) {
    const unsigned front = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
    const bool hasFront = (mask & front) == front;
    const bool hasBack = (mask & audio_chunk::channels_back_left_right) == audio_chunk::channels_back_left_right;
    const bool hasSide = (mask & audio_chunk::channels_side_left_right) == audio_chunk::channels_side_left_right;
    return hasFront && hasBack && hasSide && audio_chunk::g_count_channels(mask) == 8;
}

uint32_t spatial_audio_output::fixed_sample_rate(SampleRateMode mode) {
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

uint32_t spatial_audio_output::highest_supported_sample_rate() {
    const uint32_t descendingRates[] = {192000, 176400, 96000, 88200, 48000, 44100};
    for (const uint32_t sampleRate : descendingRates) {
        if (spatial_sample_rate_supported(sampleRate)) {
            return sampleRate;
        }
    }
    return 48000;
}

bool spatial_audio_output::spatial_sample_rate_supported(uint32_t sampleRate) {
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitialize = SUCCEEDED(coInit);
    if (FAILED(coInit) && coInit != RPC_E_CHANGED_MODE) {
        return sampleRate == 48000;
    }
    const auto cleanupCom = std::unique_ptr<void, void (*)(void*)>(shouldUninitialize ? reinterpret_cast<void*>(1) : nullptr, [](void* marker) {
        if (marker != nullptr) {
            CoUninitialize();
        }
    });

    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)), "Create MMDeviceEnumerator");

        ComPtr<IMMDevice> device;
        throw_if_failed(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device), "Get default render endpoint");

        ComPtr<ISpatialAudioClient> spatialClient;
        throw_if_failed(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(spatialClient.GetAddressOf())), "Activate ISpatialAudioClient");

        const WAVEFORMATEX format = make_object_format(sampleRate);
        return SUCCEEDED(spatialClient->IsAudioObjectFormatSupported(&format));
    } catch (...) {
        return sampleRate == 48000;
    }
}

uint32_t spatial_audio_output::forced_sample_rate(const RuntimeConfig& config) {
    if (config.sampleRateMode == SampleRateMode::SourceIfSupported) {
        return 0;
    }
    if (config.sampleRateMode == SampleRateMode::AutoHighest) {
        return highest_supported_sample_rate();
    }
    return fixed_sample_rate(config.sampleRateMode);
}

float spatial_audio_output::sample_by_flag(const audio_sample* samples, size_t frame, unsigned channels, unsigned mask, unsigned flag, unsigned fallbackIndex) {
    unsigned index = audio_chunk::g_channel_index_from_flag(mask, flag);
    if (index == static_cast<unsigned>(-1) || index >= channels) {
        index = fallbackIndex;
    }
    return index < channels ? static_cast<float>(samples[(frame * channels) + index]) : 0.0f;
}

int spatial_audio_output::target_from_key(const std::string& key) {
    if (key == "front_left") return target_front_left;
    if (key == "front_right") return target_front_right;
    if (key == "front_center") return target_front_center;
    if (key == "low_frequency") return target_low_frequency;
    if (key == "side_left") return target_side_left;
    if (key == "side_right") return target_side_right;
    if (key == "back_left") return target_back_left;
    if (key == "back_right") return target_back_right;
    if (key == "top_front_left") return target_top_front_left;
    if (key == "top_front_right") return target_top_front_right;
    if (key == "top_back_left") return target_top_back_left;
    if (key == "top_back_right") return target_top_back_right;
    return target_disabled;
}

AudioObjectType spatial_audio_output::requested_static_mask(const RuntimeConfig& config, AudioObjectType nativeMask) {
    if (config.layoutMode == LayoutMode::Auto) {
        return nativeMask;
    }

    AudioObjectType mask = AudioObjectType_None;
    auto include = [&](AudioObjectType type) {
        mask = add_mask(mask, type);
    };

    switch (config.layoutMode) {
    case LayoutMode::Stereo:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        break;
    case LayoutMode::FivePointOne:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);
        include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);
        include(AudioObjectType_SideRight);
        break;
    case LayoutMode::SevenPointOne:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);
        include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);
        include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);
        include(AudioObjectType_BackRight);
        break;
    case LayoutMode::FivePointOneTwo:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);
        include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);
        include(AudioObjectType_SideRight);
        include(AudioObjectType_TopFrontLeft);
        include(AudioObjectType_TopFrontRight);
        break;
    case LayoutMode::SevenPointOneFour:
    default:
        include(AudioObjectType_FrontLeft);
        include(AudioObjectType_FrontRight);
        include(AudioObjectType_FrontCenter);
        include(AudioObjectType_LowFrequency);
        include(AudioObjectType_SideLeft);
        include(AudioObjectType_SideRight);
        include(AudioObjectType_BackLeft);
        include(AudioObjectType_BackRight);
        include(AudioObjectType_TopFrontLeft);
        include(AudioObjectType_TopFrontRight);
        include(AudioObjectType_TopBackLeft);
        include(AudioObjectType_TopBackRight);
        break;
    }

    return mask;
}

bool spatial_audio_output::target_coordinates(int target, float& x, float& y, float& z) {
    switch (target) {
    case target_front_left:
        x = -1.0f; y = 0.0f; z = -1.2f; return true;
    case target_front_right:
        x = 1.0f; y = 0.0f; z = -1.2f; return true;
    case target_front_center:
        x = 0.0f; y = 0.0f; z = -1.3f; return true;
    case target_low_frequency:
        x = 0.0f; y = -0.2f; z = -0.8f; return true;
    case target_side_left:
        x = -1.3f; y = 0.0f; z = 0.0f; return true;
    case target_side_right:
        x = 1.3f; y = 0.0f; z = 0.0f; return true;
    case target_back_left:
        x = -1.0f; y = 0.0f; z = 1.1f; return true;
    case target_back_right:
        x = 1.0f; y = 0.0f; z = 1.1f; return true;
    case target_top_front_left:
        x = -0.8f; y = 1.4f; z = -0.9f; return true;
    case target_top_front_right:
        x = 0.8f; y = 1.4f; z = -0.9f; return true;
    case target_top_back_left:
        x = -0.8f; y = 1.4f; z = 0.9f; return true;
    case target_top_back_right:
        x = 0.8f; y = 1.4f; z = 0.9f; return true;
    default:
        return false;
    }
}

float spatial_audio_output::clamp_sample(double value) {
    return static_cast<float>(std::clamp(value, -1.0, 1.0));
}

double spatial_audio_output::db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

static output_factory_t<spatial_audio_output> g_spatial_audio_output_factory;

}  // namespace spatial_audio
