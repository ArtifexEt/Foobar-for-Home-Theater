#include "stdafx.h"
#include "component_version.h"

DECLARE_COMPONENT_VERSION(
    "Spatial Audio Upmix",
    SPATIAL_DSP_COMPONENT_VERSION,
    "DSP upmix component: converts stereo/5.1/7.1 to 12-channel spatial bed.\n\nhttps://github.com/ArtifexEt/Foobar-for-Home-Theater");

VALIDATE_COMPONENT_FILENAME("foo_dsp_spatial.dll");
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
