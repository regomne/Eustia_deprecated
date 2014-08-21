#pragma once

#include <vector>
#include <list>
#include "ilhook.h"
extern std::list<HookSrcObject> g_HookList;
extern CRITICAL_SECTION g_GetInfoLock;


BOOL CheckInfoHook(PVOID srcAddress);
void RemoveHook(PVOID srcAddress);
void RemoveAllHooks();
BOOL CheckInfoHook2(PVOID srcAddress);
