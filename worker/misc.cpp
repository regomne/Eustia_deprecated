#include "misc.h"

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

DWORD CallFunction(DWORD funcAddr, FunctionCallType callType, vector<DWORD> args, DWORD regFlags,Registers* regs)
{
    DWORD argCnt = args.size();
    DWORD localArgs[30];
    if (argCnt > 30)
    {

    }
}