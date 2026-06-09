# Foobar for Home Theater

Windows Spatial Audio output component for foobar2000 home theater setups, plus standalone diagnostics used to validate the Windows endpoint.

`foo_out_spatial_audio` is an early foobar2000 output component. It takes foobar2000 stereo or 5.1 PCM, forces 48 kHz when needed, and renders it to the default Windows Spatial Audio endpoint as a static home theater bed.

The repository builds the Windows tools and foobar2000 component with GitHub Actions on `windows-2022`. The action can verify MSVC/CMake compilation, but real Spatial Audio playback must be tested on a Windows machine with a compatible HDMI/eARC endpoint.

## Requirements

- Windows 10/11 with a recent Windows SDK.
- Visual Studio 2022 with C++ desktop tools.
- CMake 3.24 or newer.
- A real HDMI/eARC endpoint with Windows Spatial Audio support selected as the default output device.
- Spatial sound enabled for that endpoint in Windows sound settings.
- foobar2000 2.x 64-bit for the component package.

## Install

Download `foo_out_spatial_audio.fb2k-component` from the `nightly` release and install it from foobar2000 Preferences > Components. After restart, select `Spatial Audio for Home Theater` as the output device.

The component settings live under Preferences > Playback > Output > Spatial Audio.

Stereo input is upmixed to the available Spatial Audio bed. 5.1 input can be mapped per source channel from the component settings: FL, FR, FC, LFE, SL/BL, and SR/BR can each target front, side, rear, height, LFE, or Disabled.

## Support

[Support: Buy me a coffee](https://buymeacoffee.com/szymonrybka)

[GitHub repository](https://github.com/ArtifexEt/Foobar-for-Home-Theater)

## Build

Run these commands from the repository root.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The foobar2000 component is built by the GitHub Actions workflow using the official foobar2000 SDK.

## Standalone Tools

```powershell
.\build\Release\SpatialAudioDiagnostics.exe --probe --config .\config\spatial_audio_profile.ini
.\build\Release\SpatialAudioDiagnostics.exe --static-test --config .\config\spatial_audio_profile.ini
.\build\Release\SpatialAudioDiagnostics.exe --custom --config .\config\spatial_audio_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode bed --config .\config\spatial_audio_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode objects --config .\config\spatial_audio_profile.ini
```

`--probe` prints endpoint capabilities.

`--static-test` plays sequential tone bursts through the configured static bed. With a 7.1.4 Spatial Audio endpoint, the important checks are `top_front_left`, `top_front_right`, `top_back_left`, and `top_back_right`.

`--custom` creates dynamic spatial objects from `config/spatial_audio_profile.ini` and positions them with Windows Spatial Audio coordinates.

`StereoSpatialPlayer.exe --mode bed` plays a stereo WAV through the configured static 7.1.4 bed. `--mode objects` plays the same WAV as dynamic spatial objects using `source`, `gain_db`, `x`, `y`, and `z` from the custom object sections.

Release ZIPs include the standalone executables and `config/spatial_audio_profile.ini` in the same default layout the executables expect. The component is published separately as `foo_out_spatial_audio.fb2k-component`.

## Notes

If `max dynamic objects` is `0`, Windows Spatial Audio is not enabled for the selected endpoint, the endpoint is not compatible, or only static bed playback is available.

The tools use Windows Spatial Audio endpoint APIs, not a private encoder.

## Roadmap

The foobar2000 component plan is in `docs/FOOBAR_COMPONENT_PLAN.md`.
