#pragma once

#include <memory>
#include <string>
#include <windows.h>
#include <vector>
#include <ilhook.h>

enum class FunctionCallType
{
    Cdecl,
    Stdcall,
    Classcall,
    Othercall,
};

std::wstring GetFullModuleFileName(HMODULE mod);

void WcharDeleter(wchar_t* p);
void CharDeleter(char* p);