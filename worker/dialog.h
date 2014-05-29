#pragma once

#include <windows.h>
#include <memory>

extern HWND g_hOutputEdit;


LRESULT WINAPI WndProc(
    _In_  HWND hwnd,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    );
void OutputInfo(std::shared_ptr<wchar_t> info);
void OutputInfo(wchar_t *format, ...);