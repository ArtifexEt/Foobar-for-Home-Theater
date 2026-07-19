#include "stdafx.h"
#include "height_dsp.h"
#include "height_resource.h"

namespace spatial_audio {
namespace {

static constexpr GUID guid_height_dsp = { 0x2d9c4b16, 0xf2a8, 0x49db, { 0xa7,0xd4,0x3c,0x91,0x8e,0x62,0x75,0xb0 } };

HeightDspConfig sanitize(HeightDspConfig value) {
    if (value.layout != HeightLayout::Two && value.layout != HeightLayout::Four) value.layout = HeightLayout::Four;
    value.heightGainDb = std::clamp(value.heightGainDb, -30.0, 6.0);
    value.frontDifference = std::clamp(value.frontDifference, 0.0, 1.0);
    value.surroundFeed = std::clamp(value.surroundFeed, 0.0, 1.0);
    value.midFeed = std::clamp(value.midFeed, 0.0, 0.5);
    return value;
}

std::string serialize_config(const HeightDspConfig& raw) {
    const HeightDspConfig value = sanitize(raw);
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(17);
    out << "height_dsp_version=1\n"
        << "layout=" << static_cast<uint32_t>(value.layout) << "\n"
        << "height_gain_db=" << value.heightGainDb << "\n"
        << "front_difference=" << value.frontDifference << "\n"
        << "surround_feed=" << value.surroundFeed << "\n"
        << "mid_feed=" << value.midFeed << "\n";
    return out.str();
}

bool parse_number(const std::string& text, double& value) {
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    double parsed = 0.0;
    input >> parsed;
    if (!input || !input.eof() || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}

HeightDspConfig parse_config(const dsp_preset& preset) {
    HeightDspConfig value;
    if (preset.get_owner() != guid_height_dsp || preset.get_data() == nullptr || preset.get_data_size() == 0 || preset.get_data_size() > 4096) return value;
    const char* bytes = static_cast<const char*>(preset.get_data());
    std::istringstream input(std::string(bytes, bytes + preset.get_data_size()));
    std::string line;
    bool versionSeen = false;
    while (std::getline(input, line)) {
        const size_t separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = line.substr(0, separator);
        const std::string raw = line.substr(separator + 1);
        double number = 0.0;
        if (key == "height_dsp_version") {
            versionSeen = raw == "1";
        } else if (key == "layout" && parse_number(raw, number)) {
            value.layout = static_cast<int>(number) == 2 ? HeightLayout::Two : HeightLayout::Four;
        } else if (key == "height_gain_db" && parse_number(raw, number)) {
            value.heightGainDb = number;
        } else if (key == "front_difference" && parse_number(raw, number)) {
            value.frontDifference = number;
        } else if (key == "surround_feed" && parse_number(raw, number)) {
            value.surroundFeed = number;
        } else if (key == "mid_feed" && parse_number(raw, number)) {
            value.midFeed = number;
        }
    }
    return versionSeen ? sanitize(value) : HeightDspConfig{};
}

void make_preset(const HeightDspConfig& value, dsp_preset& preset) {
    const std::string text = serialize_config(value);
    preset.set_owner(guid_height_dsp);
    preset.set_data(text.data(), text.size());
}

double read_edit(HWND window, int id, double fallback) {
    wchar_t buffer[64] = {};
    GetDlgItemTextW(window, id, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    const double parsed = std::wcstod(buffer, &end);
    while (end != nullptr && std::iswspace(*end)) ++end;
    return end != buffer && end != nullptr && *end == L'\0' && std::isfinite(parsed) ? parsed : fallback;
}

void write_edit(HWND window, int id, double value) {
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"%.2f", value);
    SetDlgItemTextW(window, id, buffer);
}

class height_config_popup : public CDialogImpl<height_config_popup> {
public:
    height_config_popup(const dsp_preset& initial, dsp_preset_edit_callback& callback)
        : original_(initial), config_(parse_config(initial)), callback_(callback) {}

    enum { IDD = IDD_HEIGHT_DSP_CONFIG };

    BEGIN_MSG_MAP_EX(height_config_popup)
        MSG_WM_INITDIALOG(on_init)
        COMMAND_HANDLER_EX(IDOK, BN_CLICKED, on_close)
        COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, on_close)
    END_MSG_MAP()

private:
    BOOL on_init(CWindow, LPARAM) {
        HWND combo = GetDlgItem(IDC_HEIGHT_LAYOUT);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2 speakers (Top Front)"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"4 speakers (Top Front + Top Back)"));
        SendMessageW(combo, CB_SETCURSEL, config_.layout == HeightLayout::Two ? 0 : 1, 0);
        write_edit(m_hWnd, IDC_HEIGHT_GAIN, config_.heightGainDb);
        write_edit(m_hWnd, IDC_FRONT_DIFFERENCE, config_.frontDifference);
        write_edit(m_hWnd, IDC_SURROUND_FEED, config_.surroundFeed);
        write_edit(m_hWnd, IDC_MID_FEED, config_.midFeed);
        return TRUE;
    }

    void on_close(UINT, int id, CWindow) {
        if (id == IDOK) {
            const int selected = static_cast<int>(SendDlgItemMessage(IDC_HEIGHT_LAYOUT, CB_GETCURSEL));
            config_.layout = selected == 0 ? HeightLayout::Two : HeightLayout::Four;
            config_.heightGainDb = read_edit(m_hWnd, IDC_HEIGHT_GAIN, config_.heightGainDb);
            config_.frontDifference = read_edit(m_hWnd, IDC_FRONT_DIFFERENCE, config_.frontDifference);
            config_.surroundFeed = read_edit(m_hWnd, IDC_SURROUND_FEED, config_.surroundFeed);
            config_.midFeed = read_edit(m_hWnd, IDC_MID_FEED, config_.midFeed);
            dsp_preset_impl updated;
            make_preset(sanitize(config_), updated);
            callback_.on_preset_changed(updated);
        } else {
            callback_.on_preset_changed(original_);
        }
        EndDialog(id);
    }

    const dsp_preset& original_;
    HeightDspConfig config_;
    dsp_preset_edit_callback& callback_;
};

float finite_sample(audio_sample value) {
    return std::isfinite(static_cast<double>(value)) ? static_cast<float>(value) : 0.0f;
}

bool has_channel(unsigned mask, unsigned flag) {
    return (mask & flag) != 0;
}

float channel_sample(const audio_sample* input, size_t frame, unsigned channels, unsigned mask, unsigned flag, int fallback = -1) {
    unsigned index = audio_chunk::g_channel_index_from_flag(mask, flag);
    if (index == static_cast<unsigned>(-1) && fallback >= 0) index = static_cast<unsigned>(fallback);
    return index < channels ? finite_sample(input[frame * channels + index]) : 0.0f;
}

float generated_height(unsigned flag, const audio_sample* input, size_t frame, unsigned channels, unsigned mask, const HeightDspConfig& config) {
    const double left = channel_sample(input, frame, channels, mask, audio_chunk::channel_front_left, 0);
    const double right = channel_sample(input, frame, channels, mask, audio_chunk::channel_front_right, channels > 1 ? 1 : 0);
    const double mid = (left + right) * 0.5;
    const double difference = (left - right) * 0.5 * config.frontDifference;
    const double sideLeft = channel_sample(input, frame, channels, mask, audio_chunk::channel_side_left);
    const double sideRight = channel_sample(input, frame, channels, mask, audio_chunk::channel_side_right);
    const double backLeft = channel_sample(input, frame, channels, mask, audio_chunk::channel_back_left);
    const double backRight = channel_sample(input, frame, channels, mask, audio_chunk::channel_back_right);
    const double gain = std::pow(10.0, config.heightGainDb / 20.0);

    double value = 0.0;
    if (flag == audio_chunk::channel_top_front_left) value = difference + sideLeft * config.surroundFeed + mid * config.midFeed;
    if (flag == audio_chunk::channel_top_front_right) value = -difference + sideRight * config.surroundFeed + mid * config.midFeed;
    if (flag == audio_chunk::channel_top_back_left) value = difference * 0.7 + (has_channel(mask, audio_chunk::channel_back_left) ? backLeft : sideLeft) * config.surroundFeed + mid * config.midFeed;
    if (flag == audio_chunk::channel_top_back_right) value = -difference * 0.7 + (has_channel(mask, audio_chunk::channel_back_right) ? backRight : sideRight) * config.surroundFeed + mid * config.midFeed;
    value *= gain;
    return static_cast<float>(std::clamp(value, -1.0, 1.0));
}

} // namespace

height_only_dsp::height_only_dsp(const dsp_preset& preset) : config_(parse_config(preset)) {}

GUID height_only_dsp::g_get_guid() { return guid_height_dsp; }

void height_only_dsp::g_get_name(pfc::string_base& out) { out = "Add Ceiling Speakers"; }

bool height_only_dsp::g_get_default_preset(dsp_preset& out) {
    make_preset(HeightDspConfig{}, out);
    return true;
}

bool height_only_dsp::g_have_config_popup() { return true; }

void height_only_dsp::g_show_config_popup(const dsp_preset& data, HWND parent, dsp_preset_edit_callback& callback) {
    height_config_popup popup(data, callback);
    popup.DoModal(parent);
}

bool height_only_dsp::on_chunk(audio_chunk* chunk, abort_callback&) {
    const unsigned channels = chunk->get_channel_count();
    const size_t frames = chunk->get_sample_count();
    const unsigned sampleRate = chunk->get_sample_rate();
    const audio_sample* input = chunk->get_data();
    if (channels == 0 || frames == 0 || input == nullptr) return true;

    unsigned inputMask = chunk->get_channel_config();
    if (inputMask == 0 || audio_chunk::g_count_channels(inputMask) != channels) {
        inputMask = audio_chunk::g_guess_channel_config(channels);
    }

    unsigned heightMask = audio_chunk::channel_top_front_left | audio_chunk::channel_top_front_right;
    if (config_.layout == HeightLayout::Four) {
        heightMask |= audio_chunk::channel_top_back_left | audio_chunk::channel_top_back_right;
    }
    const unsigned outputMask = inputMask | heightMask;
    if (outputMask == inputMask) return true;

    const unsigned outputChannels = audio_chunk::g_count_channels(outputMask);
    std::vector<audio_sample> output(frames * outputChannels, 0.0f);
    for (size_t frame = 0; frame < frames; ++frame) {
        for (unsigned outputIndex = 0; outputIndex < outputChannels; ++outputIndex) {
            const unsigned flag = audio_chunk::g_extract_channel_flag(outputMask, outputIndex);
            const unsigned inputIndex = audio_chunk::g_channel_index_from_flag(inputMask, flag);
            if (inputIndex != static_cast<unsigned>(-1) && inputIndex < channels) {
                output[frame * outputChannels + outputIndex] = input[frame * channels + inputIndex];
            } else {
                output[frame * outputChannels + outputIndex] = generated_height(flag, input, frame, channels, inputMask, config_);
            }
        }
    }
    chunk->set_data(output.data(), frames, outputChannels, sampleRate, outputMask);
    return true;
}

static dsp_factory_t<height_only_dsp> g_height_dsp_factory;

} // namespace spatial_audio
