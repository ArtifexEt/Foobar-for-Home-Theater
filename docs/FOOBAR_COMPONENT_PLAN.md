# Foobar component plan

This file tracks only unfinished or deliberately parked work. Completed
milestones were removed from the plan so the remaining list stays actionable.

## Current shape

`foo_out_spatial_audio` is a foobar2000 output component built from
`components/foo_out_spatial_audio`. It owns the Windows Spatial Audio stream
through `ISpatialAudioClient` and `ISpatialAudioObjectRenderStream`. The stable
baseline is static-bed playback with configurable upmix, channel calibration,
5.1 mapping, endpoint probe, and directional testing.

## Remaining useful work

### 1. Custom/object music routing in the foobar output

The standalone tools already support custom dynamic object experiments, and the
component already supports dynamic objects for directional test tones. The
missing high-value feature is routing real foobar playback into user-defined
static channels or dynamic Spatial Audio objects.

Suggested shape:

- Add a Custom page or extend the Test/Upmix UI with a compact source list.
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

### 2. Profile file import/export

Clipboard profile copy/paste is implemented and covers the common sharing path.
File import/export would still be useful for backups, release issue reports, and
switching between multiple room profiles.

Suggested shape:

- Add Export profile and Import profile controls near the existing copy/paste
  buttons.
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
