#include "../worker/common.h"
#include "Process.h"
#include "../worker/Communication.h"

#define CODE_OFFSET 1024

__declspec(naked) int LoadLib()
{
	__asm{
		//_emit 0xcc
		pushad
		call lbl
		lbl :
		pop ebx
		sub ebx, CODE_OFFSET + 6

		lea eax, [ebx + 4]
		push eax
		lea eax, [ebx + 8]
		push eax
		xor eax, eax
		push eax
		push eax
		call[ebx]
		popad
		_emit 0xff
		_emit 0xff
		_emit 0xff
		_emit 0xff
		_emit 0xff
	}

}
int InjectStartingProcess(HANDLE process, HANDLE thread, wchar_t* dllPath)
{
	BYTE* CodeAddr = (BYTE*)VirtualAllocEx(process, 0, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!CodeAddr)
		return -1;
	DWORD strLen = lstrlen(dllPath);
	PWCH StringAddr = (PWCH)VirtualAllocEx(process, 0, (strLen + 1) * 2, MEM_COMMIT, PAGE_READWRITE);
	if (!StringAddr)
		return -1;

	MyDataStruct mds;

	mds.dllName.Buffer = StringAddr;
	mds.dllName.Length = (WORD)(strLen * 2);
	mds.dllName.MaximumLength = (USHORT)((strLen + 1) * 2);

	mds.loadDll = LdrLoadDll;

	int jmpPos;
	BYTE* p = (BYTE*)LoadLib;
	for (int i = 0;; i++)
	{
		if (p[i] == 0xff && *((DWORD*)(&p[i + 1])) == 0xffffffff)
		{
			jmpPos = i;
			break;
		}
	}

	CONTEXT ctt;
	ctt.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	NTSTATUS val = NtGetContextThread(thread, &ctt);
	if (!NT_SUCCESS(val))
		return -1;

	DWORD readBytes;
	BYTE inst[8];
	BOOL rslt = WriteProcessMemory(process, CodeAddr, &mds, sizeof(mds), &readBytes);
	rslt = rslt&&WriteProcessMemory(process, CodeAddr + CODE_OFFSET, LoadLib, jmpPos, &readBytes);
	inst[0] = 0xe9;
	*(DWORD*)(&inst[1]) = ctt.Eip - ((DWORD)CodeAddr + CODE_OFFSET + jmpPos + 5);
	rslt = rslt&&WriteProcessMemory(process, CodeAddr + CODE_OFFSET + jmpPos, inst, 5, &readBytes);
	rslt = rslt&&WriteProcessMemory(process, StringAddr, dllPath, strLen * 2, &readBytes);

	if (!rslt)
		return -1;

	ctt.Eip = (DWORD)CodeAddr + CODE_OFFSET;
	if (!NT_SUCCESS(NtSetContextThread(thread, &ctt)))
		return -1;

	return 0;
}

int CreateAndInject(TCHAR* appName, TCHAR* dllName, Communication& comm)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	if (GetFileAttributes(appName) == -1)
		return -1;

	if (!CreateProcess(0, appName, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &si, &pi))
		return -1;

	if (InjectStartingProcess(pi.hProcess, pi.hThread, dllName))
	{
		TerminateProcess(pi.hProcess, 0);
		return -1;
	}

	comm.Init(pi.dwProcessId);

	ResumeThread(pi.hThread);

	return 0;
}