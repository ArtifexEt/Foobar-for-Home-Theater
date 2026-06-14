# Foobar component plan

This file tracks only unfinished or deliberately parked work. Completed
milestones were removed from the plan so the remaining list stays actionable.

## Current shape

`foo_out_spatial_audio` is a foobar2000 output component built from
`components/foo_out_spatial_audio`. It owns the Windows Spatial Audio stream
through `ISpatialAudioClient` and `ISpatialAudioObjectRenderStream`. The stable
baseline is static-bed playback with configurable upmix, channel calibration,
channel source mapping, endpoint probe, and directional testing.

The preferences dialog has six tabs — Layout, Upmix, Channels, Channel Mapping,
Testing, About — implemented as child dialogs inside a tab control that fills the
entire preferences window. Each child dialog scrolls vertically when its content
is taller than the available area. `CDialogResizeHelper` from libPPUI stretches
the tab control when the user resizes the preferences window.

## Remaining useful work

### 1. Dynamic channel mapping rows

The Channel Mapping tab currently always shows six rows (5.1 FL/FR/FC/LFE/SL/SR).
The number of visible rows should depend on the output bed selected in the Layout
tab: stereo has two rows, 5.1 six, 7.1 eight, 5.1.2 eight, 5.1.4 ten, 7.1.4
twelve.

Required changes:

- Extend `RuntimeConfig` in `component_config.h` with mapping fields for the
  additional source channels (currently only the six 5.1 fields exist).
- Update `SerializeConfig`/`DeserializeConfig` in `component_config.cpp` to
  handle the new fields while keeping backward-compatible defaults.
- Update the audio processing code to apply the extended mapping when the source
  format has more than six channels.
- In the preferences UI, either create all rows in the RC and show/hide them at
  runtime when the layout combo changes, or create controls dynamically in
  `populate_mapping_page()` based on the current layout setting.
- The RC approach is simpler: add rows for all twelve channels, hide the unused
  ones in `populate_mapping_page()` and when `idLayoutMode` changes.

### 2. Screenshot refresh

The screenshots in `docs/screenshots/` were taken during earlier development and
no longer match the current six-tab layout. All six tabs should be re-shot:

- `layout.png` — Layout tab (Output bed, Render rate, Limiter, Copy/Paste profile)
- `upmix-controls.png` — Upmix tab
- `channel-trims.png` — Channels tab
- `channel-mapping.png` — Channel Mapping tab
- `testing.png` — Testing tab
- `about.png` — About tab (currently missing from the screenshots folder)

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
switching between multiple room profiles.

Suggested shape:

- Add Export profile and Import profile controls near the existing copy/paste
  buttons on the Layout tab.
- Use the same text profile format as the clipboard implementation.
- If the import UI opens a file picker, also support drag and drop onto that
  import area.
- Validate imported profiles before applying them and keep Apply as the final
  save step.

## Parked

- A separate helper DSP is no longer needed for the main feature. Windows
  Spatial Audio rendering belongs in the output component, where the endpoint
  stream is owned.
- Full dynamic-object music rendering should stay optional because many Windows
  Spatial Audio endpoints expose zero dynamic objects. Static bed playback is
  the reliable home-theater baseline.
