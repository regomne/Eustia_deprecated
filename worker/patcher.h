#pragma once

#include <vector>
#include <list>
#include "ilhook.h"
extern std::list<HookSrcObject> g_HookList;

BOOL CheckInfoHook(PVOID srcAddress);
void RemoveHook(PVOID srcAddress);
void RemoveAllHooks();
