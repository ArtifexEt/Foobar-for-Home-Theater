# Foobar component plan

This file tracks only unfinished or deliberately parked work. Completed
milestones were removed from the plan so the remaining list stays actionable.

## Current shape

The foobar2000 integration provides three cooperating components:

- `foo_dsp_spatial`, built from `components/foo_dsp_spatial`, owns audio
  processing. It receives stereo, 5.1, or 7.1 PCM from foobar2000 and emits a
  selected surround/height Spatial Audio bed.
- `foo_out_spatial_audio`, built from `components/foo_out_spatial_audio`, owns
  the Windows Spatial Audio stream through `ISpatialAudioClient` and
  `ISpatialAudioObjectRenderStream`. It receives the 12-channel bed and writes
  each channel to the matching static `ISpatialAudioObject`.
- `foo_dsp_height`, built from `components/foo_dsp_height`, is an alternative
  chain-friendly DSP for users who already have a surround upmixer. It copies
  the existing bed unchanged and adds only missing ceiling channels. Its
  settings live only in the DSP Manager preset popup.

The DSP preferences live under Tools > Spatial Audio DSP and cover upmix,
limiting, per-channel gains/delays/inversion, and 5.1 source mapping. The output
preferences live under Playback > Output > Spatial Audio for Home Theater and
cover bed layout, render rate, directional testing, endpoint probing, and links.

The reliable baseline is static-bed playback. Dynamic objects are currently used
only for directional test tones when the endpoint exposes dynamic object support.

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

The screenshots in `docs/screenshots/` should be refreshed after the two
components are installed together in foobar2000:

- `layout.png` — output Layout tab
- `upmix-controls.png` — DSP Upmix tab
- `channel-trims.png` — DSP Channels tab
- `channel-mapping.png` — DSP Channel Mapping tab
- `testing.png` — output Testing tab
- `about.png` — output About tab

### 3. Custom/object music routing in the foobar output

The standalone tools already support custom dynamic object experiments, and the
component already supports dynamic objects for directional test tones. The
missing high-value feature is routing real foobar playback into user-defined
static channels or dynamic Spatial Audio objects.

Suggested shape:

- Add a Custom page or extend the Upmix UI with a compact source list.
- Each virtual source chooses input material: left, right, mid, side, ambience,
  height ambience, LFE, or full mix.
- Each source targets either a static bed channel or dynamic object coordinates
  using Windows Spatial Audio coordinates: x right, y up, z behind.
- Per-source gain, delay, polarity, and optional simple motion can be added once
  the static object routing is stable.
- Respect the endpoint dynamic object count and gracefully fall back to static
  bed routing when the endpoint exposes no dynamic objects.
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

- Full dynamic-object music rendering should stay optional because many Windows
  Spatial Audio endpoints expose zero dynamic objects. Static bed playback is
  the reliable home-theater baseline.
