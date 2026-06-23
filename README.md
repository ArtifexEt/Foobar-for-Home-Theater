# Foobar for Home Theater

Windows Spatial Audio output component for foobar2000 home theater setups, plus standalone diagnostics used to validate the Windows endpoint.

This repository contains two foobar2000 components that work together:

- **`foo_dsp_spatial`** — DSP component that upmixes stereo, 5.1, or 7.1 PCM to a 12-channel spatial mix. All audio processing (upmix, LFE extraction, limiter, per-channel delays) lives here.
- **`foo_out_spatial_audio`** — Output component that sends the 12-channel PCM from the DSP directly to the Windows Spatial Audio API. One channel per Spatial Audio bed object. No audio processing in this component.

The repository builds both components and the standalone Windows tools with GitHub Actions on `windows-2022`. Real Spatial Audio playback must be tested on a Windows machine with a compatible HDMI/eARC endpoint.

## Download and install

Install both components — they work as a chain: the DSP processes audio, the output sends it to Windows Spatial Audio.

1. Download the latest release:
   - [`foo_dsp_spatial.fb2k-component`](https://github.com/ArtifexEt/Foobar-for-Home-Theater/releases/latest/download/foo_dsp_spatial.fb2k-component)
   - [`foo_out_spatial_audio.fb2k-component`](https://github.com/ArtifexEt/Foobar-for-Home-Theater/releases/latest/download/foo_out_spatial_audio.fb2k-component)
2. In foobar2000, open Preferences > Components, click Install, choose each `.fb2k-component` file, then Apply.
3. Restart foobar2000 when prompted.
4. Open Preferences > Playback > Output and select `Spatial Audio for Home Theater`.
5. Open Preferences > Playback > DSP Manager, add `Spatial Audio Upmix` to the active DSP chain, and place it first.
6. Open Preferences > Tools > Spatial Audio DSP to configure upmix mode, limiter, channel gains, and delays.
7. Open Preferences > Playback > Output > Spatial Audio for Home Theater to configure the output bed layout and render rate.

Release notes are published on the [GitHub releases page](https://github.com/ArtifexEt/Foobar-for-Home-Theater/releases). The release ZIP contains standalone diagnostics and test executables. The foobar2000 plugins are the two `.fb2k-component` files linked above.

The component version shown inside foobar2000 is embedded during the GitHub Actions build and matches the release tag without the leading `v`, for example release `v0.3.30` installs as version `0.3.30`.

## Screenshots

### Layout (output plugin)

Choose the Windows Spatial Audio bed and render rate.

![Layout](docs/screenshots/layout.png)

### DSP — Upmix

Select the listening mode and tune how stereo is spread into center, surround, rear, height, and optional LFE channels. `Reference` is the safer default; `Full spatial` is wider. `Beginner defaults` resets to conservative values.

![Upmix controls](docs/screenshots/upmix-controls.png)

### DSP — Channels

Adjust each output bed channel independently with gain, delay, and polarity inversion.

![Per-channel trims](docs/screenshots/channel-trims.png)

### DSP — Channel Mapping

Map each 5.1 source channel to any output bed channel.

![Channel Mapping](docs/screenshots/channel-mapping.png)

### Test (output plugin)

Play a test tone through a selected bed direction to verify routing.

![Testing](docs/screenshots/testing.png)

### About (output plugin)

Shows the plugin version and links.

![About](docs/screenshots/about.png)

## Requirements

- Windows 10/11 with a recent Windows SDK.
- Visual Studio 2022 with C++ desktop tools.
- CMake 3.24 or newer.
- A real HDMI/eARC endpoint with Windows Spatial Audio support selected as the default output device.
- Spatial sound enabled for that endpoint in Windows sound settings.
- foobar2000 2.x 64-bit for the component packages.

## How it works

The DSP component (`foo_dsp_spatial`) receives foobar2000 audio chunks and outputs 12-channel PCM with no channel mask (`chanMask=0`). Channel order matches the Windows Spatial Audio bed: front left, front right, front center, LFE, side left, side right, back left, back right, top front left, top front right, top back left, top back right.

Stereo input is upmixed into the selected bed. 5.1 input can be mapped per source channel, and 7.1 input passes through to the matching bed channels.

The output component (`foo_out_spatial_audio`) receives the 12-channel PCM and copies each bed channel to the matching `ISpatialAudioObject` on the Windows Spatial Audio endpoint. It accepts the DSP's unmasked bed order and also handles standard masked 12-channel chunks. Supported output beds are Auto, Stereo, 5.1, 7.1, 5.1.2, 5.1.4, and 7.1.4.

## Quality

The component renders Windows Spatial Audio objects as mono float32 streams and keeps internal mix math in double precision. `Render rate` defaults to `48000 Hz` for broad HDMI/eARC receiver compatibility.

For cleanest output, keep enough headroom in the DSP upmix settings, leave the limiter enabled in `Transparent soft` mode, and keep per-channel trims at or below 0 dB unless compensating for real speaker calibration.

## Support

[Support: Buy me a coffee](https://buymeacoffee.com/szymonrybka)

[GitHub repository](https://github.com/ArtifexEt/Foobar-for-Home-Theater)

## Build

Run these commands from the repository root.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The foobar2000 components are built by the GitHub Actions workflow using the official foobar2000 SDK.

## Standalone Tools

```powershell
.\build\Release\SpatialAudioDiagnostics.exe --probe --config .\config\spatial_audio_profile.ini
.\build\Release\SpatialAudioDiagnostics.exe --static-test --config .\config\spatial_audio_profile.ini
.\build\Release\SpatialAudioDiagnostics.exe --custom --config .\config\spatial_audio_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode bed --config .\config\spatial_audio_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode objects --config .\config\spatial_audio_profile.ini
```

`--probe` prints endpoint capabilities.

`--static-test` plays sequential tone bursts through the configured static bed.

`--custom` creates dynamic spatial objects from `config/spatial_audio_profile.ini` and positions them with Windows Spatial Audio coordinates.

`StereoSpatialPlayer.exe --mode bed` plays a stereo WAV through the configured static bed. `--mode objects` plays the same WAV as dynamic spatial objects.

Release ZIPs include the standalone executables and `config/spatial_audio_profile.ini`. The components are published separately as `.fb2k-component` files.

## Notes

If `max dynamic objects` is `0`, Windows Spatial Audio is not enabled for the selected endpoint, or the endpoint is not compatible.

The tools use Windows Spatial Audio endpoint APIs, not a private encoder.

## Roadmap

The foobar2000 component plan is in `docs/FOOBAR_COMPONENT_PLAN.md`.
