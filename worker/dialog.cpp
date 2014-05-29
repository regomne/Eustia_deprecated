

#include "dialog.h"
#include <windows.h>
#include <memory>
#include "resource.h"
#include "ConcurrentQueue.h"
#include <v8.h>
#include "jsInterfaces.h"
#include "worker.h"
#include <vector>

using namespace v8;
using namespace std;

void OutputInfo(shared_ptr<wchar_t> info)
{
    int ndx = GetWindowTextLength(g_hOutputEdit);
    SendMessage(g_hOutputEdit, EM_SETSEL, ndx, ndx);
    SendMessage(g_hOutputEdit, EM_REPLACESEL, 0, (LPARAM)info.get());
}

struct CommandBuffer
{
    vector<shared_ptr<wchar_t>> buffer;
    int curIdx;
};

static CommandBuffer g_CmdBuffer;

void OutputInfo(wchar_t *format, ...)
{
    if (!format)
        return;

    shared_ptr<wchar_t> msg(new wchar_t[0x1000], [](wchar_t*p){delete[] p; });

    va_list ap;
    va_start(ap, format);
    _vsnwprintf(msg.get(), 4096 - 1, format, ap);
    va_end(ap);

    OutputInfo(msg);
}

DWORD WINAPI CommandProc(LPARAM param)
{
    auto isolate = Isolate::New();
    isolate->Enter();

    HandleScope handle_scope(isolate);

    auto context = InitV8();
    context->Enter();

    shared_ptr<wchar_t> cmd;
    while (true)
    {
        if (CommandQueue.Dequeue(&cmd))
        {
            auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd.get());
            ExecuteString(isolate, source, String::NewFromUtf8(isolate, "console"), true, true);
            OutputInfo(L"\n");
        }
        else
        {
            Sleep(1);
        }
    }

    context->Exit();
    isolate->Exit();
    return 0;
}

HWND g_hOutputEdit;
LRESULT WINAPI WndProc(
    _In_  HWND hwnd,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    )
{
    static HWND hInputEdit;
    static HANDLE commandThread;

    int textLen;

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (wParam & 0xffff)
        {
        case IDC_INPUT:
            //OutputInfo(L"%x,%x\n", wParam >> 16, lParam);
            break;
        case IDOK:
        {
                     textLen = GetWindowTextLength(hInputEdit);
                     if (textLen == 0)
                         break;

                     shared_ptr<wchar_t> text(new wchar_t[textLen + 1], [](wchar_t* p){delete[] p; });
                     GetWindowText(hInputEdit, text.get(), textLen + 1);
                     OutputInfo(L"> ");
                     OutputInfo(text);
                     OutputInfo(L"\n");
                     CommandQueue.Enqueue(text);
                     SetWindowText(hInputEdit, L"");
                     break;
        }
        default:
            break;
        }
        break;
    case WM_INITDIALOG:
        g_hOutputEdit = GetDlgItem(hwnd, IDC_OUTPUT);
        hInputEdit = GetDlgItem(hwnd, IDC_INPUT);
        commandThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CommandProc, 0, 0, 0);
        if (!commandThread)
        {
            MessageBox(hwnd, L"Can't create command thread!", 0, 0);
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
        break;
    case WM_CLOSE:
        TerminateThread(commandThread, 0);
        FreeLibraryAndExitThread(g_hModule, 0);
        EndDialog(hwnd, 0);
        break;
    default:
        break;
    }
    return FALSE;
}
