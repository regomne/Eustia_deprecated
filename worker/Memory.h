#pragma once

#include <vector>
#include <windows.h>
BOOL GetMemoryBlocks(HANDLE hp, std::vector<MEMORY_BASIC_INFORMATION>& blocks);
BOOL GetAPIAddress(wchar_t* moduleName, char* funcName, PVOID* addr);
BOOL DumpMemory(HANDLE process, void* start, int size, wchar_t* fileName);