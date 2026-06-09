#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <SDK/foobar2000.h>

#include <windows.h>
#include <windowsx.h>
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
