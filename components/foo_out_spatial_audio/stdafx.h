#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <objidl.h>
#include <shellapi.h>
#include <timeapi.h>

#include <SDK/foobar2000.h>
#include <SDK/coreDarkMode.h>

#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <spatialaudioclient.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <deque>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
