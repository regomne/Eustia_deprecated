#pragma once

#include <windows.h>
#include <memory>
#include <vector>

extern HWND g_hOutputEdit;

struct CommandBuffer
{
    std::vector<std::wstring> buffer;
    int curIdx;
};



LRESULT WINAPI WndProc(
    _In_  HWND hwnd,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    );

class OutputWriter
{
    static bool isNotDisplay;
public:
    static void OutputInfo(std::shared_ptr<wchar_t> info)
    {
        if (!isNotDisplay)
        {
            int ndx = GetWindowTextLength(g_hOutputEdit);
            SendMessage(g_hOutputEdit, EM_SETSEL, ndx, ndx);
            SendMessage(g_hOutputEdit, EM_REPLACESEL, 0, (LPARAM)info.get());
        }
    }
    static void OutputInfo(wchar_t *format, ...)
    {
        if (!format || isNotDisplay)
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
    static void ChangeDisplay(bool isDisplay)
    {
        isNotDisplay = !isDisplay;
    }
};

