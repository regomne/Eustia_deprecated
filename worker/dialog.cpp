

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
#include "luaInterface.h"
#include "worker.h"
#include "misc.h"
#include "patcher.h"
#include "common.h"

#include "..\gs\ToolFun.h"

using namespace v8;
using namespace std;

static const bool g_DisplayRslt = true; //�Ƿ���ʾÿ��jsָ��ķ���ֵ
//long g_isProcessed = 0;
ScriptType g_CurScriptType = ScriptType::JavaScript;

static CommandBuffer g_CmdBuffer; //js���뻺��
static CommandBuffer g_ShortCmdBuffer; //�������
map<HWND, CommandBuffer*> g_BufferSelector; //

OutputWriter::DispState OutputWriter::state_ = OutputWriter::Console;

ConcurrentQueue<JSCommand> CommandQueue; //js�������

HWND g_hDlgMain;
static WNDPROC g_OldEditProc;

//global info of v8
Isolate* g_mainIsolate;
Platform* g_platform;

//global info of luajit
lua_State* g_luaState;

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
    auto name = String::NewFromTwoByte(isolate, (uint16_t*)initJsFileName.c_str(), NewStringType::kNormal).ToLocalChecked();
    auto source = ReadJSFile(isolate, initJsFileName.c_str());
    if (!source.IsEmpty() && ExecuteString(isolate, source, name, false, true))
    {
        if (!context->Global()->Get(NEW_CONST_STRING8("cmdparser"))->IsObject())
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
    auto isolate = g_mainIsolate;
    static MyArrayBufferAllocator* abAlloc = nullptr;
    switch (msg->message)
    {
    case JSENGINE_INIT:
        if(!isolate)
        {
            g_platform = platform::CreateDefaultPlatform(0);
            V8::InitializePlatform(g_platform);
            V8::Initialize();
            Isolate::CreateParams create_params;
            if (!abAlloc)
            {
                abAlloc = new MyArrayBufferAllocator();
            }
            create_params.array_buffer_allocator = abAlloc;
            g_mainIsolate = Isolate::New(create_params);
            isolate = g_mainIsolate;
            isolate->Enter();
            {
                HandleScope scope(isolate);
                auto context = InitV8();
                context->Enter();
                context->Global()->Set(NEW_CONST_STRING8("global"), context->Global()->GetPrototype());
                LoadInitJsFiles(isolate);
            }
            InitializeCriticalSection(&g_v8ThreadLock);
        }
        break;
    case JSENGINE_RUNCMD:
        if(isolate)
        {
            EnterCriticalSection(&g_v8ThreadLock);

            //don't know why have to set this. will crash if not set when inject to a exe with random base.
            isolate->SetStackLimit(1);
            HandleScope scope(isolate);
            auto cmd = (wchar_t*)msg->wParam;

            auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd, NewStringType::kNormal).ToLocalChecked();
            ExecuteString(isolate, source, NEW_CONST_STRING8("console_main"), true, true);
            delete[] cmd;
            LeaveCriticalSection(&g_v8ThreadLock);
        }
        break;
    case JSENGINE_EXIT:
        if(isolate)
        {
            {
                HandleScope scope(isolate);
                delete g_cloneObjectMethod;
                g_cloneObjectMethod = nullptr;
                while (true)
                {
                    auto ctx = isolate->GetEnteredContext();
                    if (ctx.IsEmpty()) break;
                    else ctx->Exit();
                }
            }

            isolate->Exit();
            isolate->Dispose();
            g_mainIsolate = nullptr;
            if (abAlloc)
            {
                delete abAlloc;
                abAlloc = nullptr;
            }

            V8::Dispose();
            V8::ShutdownPlatform();
            delete g_platform;
            DeleteCriticalSection(&g_v8ThreadLock);

            if (!g_isIndependent)
            {
                UnhookWindowsHookEx(g_msgHook);
            }
            //FreeLibrary(g_hModule);
        }
        break;
    case LUAENGINE_INIT:
        if (!g_luaState)
        {
            g_luaState = lua_open();
            if (!g_luaState)
            {
                DBGOUT(("ProcessEngineMsg: error open lua state!"));
                break;
            }
            luaL_openlibs(g_luaState);
            InitLuajit(g_luaState);
        }
        break;
    case LUAENGINE_RUNCMD:
        if (g_luaState)
        {
            auto scrW = (wchar_t*)msg->wParam;
            if (!scrW)
                break;

            int len = WideCharToMultiByte(CP_UTF8, 0, scrW, -1, 0, 0, 0, 0);
            if (len > 0)
            {
                auto scr = new char[len];
                if (scr)
                {
                    WideCharToMultiByte(CP_UTF8, 0, scrW, -1, scr, len, 0, 0);
                    ExecuteLuaString(g_luaState, scr);
                    delete[] scr;
                }
            }
            delete[] scrW;
        }
        break;
    case LUAENGINE_EXIT:
        if (g_luaState)
        {
            lua_close(g_luaState);
        }
        break;
    default:
        break;
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

// DWORD WINAPI CommandProc(LPARAM param)
// {
//     Isolate::CreateParams create_params;
//     auto isolate = Isolate::New(create_params);
//     isolate->Enter();
// 
//     HandleScope handle_scope(isolate);
// 
//     auto context = InitV8();
//     context->Enter();
//     
//     LoadInitJsFiles(isolate);
// 
//     JSCommand cmd;
//     while (true)
//     {
//         if (CommandQueue.Dequeue(cmd))
//         {
//             if (cmd.text.get() == nullptr)
//                 break;
//             auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd.text.get());
//             ExecuteString(isolate, source, String::NewFromUtf8(isolate, "console"), g_DisplayRslt, true);
//             if (cmd.compFlag)
//                 SetEvent(cmd.compFlag);
//         }
//         else
//         {
//             Sleep(2);
//         }
//     }
// 
//     context->Exit();
//     isolate->Exit();
//     return 0;
// }
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

void ReadCmdAndExecute(HWND hEdit, int runCmdId)
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

        PostThreadMessage(g_hookWindowThreadId, runCmdId, (WPARAM)str, MAKE_ENGINE_PARAM(str));
        //DBGOUT(("ReadCmdAndExecute: tid=%d", g_hookWindowThreadId));

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
            if (g_CurScriptType == ScriptType::JavaScript)
            {
                ReadCmdAndExecute(GetFocus(), JSENGINE_RUNCMD);
            }
            else
            {
                ReadCmdAndExecute(GetFocus(), LUAENGINE_RUNCMD);
            }
            break;
        case IDC_SWITCH:
            if (g_CurScriptType == ScriptType::JavaScript)
            {
                g_CurScriptType = ScriptType::Lua;
                SetDlgItemText(hwnd, IDC_SCRIPTTYPE, L"lua:");
            }
            else if (g_CurScriptType == ScriptType::Lua)
            {
                g_CurScriptType = ScriptType::JavaScript;
                SetDlgItemText(hwnd, IDC_SCRIPTTYPE, L"js:");
            }
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

        g_BufferSelector[hInputEdit] = &g_CmdBuffer;
        g_BufferSelector[hInputShortEdit] = &g_ShortCmdBuffer;
        g_OldEditProc = (WNDPROC)GetWindowLongPtr(hInputShortEdit, GWL_WNDPROC);
        SetWindowLongPtr(hInputEdit, GWL_WNDPROC, (LONG)NewEditProc);
        SetWindowLongPtr(hInputShortEdit, GWL_WNDPROC, (LONG)NewEditProc);

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UIProc, 0, 0, &g_UIThreadId);
        PostThreadMessage(g_hookWindowThreadId, JSENGINE_INIT, 0, MAKE_ENGINE_PARAM(0));
        PostThreadMessage(g_hookWindowThreadId, LUAENGINE_INIT, 0, MAKE_ENGINE_PARAM(0));
        //DBGOUT(("WndProc: Init Completed. tid=%d", g_hookWindowThreadId));
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
        PostThreadMessage(g_hookWindowThreadId, JSENGINE_EXIT, 0, MAKE_ENGINE_PARAM(0));
        PostThreadMessage(g_hookWindowThreadId, LUAENGINE_EXIT, 0, MAKE_ENGINE_PARAM(0));
        PostThreadMessage(g_UIThreadId, UIPROC_EXIT, 0, 0);
        EndDialog(hwnd, 0);
        break;
    default:
        break;
    }
    return FALSE;
}
