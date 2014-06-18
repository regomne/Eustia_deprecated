#pragma once

#include <memory>
#include <string>
#include <windows.h>

std::wstring GetFullModuleFileName(HMODULE mod);

void WcharDeleter(wchar_t* p);
void CharDeleter(char* p);