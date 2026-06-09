# Foobar for Home Theater

Windows Spatial Audio / Dolby Atmos for Home Theater experiments for a future foobar2000 output component.

This is not the foobar plugin yet. It contains small C++ experiments for proving the Windows Spatial Audio path before it is wrapped as a foobar2000 output component.

The repository builds the experiment with GitHub Actions on `windows-2022`. The action can verify MSVC/CMake compilation, but real Atmos playback must be tested on a Windows machine with a compatible HDMI/eARC endpoint.

## Requirements

- Windows 10/11 with a recent Windows SDK.
- Visual Studio 2022 with C++ desktop tools.
- CMake 3.24 or newer.
- A real HDMI/eARC Atmos endpoint selected as the default output device.
- Spatial sound set to Dolby Atmos for Home Theater in Windows sound settings.

## Build

Run these commands from the repository root.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Run

```powershell
.\build\Release\AtmosHomeTheaterExperiment.exe --probe --config .\config\atmos_profile.ini
.\build\Release\AtmosHomeTheaterExperiment.exe --static-test --config .\config\atmos_profile.ini
.\build\Release\AtmosHomeTheaterExperiment.exe --custom --config .\config\atmos_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode bed --config .\config\atmos_profile.ini
.\build\Release\StereoSpatialPlayer.exe --wav C:\path\to\stereo-48k.wav --mode objects --config .\config\atmos_profile.ini
```

`--probe` prints endpoint capabilities.

`--static-test` plays sequential tone bursts through the configured static bed. With a 7.1.4 Atmos endpoint, the important checks are `top_front_left`, `top_front_right`, `top_back_left`, and `top_back_right`.

`--custom` creates dynamic spatial objects from `config/atmos_profile.ini` and positions them with Windows Spatial Audio coordinates.

`StereoSpatialPlayer.exe --mode bed` plays a stereo WAV through the configured static 7.1.4 bed. `--mode objects` plays the same WAV as dynamic spatial objects using `source`, `gain_db`, `x`, `y`, and `z` from the custom object sections.

Release ZIPs include the executables and `config/atmos_profile.ini` in the same default layout the executables expect.

## Notes

If `max dynamic objects` is `0`, Windows Spatial Audio is not enabled for the selected endpoint, the endpoint is not compatible, or only static bed playback is available. The future foobar output component should handle this by falling back to static bed routing.

The experiment uses Windows Spatial Audio, not a private Dolby encoder. Dolby Atmos for Home Theater is handled by the Windows endpoint/driver when the system exposes that spatial format.

## Roadmap

The foobar2000 component plan is in `docs/FOOBAR_COMPONENT_PLAN.md`.
