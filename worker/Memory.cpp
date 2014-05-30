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