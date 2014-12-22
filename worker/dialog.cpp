

#include "dialog.h"
#include <windows.h>
#include <intrin.h>
#include <memory>
#include <vector>
#include <map>
#include <string>

#include "resource.h"
#include "ConcurrentQueue.h"
#include <include/v8.h>
#include <include/libplatform/libplatform.h>
#include "jsInterfaces.h"
#include "worker.h"
#include "misc.h"
#include "patcher.h"
#include "common.h"

using namespace v8;
using namespace std;

static const bool g_DisplayRslt = true; //是否显示每条js指令的返回值
//long g_isProcessed = 0;

static CommandBuffer g_CmdBuffer; //js输入缓冲
static CommandBuffer g_ShortCmdBuffer; //短命令缓冲
map<HWND, CommandBuffer*> g_BufferSelector; //

OutputWriter::DispState OutputWriter::state_ = OutputWriter::Console;

ConcurrentQueue<JSCommand> CommandQueue; //js命令队列

HWND g_hDlgMain;
static WNDPROC g_OldEditProc;

Isolate* g_mainIsolate;
Platform* g_platform;

void LoadInitJsFiles(Isolate* isolate)
{
    auto context = isolate->GetCurrentContext();
    /*auto parserJsFileName = g_dllPath + L"parser.js";
    auto name = String::NewFromTwoByte(isolate, (uint16_t*)parserJsFileName.c_str());
    auto source = ReadJSFile(isolate, parserJsFileName.c_str());
    if (source.IsEmpty() || !ExecuteString(isolate, source, name, false, true) ||
        !context->Global()->Get(String::NewFromUtf8(isolate, "ParseShortCmd"))->IsFunction())
    {
        OutputWriter::OutputInfo(L"parser.js faild, short cmd disabled.\n");
        EnableWindow(GetDlgItem(g_hDlgMain, IDC_INPUTCMD), FALSE);
    }*/
    auto initJsFileName = g_dllPath + L"init.js";
    auto name = String::NewFromTwoByte(isolate, (uint16_t*)initJsFileName.c_str());
    auto source = ReadJSFile(isolate, initJsFileName.c_str());
    if (!source.IsEmpty() && ExecuteString(isolate, source, name, false, true))
    {
        OutputWriter::OutputInfo(L"Init Success.\r\n");
        if (!context->Global()->Get(String::NewFromUtf8(isolate, "cmdparser"))->IsObject())
        {
            OutputWriter::OutputInfo(L"can't find cmdparser, short cmd disabled.\r\n");
            EnableWindow(GetDlgItem(g_hDlgMain, IDC_INPUTCMD), FALSE);
        }
    }
    else
    {
        OutputWriter::OutputInfo(L"Init failed\r\n");
    }

}

void ProcessEngineMsg(MSG* msg)
{
    if (msg->message == JSENGINE_INIT)
    {
        g_platform = platform::CreateDefaultPlatform(0);
        V8::InitializePlatform(g_platform);
        g_mainIsolate = Isolate::New();
        g_mainIsolate->Enter();
        {
            HandleScope scope(g_mainIsolate);
            auto context = InitV8();
            context->Enter();
            context->Global()->Set(String::NewFromUtf8(g_mainIsolate, "global"), context->Global()->GetPrototype());
            LoadInitJsFiles(g_mainIsolate);
        }
        InitializeCriticalSection(&g_v8ThreadLock);
    }
    else if (msg->message == JSENGINE_RUNCMD)
    {
        EnterCriticalSection(&g_v8ThreadLock);
        
        g_mainIsolate->SetStackLimit(1);
        HandleScope scope(g_mainIsolate);
        auto cmd = (wchar_t*)msg->wParam;

        auto source = String::NewFromTwoByte(g_mainIsolate, (uint16_t*)cmd);
        ExecuteString(g_mainIsolate, source, String::NewFromUtf8(g_mainIsolate, "console_main"), true, true);
        delete[] cmd;
        LeaveCriticalSection(&g_v8ThreadLock);
    }
    else if (msg->message == JSENGINE_EXIT)
    {
        HandleScope scope(g_mainIsolate);
        g_mainIsolate->GetCurrentContext()->Exit();
        g_mainIsolate->Exit();
        g_mainIsolate->Dispose();
        V8::ShutdownPlatform();
        delete g_platform;
        DeleteCriticalSection(&g_v8ThreadLock);
        //FreeLibrary(g_hModule);
    }

}
DWORD WINAPI UIProc(LPARAM param)
{
    MSG msg;
    while (GetMessage(&msg, (HWND)-1, 0, 0))
    {
        if (msg.message == UIPROC_ADD_STRING)
        {
            if (msg.wParam == 0)
            {
                int ndx = GetWindowTextLength(g_hOutputEdit);
                SendMessage(g_hOutputEdit, EM_SETSEL, ndx, ndx);
                SendMessage(g_hOutputEdit, EM_REPLACESEL, 0, msg.lParam);
                delete[](wchar_t*)msg.lParam;
            }
            else if (msg.wParam == 1)
            {
                OutputDebugStringW((wchar_t*)msg.lParam);
                delete[](wchar_t*)msg.lParam;
            }
        }
        else if (msg.message == UIPROC_EXIT)
        {
            break;
        }
    }
    return 0;
}

