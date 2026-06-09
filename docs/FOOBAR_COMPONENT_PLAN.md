# Foobar component plan

The standalone Windows Spatial Audio experiments remain in the repository as diagnostics. The first foobar2000 output component is now `foo_out_spatial_atmos`, built from `components/foo_out_spatial_atmos`.

## Component shape

- `foo_out_spatial_atmos` should be an output component, not a plain DSP.
- A helper DSP can still exist for stereo stem extraction, but the Windows Spatial Audio stream must live in the output component.
- The output component should use `ISpatialAudioClient` and `ISpatialAudioObjectRenderStream`.
- For Atmos home theater, the main target bed is 7.1.4: front L/R/C, LFE, side L/R, back L/R, top front L/R, top back L/R.
- Dynamic object mode should be optional, because some devices or Windows configurations can report zero available dynamic objects.

## Configuration view

The foobar configuration dialog should have four compact tabs:

1. Layout
   - Output mode: Auto, Stereo, 5.1, 7.1, 5.1.2, 7.1.4, Custom.
   - Windows Spatial Audio diagnostics: endpoint name, native static bed, max dynamic objects.
   - Test buttons for front, side, rear, ceiling, and LFE.

2. Channel gains
   - Per-channel gain sliders in dB.
   - Master gain.
   - Reset and copy/paste profile controls.

3. Upmix
   - Center extraction amount.
   - Width.
   - Rear ambience.
   - Height ambience.
   - LFE low-pass frequency and gain.
   - Decorrelation strength for rear/height channels.

4. Custom mode
   - A list of virtual sources.
   - Each source selects input material: left, right, mid, side, ambience, height ambience, LFE, or full mix.
   - Each source can target either a static channel or dynamic object coordinates.
   - Coordinates use Windows Spatial Audio convention: x right, y up, z behind.
   - Per-source gain, delay, polarity, and optional simple motion.

## First implementation milestones

1. Standalone probe: verify `ISpatialAudioClient`, native static object mask, and max dynamic objects.
2. Static bed test: send sequential tone bursts into 7.1.4 static channels, especially top front/top back.
3. Dynamic object test: send synthetic tones to configured x/y/z positions.
4. Standalone stereo WAV player: route real stereo PCM into the same static bed and dynamic object paths.
5. Foobar output skeleton: accept PCM and route stereo to Windows Spatial Audio static bed.
6. Stereo upmix: tune mid/side/ambience/LFE/height ambience routing and expose per-channel controls.
7. Dynamic object mode inside the foobar output.
8. Rich configuration UI and profile import/export.
