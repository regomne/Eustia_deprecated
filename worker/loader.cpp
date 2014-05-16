
#include <windows.h>


typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCH   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

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
		IN CONTEXT*             Context );

	NTSYSAPI 
		NTSTATUS
		NTAPI
		NtGetContextThread(
		IN HANDLE               ThreadHandle,
		OUT CONTEXT*            pContext );

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

void memset1(void* dest,int val,int size)
{
    for(int i=0;i<size;i++)
        ((BYTE*)dest)[i]=val;
}

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
int InjectProcess(HANDLE process, HANDLE thread, wchar_t* dllPath)
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
	mds.dllName.MaximumLength = (strLen + 1) * 2;

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

BOOL CreateAndInject(TCHAR* appName, TCHAR* dllName)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    memset1(&si,0,sizeof(si));
    si.cb=sizeof(si);

    if(GetFileAttributes(appName)==-1)
        return FALSE;

    if(!CreateProcess(0,appName,0,0,FALSE,CREATE_SUSPENDED,0,0,&si,&pi))
        return FALSE;

	if(InjectProcess(pi.hProcess,pi.hThread,dllName))
	{
		TerminateProcess(pi.hProcess,0);
		return FALSE;
	}

    ResumeThread(pi.hThread);

    return TRUE;
}