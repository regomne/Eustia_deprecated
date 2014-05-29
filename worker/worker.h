#include <windows.h>
#include <memory>

#include "ConcurrentQueue.h"

extern ConcurrentQueue<std::shared_ptr<wchar_t>> CommandQueue;
extern HINSTANCE g_hModule;
