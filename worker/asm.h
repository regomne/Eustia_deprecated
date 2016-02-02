#pragma once
#include <windows.h>
#include <vector>
#include <ilhook.h>
#include <string>

#define FunctionCallTypeMask (0xf)
enum FunctionCallType
{
    FunctionCallTypeCdecl,
    FunctionCallTypeStdcall,
    //Classcall,
    //FunctionCallTypeOthercall,
};

#define FunctionCallReturnTypeMask (0xf0)
enum FunctionCallReturnType
{
    FunctionCallReturnTypeSt0 = 0x10,
    FunctionCallReturnTypeXmm0 = 0x20,
};

struct ReturnValues
{
    DWORD eax;
    DWORD edx;
    float st0;
    DWORD xmm0;
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

BOOL CallFunction(DWORD funcAddr, FunctionCallType callType, std::vector<DWORD>& args, DWORD regFlags, Registers* regs, ReturnValues* retVal);
BOOL CreateNewFunction(int funcId, int argsCnt, FunctionCallType callType, void** funcPtr);
