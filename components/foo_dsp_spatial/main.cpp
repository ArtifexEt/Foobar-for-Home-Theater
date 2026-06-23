#include "stdafx.h"
#include "component_version.h"

DECLARE_COMPONENT_VERSION(
    "Spatial Audio for Home Theater DSP",
    SPATIAL_DSP_COMPONENT_VERSION,
    "DSP upmix component: converts stereo/5.1/7.1 to 12-channel spatial bed for Spatial Audio for Home Theater Output.\n\nRepository: https://github.com/ArtifexEt/Foobar-for-Home-Theater\nSupport: https://buymeacoffee.com/szymonrybka");

VALIDATE_COMPONENT_FILENAME("foo_dsp_spatial.dll");
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
