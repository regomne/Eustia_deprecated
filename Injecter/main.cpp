#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "../worker/common.h"

HANDLE hSuccess;
HHOOK hHook;

int WINAPI HookThread(LPARAM _)
{
    auto mod = LoadLibrary(DLL_NAME);
    if (!mod)
    {
        wprintf(L"Can't find " DLL_NAME L"!\n");
        return 0;
    }

    auto func = GetProcAddress(mod, "GetMsgProc");
    if (!func)
    {
        wprintf(L"Can't find MessageProc in " DLL_NAME L"!\n");
        return 0;
    }

    hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)func, mod, 0);
    if (!hHook)
    {
        wprintf(L"Can't set windows hook message!\n");
        return 0;
    }
    func = GetProcAddress(mod, "KeyboardProc");
    if (!func)
    {
        wprintf(L"Can't find Keyboard in " DLL_NAME L"!\n");
        return 0;
    }
    hHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)func, mod, 0);
    if (!hHook)
    {
        wprintf(L"Can't set windows hook kbd!\n");
        return 0;
    }

    SetEvent(hSuccess);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0) != 0) 
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}

int main()
{
    
    hSuccess = CreateEvent(0, FALSE, FALSE, 0);

    DWORD thId;
    auto hth = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HookThread, 0, 0, &thId);
    if (hth == NULL)
    {
        wprintf(L"Can't create working thread!\n");
        return 0;
    }
    
    if (WaitForSingleObject(hSuccess, 1000) != WAIT_OBJECT_0)
    {
        wprintf(L"Some error occured.\n");
        return 0;
    }

    wprintf(L"Hook installed.\nPress any key to exit...");
    _getch();

    UnhookWindowsHookEx(hHook);
    //PostThreadMessage(thId, WM_QUIT, 0, 0);
    return 0;
}