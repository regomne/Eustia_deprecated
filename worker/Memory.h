#pragma once

#include <vector>
#include <windows.h>

BOOL GetMemoryBlocks(std::vector<MEMORY_BASIC_INFORMATION>&);
BOOL GetAPIAddress(wchar_t* moduleName, char* funcName, PVOID* addr);
BOOL DumpMemory(HANDLE process, void* start, int size, wchar_t* fileName);