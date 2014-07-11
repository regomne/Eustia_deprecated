#include "misc.h"
#include "../gs/ToolFun.h"

using namespace std;

std::wstring GetFullModuleFileName(HMODULE hm)
{
    int pathLen = 256;
    wchar_t* dllPath = new wchar_t[pathLen];
    int retlen = GetModuleFileName(hm, dllPath, pathLen);
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        delete[] dllPath;
        pathLen *= 2;
        dllPath = new wchar_t[pathLen];
        retlen = GetModuleFileName(0, dllPath, pathLen);
    };
    
    wstring str(dllPath);
    delete[] dllPath;
    return str;
}


void WcharDeleter(wchar_t* p)
{
    delete[] p;
}

void CharDeleter(char* p)
{
    delete[] p;
}

BOOL CallFunction(DWORD funcAddr, FunctionCallType callType, vector<DWORD>& args, DWORD regFlags,Registers* regs,DWORD* retVal)
{
    DWORD argCnt = args.size();
    DWORD* argsPtr = &args[0];
    DWORD retBytes = argCnt * 4;
    __try
    {
        __asm
        {
            pushad;

            mov ecx, argCnt;
            test ecx, ecx;
            jz _lbl2;
            mov eax, argsPtr;
        _lbl1:
            push dword ptr[eax];
            add eax, 4
                dec ecx;
            jnz _lbl1;
        _lbl2:

            mov edx, regs;
            mov eax, regFlags;
            cmp eax, 0;
            je _lbl9;
            cmp edx, 0;
            je _lbl9;

            test eax, RegisterFlagEcx;
            jz _lbl3;
            mov ecx, [edx + 1ch];
        _lbl3:
            test eax, RegisterFlagEbx;
            jz _lbl4;
            mov ebx, [edx + 14h];
        _lbl4:
            //    test eax, RegisterFlagEbp;
            //    jz _lbl5;
            //    mov ebp, [edx + 0ch];
            //_lbl5:
            test eax, RegisterFlagEsi;
            jz _lbl6;
            mov esi, [edx + 8];
        _lbl6:
            test eax, RegisterFlagEdi;
            jz _lbl7;
            mov edi, [edx + 4];
        _lbl7:
            test eax, RegisterFlagEax;
            jz _lbl8;
            mov eax, [edx + 20h];
        _lbl8:
            test regFlags, RegisterFlagEdx;
            jz _lbl9;
            mov edx, [edx + 18h];
        _lbl9:
            call funcAddr;
            cmp callType, FunctionCallTypeStdcall;
            je _lbl10;
            add esp, retBytes;
        _lbl10:
            mov ecx, retVal;
            cmp ecx, 0;
            je _lbl11;
            mov[ecx], eax;
        _lbl11:

            popad;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DBGOUT(("exception occured. "));
        return FALSE;
    }
    return TRUE;
}