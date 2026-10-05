#pragma once

#ifdef SPATIAL_AUDIO_PORTABLE_TEST
#include "../../tests/foobar2000_test_shim.h"
#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commctrl.h>
#include <timeapi.h>
#include <helpers/foobar2000+atl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#endif
