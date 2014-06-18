

#include "dialog.h"
#include <windows.h>
#include <memory>
#include "resource.h"
#include "ConcurrentQueue.h"
#include <v8.h>
#include "jsInterfaces.h"
#include "worker.h"
#include <vector>
#include <map>
#include "misc.h"
#include "common.h"

using namespace v8;
using namespace std;

static const bool g_DisplayRslt=true;

static CommandBuffer g_CmdBuffer;
static CommandBuffer g_ShortCmdBuffer;
map<HWND,CommandBuffer*> g_BufferSelector;

bool OutputWriter::isNotDisplay = false;

ConcurrentQueue<JSCommand> CommandQueue;

HWND g_hDlgMain;
static WNDPROC g_OldEditProc;

DWORD WINAPI CommandProc(LPARAM param)
{
    auto isolate = Isolate::New();
    isolate->Enter();

    HandleScope handle_scope(isolate);

    auto context = InitV8();
    context->Enter();

    {
        auto moduleFileName = GetFullModuleFileName(g_hModule);
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

            auto parserJsFileName= moduleFileName.substr(0, it+1) + L"parser.js";
            name=String::NewFromTwoByte(isolate, (uint16_t*)parserJsFileName.c_str());
            source = ReadJSFile(isolate, parserJsFileName.c_str());
            if (source.IsEmpty() || !ExecuteString(isolate, source, name, false, true) ||
                !context->Global()->Get(String::NewFromUtf8(isolate,"ParseShortCmd"))->IsFunction())
            {
                OutputWriter::OutputInfo(L"parser.js faild, short cmd disabled.\n");
                EnableWindow(GetDlgItem(g_hDlgMain,IDC_INPUTCMD),FALSE);
            }
        }
    }

    JSCommand cmd;
    while (true)
    {
        if (CommandQueue.Dequeue(cmd))
        {
            auto source = String::NewFromTwoByte(isolate, (uint16_t*)cmd.text.get());
            ExecuteString(isolate, source, String::NewFromUtf8(isolate, "console"), g_DisplayRslt, true);
            //OutputWriter::OutputInfo(L"\n");
            if(cmd.compFlag)
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
    if(uMsg==WM_KEYDOWN)
    {
        if(wParam==VK_UP || wParam==VK_DOWN)
        {
            auto itr=g_BufferSelector.find(hwnd);
            if(itr!=g_BufferSelector.end())
            {
                auto cmdBuffer=itr->second;
                auto newIdx=cmdBuffer->curIdx+((wParam==VK_UP)?-1:1);
                if(newIdx<0 || newIdx>=(int)cmdBuffer->buffer.size())
                    goto _End;

                cmdBuffer->curIdx=newIdx;
                SetWindowText(hwnd,cmdBuffer->buffer[newIdx].c_str());
                return 0;
            }
        }
    }
    _End:
    return CallWindowProc(g_OldEditProc,hwnd,uMsg,wParam,lParam);
}

bool AddParseString(shared_ptr<wchar_t>& text)
{
    wstring cmd=text.get();
    bool hasQuote=(cmd.find(L'\'')!=wstring::npos);
    bool hasDquote=(cmd.find(L'"')!=wstring::npos);
    //bool hasEscape=(cmd.find(L'\\')!=wstring::npos);
    if(hasQuote && !hasDquote)
        cmd=L"ParseShortCmd(\""+cmd+L"\")";
    else if(!(hasDquote && hasQuote))
        cmd=L"ParseShortCmd('"+cmd+L"')";
    else
        return false;
    wcscpy(text.get(),cmd.c_str());
    return true;
}

void ReadCmdAndExecute(HWND hEdit)
{
    auto itr=g_BufferSelector.find(hEdit);
    if(itr!=g_BufferSelector.end())
    {
        bool isShortCmd=(itr->second==&g_ShortCmdBuffer);

        auto textLen = GetWindowTextLength(hEdit);
        if (textLen == 0)
            return;

        //textLen+20 for ParseShortCmd(...);
        JSCommand cmd={
            shared_ptr<wchar_t>(new wchar_t[textLen + 20], [](wchar_t* p){delete[] p; }),
            0
        };
        GetWindowText(hEdit, cmd.text.get(), textLen + 1);

        if(isShortCmd)
        {
            OutputWriter::OutputInfo(L"$ ");
            OutputWriter::OutputInfo(cmd.text);
            OutputWriter::OutputInfo(L"\n");
            auto buffer=itr->second;
            buffer->buffer.push_back(cmd.text.get());
            buffer->curIdx=buffer->buffer.size();

            if(!AddParseString(cmd.text))
            {
                OutputWriter::OutputInfo(L"invalid short cmd\n");
                SetWindowText(hEdit, L"");
                return;
            }

            CommandQueue.Enqueue(cmd);
        }
        else
        {
            OutputWriter::OutputInfo(L"> ");
            OutputWriter::OutputInfo(cmd.text);
            OutputWriter::OutputInfo(L"\n");
            auto buffer=itr->second;
            buffer->buffer.push_back(cmd.text.get());
            buffer->curIdx=buffer->buffer.size();

            CommandQueue.Enqueue(cmd);
       }


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
    static HWND hCheck1;
    static HWND hCheck2;

//#define DEF_CONST_SHARE_STRING(name, str) shared_ptr<wchar_t> name(new wchar_t[wcslen(str)+1]);wcscpy(name .get(),str);

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (wParam & 0xffff)
        {
        //case IDC_CHECKMEM1:
        //{
        //                      DEF_CONST_SHARE_STRING(cmd, L"mm1=getMemoryBlocks()");
        //                      OutputWriter::OutputInfo(L"Working...\n");
        //                      EnableWindow(hCheck1, FALSE);
        //                      EnableWindow(hCheck2, FALSE);
        //                      //OutputWriter::ChangeDisplay(false);
        //                      CommandQueue.Enqueue(cmd);
        //}
        //    break;
        //case IDC_CHECKMEM2:
        //{
        //                      DEF_CONST_SHARE_STRING(cmd, L"mm2=getMemoryBlocks();rslt=getNewExecuteMemory(mm1,mm2);");
        //                      OutputWriter::OutputInfo(L"Working...\n");
        //                      EnableWindow(hCheck1, FALSE);
        //                      EnableWindow(hCheck2, FALSE);
        //                      //OutputWriter::ChangeDisplay(false);
        //                      CommandQueue.Enqueue(cmd);

        //                      DEF_CONST_SHARE_STRING(cmd2, L"displayObject(rslt)");
        //                      CommandQueue.Enqueue(cmd2);
        //}
        //    break;
        case IDOK:
            ReadCmdAndExecute(GetFocus());
            break;
        default:
            break;
        }
        break;
    case WM_INITDIALOG:
        g_hDlgMain=hwnd;
        g_hOutputEdit = GetDlgItem(hwnd, IDC_OUTPUT);
        hInputEdit = GetDlgItem(hwnd, IDC_INPUT);
        hInputShortEdit=GetDlgItem(hwnd,IDC_INPUTCMD);
        hCheck1 = GetDlgItem(hwnd, IDC_CHECKMEM1);
        hCheck2 = GetDlgItem(hwnd, IDC_CHECKMEM2);

        g_BufferSelector[hInputEdit]=&g_CmdBuffer;
        g_BufferSelector[hInputShortEdit]=&g_ShortCmdBuffer;
        g_OldEditProc=(WNDPROC)GetWindowLongPtr(hInputShortEdit,GWL_WNDPROC);
        SetWindowLongPtr(hInputEdit,GWL_WNDPROC,(LONG)NewEditProc);
        SetWindowLongPtr(hInputShortEdit,GWL_WNDPROC,(LONG)NewEditProc);

        commandThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CommandProc, 0, 0, 0);
        //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UIProc, (LPVOID)hwnd, 0, 0);
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
