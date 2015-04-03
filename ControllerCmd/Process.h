#pragma once

#include <windows.h>
#include "../worker/Communication.h"
#include "../worker/common.h"
#include <vector>

struct ProcInfo
{
	LPVOID LdrLoadDllRoutine;
	HANDLE dllHandle;
	UNICODE_STRING dllName;
	WCHAR nameString[20];
};

extern "C"
{
	NTSYSAPI
		NTSTATUS
		NTAPI
		NtSetContextThread(
		IN HANDLE               ThreadHandle,
		IN CONTEXT*             Context);

	NTSYSAPI
		NTSTATUS
		NTAPI
		NtGetContextThread(
		IN HANDLE               ThreadHandle,
		OUT CONTEXT*            pContext);

	NTSYSAPI NTSTATUS NTAPI LdrLoadDll(
		IN PWCHAR               PathToFile OPTIONAL,
		IN ULONG                Flags OPTIONAL,
		IN PUNICODE_STRING      ModuleFileName,
		OUT PHANDLE             ModuleHandle);

}

typedef NTSTATUS(NTAPI *LoadDllRoutine)(PWCHAR, ULONG, UNICODE_STRING*, PHANDLE);
typedef int(__cdecl *DecoprFunc)(PVOID, LPDWORD, PVOID, ULONG, ULONG, ULONG);
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

struct MyDataStruct
{
	LoadDllRoutine loadDll;
	HANDLE dllHandle;
	UNICODE_STRING dllName;
};


struct IntervalThreadParam
{
    int interval;
    bool(*func)();
    bool needStop;
};

int InjectStartingProcess(HANDLE process, HANDLE thread, wchar_t* dllPath);
int CreateAndInject(TCHAR* appName, TCHAR* dllName, Communication& comm);

BOOL SuspendAllThreads(int processId, std::vector<int>& ignoreIdList, std::vector<int>& theadIdStack);
BOOL ResumeAllThreads(std::vector<int>& threadIdStack);

DWORD WINAPI IntervalThread(LPVOID lParam);