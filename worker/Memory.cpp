#include "Memory.h"
#include "../gs/ToolFun.h"
#include <string>

using namespace std;
BOOL GetMemoryBlocks(HANDLE hp, std::vector<MEMORY_BASIC_INFORMATION>& blocks)
{
    
    for (DWORD addr = 0; addr<0x80000000;)
    {
        /*if (IsBadReadPtr((void*)addr, 1))
        {
            addr -= 0x1000;
            continue;
        }*/

        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(hp, (void*)addr, &mbi, sizeof(mbi)))
        {
            DBGOUT(("query failed: %x", addr));
            addr += 0x1000;
            continue;
        }

        if (!(mbi.Protect&PAGE_NOACCESS))
        {
            blocks.push_back(mbi);
        }
        addr = (DWORD)((BYTE*)mbi.BaseAddress + mbi.RegionSize);
    }
    return TRUE;
}

BOOL GetAPIAddress(wchar_t* moduleName, char* funcName, PVOID* addr)
{
    DBGOUTL((L"mod: %s", moduleName));
    DBGOUT(("func: %s", funcName));
    if (moduleName)
    {
        auto mod = GetModuleHandle(moduleName);
        if (!mod)
            return FALSE;

        *addr = GetProcAddress(mod, funcName);
        if (!*addr)
        {
            return FALSE;
        }
        return TRUE;
    }

    wchar_t* mods[] = { L"ntdll.dll", L"kernel32.dll", L"kernelbase.dll", L"user32.dll" };
    for (int i = 0; i < sizeof(mods) / sizeof(wchar_t*); i++)
    {
        auto mod = GetModuleHandle(mods[i]);
        if (mod)
        {
            *addr = GetProcAddress(mod, funcName);
            if (*addr)
                return TRUE;
        }
    }
    return FALSE;
}

BOOL DumpMemory(HANDLE process, void* start, int size, wchar_t* fileName)
{
    static BYTE* memBuff = 0;
    if (!memBuff)
    {
        memBuff = new BYTE[0x10000];
    }
    auto hf = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hf == INVALID_HANDLE_VALUE)
    {
        DBGOUT(("Can't open dump file. err: %d", GetLastError()));
        return FALSE;
    }

    int leftBytes = size;
    BYTE* curStart = (BYTE*)start;
    while (leftBytes > 0)
    {
        SIZE_T readBytes=0;
        auto ret=ReadProcessMemory(process, curStart, memBuff, (leftBytes > 0x10000 ? 0x10000 : leftBytes), &readBytes);
        if (ret==0 || readBytes==0)
        {
            DBGOUT(("Can't read memory. addr:%x, err:%d, left:%x, readBytes,%x", curStart, GetLastError(),leftBytes,readBytes));
            CloseHandle(hf);
            DeleteFile(fileName);
            return FALSE;
        }

        leftBytes -= readBytes;
        curStart += readBytes;

        DWORD wroteBytes;
        WriteFile(hf, memBuff, readBytes, &wroteBytes, 0);
    }

    CloseHandle(hf);

    return TRUE;
}

