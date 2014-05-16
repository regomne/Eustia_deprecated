#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "Process.h"
#include "../worker/Communication.h"
#include "../worker/common.h"

//#pragma comment(linker,"/entry:main2")

using namespace std;
#define DBG1

#ifdef DBG1
#define DBGSTATEMENT(x) x
#else
#define DBGSTATEMENT(x)
#endif

int main3(int argc, wchar_t** argv)
{
	if (argc < 2)
	{
		printf("need 2 arguments!");
		return 0;
	}

	Communication comm;
	wchar_t* appName = new wchar_t[wcslen(argv[1]) + 1];
	wcscpy(appName, argv[1]);
	//wchar_t appName[] = L"E:\\Program\\lneditor\\lnedit.exe";
	int pathLen = 256;
	auto hm = GetModuleHandle(0);
	wchar_t* dllPath = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, pathLen * 2);
	int retlen = GetModuleFileName(hm, dllPath, pathLen);
	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		HeapFree(GetProcessHeap(), 0, dllPath);
		pathLen *= 2;
		dllPath = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, pathLen * 2);
		retlen = GetModuleFileName(0, dllPath, pathLen);
	};
	wchar_t* p = dllPath + retlen;
	for (; p>dllPath; p--)
	if (*p == L'\\')
		break;
	*(p + 1) = L'\0';
	lstrcat(dllPath, L"worker.dll");

	if (MYFAILED(CreateAndInject(appName, dllPath, comm)))
	{
		printf("failed to start");
		return 0;
	}
	HeapFree(GetProcessHeap(), 0, dllPath);
	if (MYFAILED(comm.Init2()))
	{
		printf("no init2\n");
	}
	Sleep(2000);

	InstructionHeader hdr;
	DWORD temp;
    hdr.instType = InstructionType::Success;
    hdr.instLen = 0;
    comm.Write(&hdr, sizeof(hdr), &temp);
	
	string cmd;
	while (true)
	{
        if (MYFAILED(comm.Read(&hdr, sizeof(hdr), &temp)))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                break;
        }
        if (0)
        {
            printf("Received a buffer, len: %d\n", hdr.instLen);
            if (hdr.instLen != 0)
            {
                BYTE* buff = new BYTE[hdr.instLen];
                for (int total = 0; total != hdr.instLen;)
                {
                    comm.Read(buff, hdr.instLen - total, &temp);
                    total += temp;
                }
                printf("Buff: %s", buff);
            }
        }
        else
        {
            printf("Unk header.\n");
        }
		//getline(cin, cmd);
		//if (cmd == "exit")
		//	break;
		//else if (cmd == "hookDic")
		//{
		//	hdr.instType = InstructionType::hookDic;
		//	hdr.instLen = 0;
		//	comm.Write(&hdr, sizeof(hdr), &temp);
		//}
		//else if (cmd == "hookWvm")
		//{
		//	hdr.instType = InstructionType::hookWvm;
		//	hdr.instLen = 0;
		//	comm.Write(&hdr, sizeof(hdr), &temp);
		//}
	}

	printf("ended");
	getchar();
	return 0;
}

int main()
{
	auto cmd = GetCommandLine();

	int argc;
	auto argv = CommandLineToArgvW(cmd, &argc);
	ExitProcess(main3(argc, argv));
}