DWORD WINAPI CommandProc(LPARAM param)
{
    auto isolate = Isolate::New();
    isolate->Enter();

    HandleScope handle_scope(isolate);

    auto context = InitV8();
    context->Enter();
    
    LoadInitJsFiles(isolate);

    JSCommand cmd;
    while (true)
    {
        if (CommandQueue.Dequeue(cmd))
        {
            if (cmd.text.get() == nullptr)
                break;
            auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd.text.get());
            ExecuteString(isolate, source, String::NewFromUtf8(isolate, "console"), g_DisplayRslt, true);
            if (cmd.compFlag)
                SetEvent(cmd.compFlag);
        }
        else
        {
            Sleep(2);
        }
    }

    context->Exit();
    isolate->Exit();
    return 0;
}
DWORD WINAPI NewEditProc(
    _In_  HWND hwnd,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam)
{
    if (uMsg == WM_KEYDOWN)
    {
        if (wParam == VK_UP || wParam == VK_DOWN)
        {
            auto itr = g_BufferSelector.find(hwnd);
            if (itr != g_BufferSelector.end())
            {
                auto cmdBuffer = itr->second;
                auto newIdx = cmdBuffer->curIdx + ((wParam == VK_UP) ? -1 : 1);
                if (newIdx < 0 || newIdx >= (int)cmdBuffer->buffer.size())
                    goto _End;

                cmdBuffer->curIdx = newIdx;
                SetWindowText(hwnd, cmdBuffer->buffer[newIdx].c_str());
                return 0;
            }
        }
    }
_End:
    return CallWindowProc(g_OldEditProc, hwnd, uMsg, wParam, lParam);
}

bool AddParseString(shared_ptr<wchar_t>& text)
{
    wstring cmd = text.get();
    bool hasQuote = (cmd.find(L'\'') != wstring::npos);
    bool hasDquote = (cmd.find(L'"') != wstring::npos);
    //bool hasEscape=(cmd.find(L'\\')!=wstring::npos);
    if (hasQuote && !hasDquote)
        cmd = L"cmdparser.parseShortCmd(\"" + cmd + L"\")";
    else if (!(hasDquote && hasQuote))
        cmd = L"cmdparser.parseShortCmd('" + cmd + L"')";
    else
        return false;
    wcscpy(text.get(), cmd.c_str());
    return true;
}

