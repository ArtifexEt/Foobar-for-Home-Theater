#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
    "Spatial Atmos for Home Theater",
    "0.2.0",
    "Windows Spatial Audio / Dolby Atmos for Home Theater output component. Supports stereo upmix and 5.1 channel mapping to static Spatial Audio bed.\n\nRepository: https://github.com/ArtifexEt/Foobar-for-Home-Theater\nSupport: https://buymeacoffee.com/szymonrybka");

VALIDATE_COMPONENT_FILENAME("foo_out_spatial_atmos.dll");
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
