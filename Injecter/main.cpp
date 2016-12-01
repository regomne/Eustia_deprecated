#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "../worker/common.h"
#include "resource.h"

#pragma comment(linker,"/entry:main1")

HHOOK g_hHook;
HANDLE g_hGlobalShareFile;

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

    //hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)func, mod, 0);
    //if (!hHook)
    //{
    //    MessageBox(0, L"Can't set windows hook message!", 0, 0);
    //    return 0;
    //}
    func = GetProcAddress(mod, "KeyboardProc");
    if (!func)
    {
        MessageBox(0, L"Can't find Keyboard in " DLL_NAME L"!", 0, 0);
        return 0;
    }
    g_hHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)func, mod, 0);
    if (!g_hHook)
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
    case WM_COMMAND:
        if ((wParam & 0xffff) == IDC_CHECK1)
        {
            //勾选之后，修改共享内存中的标志
            int isIndepent = IsDlgButtonChecked(hwnd, IDC_CHECK1);
            BYTE* ptr = (BYTE*)MapViewOfFile(g_hGlobalShareFile, FILE_MAP_WRITE, 0, 0, 1);
            if (ptr)
            {
                *ptr = (BYTE)isIndepent;
                UnmapViewOfFile(ptr);
            }
            else
            {
                CheckDlgButton(hwnd, IDC_CHECK1, BST_UNCHECKED);
            }
        }
        break;
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_CHECK1, BST_CHECKED);
        if (!HookThread(0))
        {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            g_hGlobalShareFile = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 1, SHARE_MEM_NAME);
            if (g_hGlobalShareFile == INVALID_HANDLE_VALUE)
            {
                MessageBox(hwnd, L"Can't create file mapping!", 0, 0);
                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }
            BYTE* ptr = (BYTE*)MapViewOfFile(g_hGlobalShareFile, FILE_MAP_WRITE, 0, 0, 1);
            if (ptr)
            {
                *ptr = 1;
                UnmapViewOfFile(ptr);
            }
        }
        break;
    case WM_CLOSE:
        if (g_hHook)UnhookWindowsHookEx(g_hHook);
        if (g_hGlobalShareFile)CloseHandle(g_hGlobalShareFile);
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