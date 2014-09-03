#pragma once

#include <vector>
#include <list>
#include "ilhook.h"
extern std::list<HookSrcObject> g_HookList;
extern CRITICAL_SECTION g_v8ThreadLock;

enum EnterFlagsDef
{
    ENTER_FLAG_CHECK_INFO = 1,
    ENTER_FLAG_CALLBACK = (1 << 1),
};

BOOL CheckInfoHook(PVOID srcAddress);
void RemoveHook(PVOID srcAddress);
void RemoveAllHooks();
BOOL CheckInfoHook2(PVOID srcAddress);
DWORD __stdcall CallbackStub(int funcId, DWORD* argsPtr, int argCnt);