void ReadCmdAndExecute(HWND hEdit)
{
    auto itr = g_BufferSelector.find(hEdit);
    if (itr != g_BufferSelector.end())
    {
        bool isShortCmd = (itr->second == &g_ShortCmdBuffer);

        auto textLen = GetWindowTextLength(hEdit);
        if (textLen == 0)
            return;

        //textLen+20 for ParseShortCmd(...);
        JSCommand cmd = {
            shared_ptr<wchar_t>(new wchar_t[textLen + 40], [](wchar_t* p){delete[] p; }),
            0
        };
        GetWindowText(hEdit, cmd.text.get(), textLen + 1);

        if (isShortCmd)
        {
            wstring output = L"$ ";
            output += cmd.text.get();
            output += L"\r\n";
            OutputWriter::OutputInfo(output.c_str());
            //OutputWriter::OutputInfo(cmd.text);
            //OutputWriter::OutputInfo(L"\r\n");
            auto buffer = itr->second;
            buffer->buffer.push_back(cmd.text.get());
            buffer->curIdx = buffer->buffer.size();

            if (!AddParseString(cmd.text))
            {
                OutputWriter::OutputInfo(L"Can't covert short cmd!\r\n");
                SetWindowText(hEdit, L"");
                return;
            }

        }
        else
        {
            wstring output = L"> ";
            output += cmd.text.get();
            output += L"\r\n";
            OutputWriter::OutputInfo(output.c_str());
            //OutputWriter::OutputInfo(cmd.text);
            //OutputWriter::OutputInfo(L"\r\n");
            auto buffer = itr->second;
            buffer->buffer.push_back(cmd.text.get());
            buffer->curIdx = buffer->buffer.size();


        }
        //CommandQueue.Enqueue(cmd);
        auto str = new wchar_t[wcslen(cmd.text.get()) + 1];
        wcscpy(str, cmd.text.get());

        PostThreadMessage(g_hookWindowThreadId, JSENGINE_RUNCMD, (WPARAM)str, MAKE_JSENGINE_PARAM(str));


        SetWindowText(hEdit, L"");
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
    static HWND hInputShortEdit;
    static HANDLE commandThread;
    static DWORD commandThreadId;
    static HWND hCheck1;
    static HWND hCheck2;
    JSCommand exitCommand;

    //#define DEF_CONST_SHARE_STRING(name, str) shared_ptr<wchar_t> name(new wchar_t[wcslen(str)+1]);wcscpy(name .get(),str);

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (wParam & 0xffff)
        {
        case IDOK:
            ReadCmdAndExecute(GetFocus());
            break;
        default:
            break;
        }
        break;
    case WM_INITDIALOG:
        g_hDlgMain = hwnd;
        g_hOutputEdit = GetDlgItem(hwnd, IDC_OUTPUT);
        hInputEdit = GetDlgItem(hwnd, IDC_INPUT);
        hInputShortEdit = GetDlgItem(hwnd, IDC_INPUTCMD);
        hCheck1 = GetDlgItem(hwnd, IDC_CHECKMEM1);
        hCheck2 = GetDlgItem(hwnd, IDC_CHECKMEM2);

        g_BufferSelector[hInputEdit] = &g_CmdBuffer;
        g_BufferSelector[hInputShortEdit] = &g_ShortCmdBuffer;
        g_OldEditProc = (WNDPROC)GetWindowLongPtr(hInputShortEdit, GWL_WNDPROC);
        SetWindowLongPtr(hInputEdit, GWL_WNDPROC, (LONG)NewEditProc);
        SetWindowLongPtr(hInputShortEdit, GWL_WNDPROC, (LONG)NewEditProc);

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UIProc, 0, 0, &g_UIThreadId);
        PostThreadMessage(g_hookWindowThreadId, JSENGINE_INIT, 0, MAKE_JSENGINE_PARAM(0));
        //commandThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CommandProc, 0, 0, &commandThreadId);
        //if (!commandThread)
        //{
        //    MessageBox(hwnd, L"Can't create command thread!", 0, 0);
        //    SendMessage(hwnd, WM_CLOSE, 0, 0);
        //}
        break;
    case WM_CLOSE:

        //TerminateThread(commandThread, 0);
        //exitCommand.text = shared_ptr<wchar_t>();
        //exitCommand.compFlag = 0;
        //CommandQueue.Enqueue(exitCommand);
        PostThreadMessage(g_hookWindowThreadId, JSENGINE_EXIT, 0, MAKE_JSENGINE_PARAM(0));
        PostThreadMessage(g_UIThreadId, UIPROC_EXIT, 0, 0);
        EndDialog(hwnd, 0);
        break;
    default:
        break;
    }
    return FALSE;
}
