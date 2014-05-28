#include "../worker/common.h"
#include "../worker/Communication.h"
#include <stdio.h>

int Communication::Init(int pid)
{
    wchar_t pipeName[50];
    wsprintf(pipeName, PIPE_NAME, pid);

    hPipe_ = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hPipe_ == INVALID_HANDLE_VALUE)
    {
        DBGOUT(("Can't open pipe! code: %d", GetLastError()));
        return -1;
    }
    DWORD dwMode = PIPE_READMODE_BYTE;
    if (!SetNamedPipeHandleState(hPipe_, &dwMode, NULL, NULL))
    {
        DBGOUT(("Can't set state! code: %d", GetLastError()));
        CloseHandle(hPipe_);
        return -1;
    }

    return 0;
}

int Communication::Init2()
{
	/*BOOL fConnected = ConnectNamedPipe(pipe_, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
	if (!fConnected)
	{
		return -1;
	}*/
	return 0;
}

int Communication::Read(void* buff, int size, DWORD* readBytes)
{
    return 0;
}
                
int Communication::Write(void* buff, int size, DWORD* writtenBytes)
{
	//int ret = WriteFile(pipe_, buff, size, writtenBytes, 0);
	return 0;
}