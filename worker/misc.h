#pragma once

#include <memory>
#include <string>
#include <windows.h>
#include <vector>
#include <ilhook.h>

std::wstring GetFullModuleFileName(HMODULE mod);
std::wstring GetDllPath(HMODULE hm);

void WcharDeleter(wchar_t* p);
void CharDeleter(char* p);
bool ReadUtf8Text(const wchar_t* fileName, char** strPointer, int* textLen);