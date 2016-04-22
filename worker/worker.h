#pragma once
#include <windows.h>
#include <memory>
#include <string>

#include "ConcurrentQueue.h"

//enable lua interface
#define ENABLE_LUA 1

extern HINSTANCE g_hModule;
extern DWORD g_CompFlagIndex;
extern std::wstring g_dllPath;
extern std::string g_dllPathA;
extern DWORD g_myWindowThreadId;
extern DWORD g_hookWindowThreadId;
extern DWORD g_UIThreadId;
extern HHOOK g_msgHook;
extern int g_isIndependent;