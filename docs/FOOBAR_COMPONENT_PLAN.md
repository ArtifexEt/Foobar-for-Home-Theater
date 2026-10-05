# Foobar component plan

This file tracks only unfinished or deliberately parked work. Completed
milestones were removed from the plan so the remaining list stays actionable.

## Current shape

The foobar2000 integration provides three cooperating components:

- `foo_dsp_spatial`, built from `components/foo_dsp_spatial`, owns audio
  processing. It receives PCM from foobar2000, including stereo, surround, and
  supplied height channels, and emits the selected surround/height layout up
  to 9.1.6.
- `foo_out_spatial_audio`, built from `components/foo_out_spatial_audio`, owns
  the Windows Spatial Audio stream through `ISpatialAudioClient` and
  `ISpatialAudioObjectRenderStream`. It follows the incoming channel mask or
  the selected layout, writes supported bed channels to static objects, and
  sends front-wide and Top Middle channels through dynamic objects.
- `foo_dsp_height`, built from `components/foo_dsp_height`, is an alternative
  chain-friendly DSP for users who already have a surround upmixer. It copies
  the existing bed unchanged and adds only missing ceiling channels for two,
  four, or six ceiling speakers. Its
  settings live only in the DSP Manager preset popup.

The DSP preferences live under Playback > DSP Manager > Spatial Audio DSP and
cover upmix, limiting, per-channel gains/delays/inversion, and 5.1 source mapping.
The output preferences live under Playback > Output > Spatial Audio Output and
cover bed layout, render rate, Top Middle positions, directional testing,
endpoint probing, and links.

Static-bed layouts remain available without dynamic objects. Front-wide
playback requires two dynamic objects, as do 5.1.6 and 7.1.6 Top Middle pairs;
9.1.6 requires four. Top Middle PCM uses private channel flags shared by the
components. Six-height playback reports missing channels or unavailable
required objects instead of silently dropping them. Physical speaker routing
requires validation with the endpoint and AVR; see [six-height setup and
validation](SIX_HEIGHT_SETUP.md).

## Remaining useful work

### 1. Extended source mapping

The DSP currently lets 5.1 source channels be remapped to output bed channels.
7.1 input is passed through to the matching bed channels. Extended mapping would
let 7.1 sources and future object-style virtual sources be routed explicitly.

Useful shape:

- Add mapping fields to `DspConfig` for 7.1 side/rear channels while keeping the
  current 5.1 defaults backward-compatible.
- Update `SerializeDspConfig`/`DeserializeDspConfig` in `dsp_config.cpp`.
- Apply the extended mapping in `spatial_dsp.cpp` for 7.1 input.
- Expand the DSP Channel Mapping UI to show the relevant source rows.

### 2. Screenshot refresh

The screenshots in `docs/screenshots/` should be refreshed after the three
components are installed together in foobar2000:

- `layout.png` — output Layout tab
- `upmix-controls.png` — DSP Upmix tab
- `channel-trims.png` — DSP Channels tab
- `channel-mapping.png` — DSP Channel Mapping tab
- `testing.png` — output Testing tab
- `about.png` — output About tab

### 3. Custom/object music routing in the foobar output

The standalone tools support custom dynamic object configurations. The output
component already routes front-wide and Top Middle PCM channels through dynamic
objects during normal playback and offers directional tests. The remaining
feature is arbitrary user-defined virtual sources and target positions beyond
those layout-specific channels.

Suggested shape:

- Add a Custom page or extend the Upmix UI with a compact source list.
- Each virtual source chooses input material: left, right, mid, side, ambience,
  height ambience, LFE, or full mix.
- Each source targets either a static bed channel or dynamic object coordinates
  using Windows Spatial Audio coordinates: x right, y up, z behind.
- Per-source gain, delay, polarity, and optional simple motion can be added once
  the static object routing is stable.
- Respect the endpoint dynamic object count and require an explicit fallback
  choice when a custom source cannot use its requested dynamic object. Keep the
  existing error behavior for incomplete six-height layouts.
- Keep LFE on the static low-frequency bed channel by default; object LFE should
  be an explicit advanced option at most.

### 4. Profile file import/export

Clipboard profile copy/paste is implemented and covers the common sharing path.
File import/export would still be useful for backups, release issue reports, and
switching between multiple room profiles. The DSP and output profile formats are
separate today, so file import/export should make that split visible.

Suggested shape:

- Add Export profile and Import profile controls to both preference surfaces.
- Use each component's existing text profile format.
- If the import UI opens a file picker, also support drag and drop onto that
  import area.
- Validate imported profiles before applying them and keep Apply as the final
  save step.

## Parked

- Arbitrary dynamic-object source routing should remain optional because
  endpoints can expose zero dynamic objects. Preserve the existing static-bed
  playback path.
