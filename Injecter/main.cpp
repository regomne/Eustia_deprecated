#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "../worker/common.h"

int main()
{
    auto mod = LoadLibrary(DLL_NAME);
    if (!mod)
    {
        wprintf(L"Can't find " DLL_NAME L"!\n");
        return 0;
    }

    auto func = GetProcAddress(mod, "KeyboardProc");
    if (!func)
    {
        wprintf(L"Can't find MessageProc in " DLL_NAME L"!\n");
        return 0;
    }

    auto hook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)func, mod, 0);
    if (!hook)
    {
        wprintf(L"Can't set windows hook!\n");
        return 0;
    }

    wprintf(L"Hook installed.\nPress any key to exit...");
    _getch();

    UnhookWindowsHookEx(hook);
    return 0;
}