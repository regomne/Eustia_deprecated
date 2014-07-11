#pragma once

#include <memory>
#include <string>
#include <windows.h>
#include <vector>
#include <ilhook.h>

enum FunctionCallType
{
    FunctionCallTypeCdecl,
    FunctionCallTypeStdcall,
    //Classcall,
    //FunctionCallTypeOthercall,
};

enum RegisterFlag
{
    RegisterFlagEax = (1 << 7),
    RegisterFlagEcx = (1 << 6),
    RegisterFlagEdx = (1 << 5),
    RegisterFlagEbx = (1 << 4),
    RegisterFlagEsp = (1 << 3), //not used
    RegisterFlagEbp = (1 << 2), //not used
    RegisterFlagEsi = (1 << 1),
    RegisterFlagEdi = (1 << 0),
};

std::wstring GetFullModuleFileName(HMODULE mod);
BOOL CallFunction(DWORD funcAddr, FunctionCallType callType, std::vector<DWORD> args, DWORD regFlags, Registers* regs, DWORD* retVal);

void WcharDeleter(wchar_t* p);
void CharDeleter(char* p);