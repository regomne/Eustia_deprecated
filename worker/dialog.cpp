

#include "dialog.h"
#include <windows.h>
#include <memory>
#include "resource.h"
#include "ConcurrentQueue.h"
#include <v8.h>
#include "jsInterfaces.h"
#include "worker.h"
#include <vector>
#include "misc.h"
#include "common.h"

using namespace v8;
using namespace std;

static const bool g_DisplayRslt=false;

struct CommandBuffer
{
    vector<shared_ptr<wchar_t>> buffer;
    int curIdx;
};

static CommandBuffer g_CmdBuffer;
bool OutputWriter::isNotDisplay = false;

HANDLE g_cmdComplete;

DWORD WINAPI CommandProc(LPARAM param)
{
    auto isolate = Isolate::New();
    isolate->Enter();

    HandleScope handle_scope(isolate);

    auto context = InitV8();
    context->Enter();

    {
        auto moduleFileName = GetFullModuleFileName(GetModuleHandle(DLL_NAME));
        auto it = moduleFileName.rfind(L'\\');
        if (it != wstring::npos)
        {
            auto initJsFileName = moduleFileName.substr(0, it+1) + L"init.js";
            auto name = String::NewFromTwoByte(isolate, (uint16_t*)initJsFileName.c_str());
            auto source = ReadJSFile(isolate, initJsFileName.c_str());
            if (!source.IsEmpty() && ExecuteString(isolate, source, name, false, true))
            {
                OutputWriter::OutputInfo(L"%s loaded\n", initJsFileName.c_str());
            }
        }
    }

    shared_ptr<wchar_t> cmd;
    while (true)
    {
        if (CommandQueue.Dequeue(cmd))
        {
            auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd.get());
            ExecuteString(isolate, source, String::NewFromUtf8(isolate, "console"), g_DisplayRslt, true);
            OutputWriter::OutputInfo(L"\n");
            SetEvent(g_cmdComplete);
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

DWORD WINAPI UIProc(LPARAM lParam)
{
    auto hDialog = (HWND)lParam;
    auto hCheck1 = GetDlgItem(hDialog, IDC_CHECKMEM1);
    auto hCheck2 = GetDlgItem(hDialog, IDC_CHECKMEM2);

    while (true)
    {
        auto rslt = WaitForSingleObject(g_cmdComplete, -1);
        if (rslt == WAIT_OBJECT_0)
        {
            EnableWindow(hCheck1, TRUE);
            EnableWindow(hCheck2, TRUE);
            //OutputWriter::ChangeDisplay(true);
        }
    }
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
    static HWND hCheck1;
    static HWND hCheck2;

    int textLen;

#define DEF_CONST_SHARE_STRING(name, str) shared_ptr<wchar_t> name(new wchar_t[wcslen(str)+1]);wcscpy(name .get(),str);

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (wParam & 0xffff)
        {
        case IDC_CHECKMEM1:
        {
                              DEF_CONST_SHARE_STRING(cmd, L"mm1=getMemoryBlocks()");
                              OutputWriter::OutputInfo(L"Working...\n");
                              EnableWindow(hCheck1, FALSE);
                              EnableWindow(hCheck2, FALSE);
                              //OutputWriter::ChangeDisplay(false);
                              CommandQueue.Enqueue(cmd);
        }
            break;
        case IDC_CHECKMEM2:
        {
                              DEF_CONST_SHARE_STRING(cmd, L"mm2=getMemoryBlocks();rslt=getNewExecuteMemory(mm1,mm2);");
                              OutputWriter::OutputInfo(L"Working...\n");
                              EnableWindow(hCheck1, FALSE);
                              EnableWindow(hCheck2, FALSE);
                              //OutputWriter::ChangeDisplay(false);
                              CommandQueue.Enqueue(cmd);

                              DEF_CONST_SHARE_STRING(cmd2, L"displayObject(rslt)");
                              CommandQueue.Enqueue(cmd2);
        }
            break;
        case IDOK:
        {
                     textLen = GetWindowTextLength(hInputEdit);
                     if (textLen == 0)
                         break;

                     shared_ptr<wchar_t> text(new wchar_t[textLen + 1], [](wchar_t* p){delete[] p; });
                     GetWindowText(hInputEdit, text.get(), textLen + 1);
                     OutputWriter::OutputInfo(L"> ");
                     OutputWriter::OutputInfo(text);
                     OutputWriter::OutputInfo(L"\n");
                     CommandQueue.Enqueue(text);
                     SetWindowText(hInputEdit, L"");
        }
            break;
        default:
            break;
        }
        break;
    case WM_INITDIALOG:
        g_hOutputEdit = GetDlgItem(hwnd, IDC_OUTPUT);
        hInputEdit = GetDlgItem(hwnd, IDC_INPUT);
        hCheck1 = GetDlgItem(hwnd, IDC_CHECKMEM1);
        hCheck2 = GetDlgItem(hwnd, IDC_CHECKMEM2);
        g_cmdComplete = CreateEvent(0, FALSE, FALSE, 0);
        commandThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CommandProc, 0, 0, 0);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UIProc, (LPVOID)hwnd, 0, 0);
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
