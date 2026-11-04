#pragma once
#include <cstdint>
// pylon 10's MinGW branch otherwise redeclares stdint types and uses the MS spelling.
#if defined(__MINGW32__)
#define _STDINT
#define _stdcall __stdcall
#endif
#include <pylonc/PylonC.h>
#include <pylonc/PylonCVersion.h>
#if defined(__MINGW32__)
#undef _stdcall
#undef _STDINT
#endif
