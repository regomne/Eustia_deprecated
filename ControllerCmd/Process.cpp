#include "../worker/common.h"
#include "Process.h"
#include "../worker/Communication.h"
#include <vector>
#include <TlHelp32.h>

using namespace std;

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


BOOL SuspendAllThreads(int processId, vector<int>& ignoreIdList, vector<int>& theadIdStack)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return(FALSE);

    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hThreadSnap, &te32))
    {
        DBGOUT(("Thread32First Failed"));  // Show cause of failure
        CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
        return(FALSE);
    }

    int owner = processId;
    int selfId = GetCurrentThreadId();
    ignoreIdList.push_back(selfId);
    do
    {
        if (te32.th32OwnerProcessID == owner)
        {
            for (int i = 0; i < ignoreIdList.size(); i++)
            {
                if (te32.th32ThreadID == ignoreIdList[i])
                    goto _Next1;
            }
            auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
            if (ht == NULL)
            {
                //log
                DBGOUT(("%d thread can't be open.", te32.th32ThreadID));
                continue;
            }
            auto ret = SuspendThread(ht);
            if (ret != -1)
                theadIdStack.push_back(te32.th32ThreadID);
            else
            {
                DBGOUT(("%d thead can't be suspended.", te32.th32ThreadID));
            }

            CloseHandle(ht);
        }
    _Next1:
        ; //傻逼c++不允许标号后直接跟}
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return(TRUE);
}

BOOL ResumeAllThreads(vector<int>& threadIdStack)
{
    for (DWORD i = 0; i < threadIdStack.size(); i++)
    {
        auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadIdStack[i]);
        if (ht == NULL)
        {
            DBGOUT(("%d thread cant be open while resume.", threadIdStack[i]));
            continue;
        }
        auto ret = ResumeThread(ht);
        if (ret == -1)
        {
            DBGOUT(("%d thread can't be resumed.", threadIdStack[i]));
        }
        CloseHandle(ht);
    }
    return TRUE;
}
