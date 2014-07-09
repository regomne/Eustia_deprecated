#include "Memory.h"
#include "../gs/ToolFun.h"

BOOL GetMemoryBlocks(std::vector<MEMORY_BASIC_INFORMATION>& blocks)
{
    for (DWORD addr = 0; addr<=0x80000000;)
    {
        /*if (IsBadReadPtr((void*)addr, 1))
        {
            addr -= 0x1000;
            continue;
        }*/

        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void*)addr, &mbi, sizeof(mbi)))
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