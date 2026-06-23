#include "stdafx.h"
#include "component_version.h"

DECLARE_COMPONENT_VERSION(
    "Spatial Audio for Home Theater Output",
    SPATIAL_AUDIO_COMPONENT_VERSION,
    "Windows Spatial Audio output component for home theater endpoints. Sends 12-channel PCM from the Spatial Audio for Home Theater DSP to static Spatial Audio bed objects.\n\nRepository: https://github.com/ArtifexEt/Foobar-for-Home-Theater\nSupport: https://buymeacoffee.com/szymonrybka");

VALIDATE_COMPONENT_FILENAME("foo_out_spatial_audio.dll");
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
