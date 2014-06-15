#pragma once

#include <vector>
#include <list>
#include "ilhook.h"
extern std::list<HookSrcObject> g_HookList;

BOOL SuspendAllThreadExpectSelf(std::vector<int>& theadIdStack);
BOOL ResumeAllThread(std::vector<int>& threadIdStack);
BOOL CheckInfoHook(PVOID srcAddress);
void RemoveHook(PVOID srcAddress);
void RemoveAllHooks();
