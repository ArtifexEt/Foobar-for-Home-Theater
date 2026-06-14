#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <shellapi.h>
#include <timeapi.h>
#include <commctrl.h>

#include <helpers/foobar2000+atl.h>
#include <SDK/coreDarkMode.h>

#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <spatialaudioclient.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <deque>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
