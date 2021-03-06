#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <string>
#include <include/v8.h>
#include "worker.h"
#include "ConcurrentQueue.h"

extern HWND g_hOutputEdit;
extern v8::Isolate* g_mainIsolate;
//extern long g_isProcessed;

struct CommandBuffer
{
    std::vector<std::wstring> buffer;
    int curIdx;
};

struct JSCommand
{
    std::shared_ptr<wchar_t> text;
    HANDLE compFlag;
};
extern ConcurrentQueue<JSCommand> CommandQueue;

LRESULT WINAPI WndProc(
    _In_  HWND hwnd,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    );

enum JSEngineMessage
{
    JSENGINE_INIT=WM_USER+521,
    JSENGINE_RUNCMD,
    JSENGINE_EXIT,

    LUAENGINE_INIT,
    LUAENGINE_RUNCMD,
    LUAENGINE_EXIT,
};

enum UIProcMessage
{
    UIPROC_EXIT=WM_USER+777,
    UIPROC_ADD_STRING,
};

enum class ScriptType
{
    JavaScript,
    Lua,
};

#define CHECK_JSENGINE_MSG(para1,para2) ((para1)==((para2)^0x15238958))
#define MAKE_ENGINE_PARAM(para1) ((DWORD)(para1)^0x15238958)
void LoadInitJsFiles(v8::Isolate* isolate);
void ProcessEngineMsg(MSG* msg);

class OutputWriter
{
public:
    enum DispState
    {
        Console = 1,
        DbgView = (1 << 1),
    };
public:

    static void OutputInfo(std::shared_ptr<wchar_t> info)
    {
        if (state_)
        {
            //int ndx = GetWindowTextLength(g_hOutputEdit);
            //SendMessage(g_hOutputEdit, EM_SETSEL, ndx, ndx);
            //SendMessage(g_hOutputEdit, EM_REPLACESEL, 0, (LPARAM)info.get());
            int len = wcslen(info.get());
            auto buff = new wchar_t[len + 1];
            wcscpy(buff, info.get());
            if (state_&Console)
                PostThreadMessage(g_UIThreadId, UIPROC_ADD_STRING, 0, (LPARAM)buff);
            if (state_&DbgView)
                PostThreadMessage(g_UIThreadId, UIPROC_ADD_STRING, 1, (LPARAM)buff);
        }
    }
    static void OutputInfo(const wchar_t *format, ...)
    {
        if (!format || !state_)
            return;

        int maxLen = 0x1000;
        auto msg = new wchar_t[maxLen];
        

        va_list ap;
        va_start(ap, format);
        while (true)
        {
            auto cnt = _vsnwprintf(msg, maxLen - 1, format, ap);
            if (cnt == -1)
            {
                delete[] msg;
                maxLen *= 2;
                msg = new wchar_t[maxLen];
            }
            else
                break;
        }
        va_end(ap);

        std::shared_ptr<wchar_t> msgPtr(msg, [](wchar_t*p){delete[] p; });
        OutputInfo(msgPtr);
    }
    static void OutputInfo(const char *format, ...)
    {
        if (!format || !state_)
            return;

        int maxLen = 0x1000;
        auto msg = new char[maxLen];


        va_list ap;
        va_start(ap, format);
        while (true)
        {
            auto cnt = _vsnprintf(msg, maxLen - 1, format, ap);
            if (cnt == -1)
            {
                delete[] msg;
                maxLen *= 2;
                msg = new char[maxLen];
            }
            else
                break;
        }
        va_end(ap);

        int curlen = MultiByteToWideChar(CP_UTF8, 0, msg, -1, 0, 0);
        if (curlen > 0)
        {
            auto buff = new wchar_t[curlen];
            if (buff)
            {
                MultiByteToWideChar(CP_UTF8, 0, msg, -1, buff, curlen);
                std::shared_ptr<wchar_t> msgPtr(buff, [](wchar_t*p) {delete[] p; });
                OutputInfo(msgPtr);
            }
        }
    }
    static void ChangeOutputStream(DispState st)
    {
        state_ = st;
    }
    static DispState GetOutputStream()
    {
        return state_;
    }
private:
    static DispState state_;
};

