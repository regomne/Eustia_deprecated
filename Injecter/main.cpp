#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "../worker/common.h"
#include "resource.h"

#pragma comment(linker,"/entry:main1")

HHOOK hHook;

int WINAPI HookThread(LPARAM _)
{
    auto mod = LoadLibrary(DLL_NAME);
    if (!mod)
    {
        MessageBox(0, L"Can't find " DLL_NAME L"!", 0, 0);
        return 0;
    }

    auto func = GetProcAddress(mod, "GetMsgProc");
    if (!func)
    {
        MessageBox(0, L"Can't find MessageProc in " DLL_NAME L"!", 0, 0);
        return 0;
    }

    hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)func, mod, 0);
    if (!hHook)
    {
        MessageBox(0, L"Can't set windows hook message!", 0, 0);
        return 0;
    }
    func = GetProcAddress(mod, "KeyboardProc");
    if (!func)
    {
        MessageBox(0, L"Can't find Keyboard in " DLL_NAME L"!", 0, 0);
        return 0;
    }
    hHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)func, mod, 0);
    if (!hHook)
    {
        MessageBox(0, L"Can't set windows hook kbd!", 0, 0);
        return 0;
    }
    return 1;
}

int WINAPI WndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (!HookThread(0))
            ExitProcess(0);
        break;
    case WM_CLOSE:
        UnhookWindowsHookEx(hHook);
        EndDialog(hwnd, 0);
        break;
    default:
        break;
    }

    return FALSE;
}

int main1()
{

    DialogBoxParam(GetModuleHandle(0), (LPCWSTR)IDD_MAIN, 0, WndProc, 0);
    ExitProcess(0);
    return 0;
}