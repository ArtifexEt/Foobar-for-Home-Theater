#include "stdafx.h"
#include "component_version.h"

DECLARE_COMPONENT_VERSION(
    "Spatial Audio for Home Theater",
    SPATIAL_AUDIO_COMPONENT_VERSION,
    "Windows Spatial Audio output component for home theater endpoints. Supports stereo upmix and 5.1 channel mapping to static Spatial Audio bed.\n\nRepository: https://github.com/ArtifexEt/Foobar-for-Home-Theater\nSupport: https://buymeacoffee.com/szymonrybka");

VALIDATE_COMPONENT_FILENAME("foo_out_spatial_audio.dll");
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
