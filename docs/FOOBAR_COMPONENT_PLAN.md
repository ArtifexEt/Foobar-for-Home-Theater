# Foobar component plan

The first checked-in program is a standalone Windows Spatial Audio experiment. The foobar2000 component should be built only after the target Windows machine confirms that Dolby Atmos for Home Theater exposes the expected static bed and dynamic object count.

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
4. Foobar output skeleton: accept PCM and forward silence/test tones to Windows Spatial Audio.
5. Stereo upmix: split incoming stereo into mid/side/ambience/LFE/height ambience and route to configured static channels.
6. Configuration UI and profile persistence.
