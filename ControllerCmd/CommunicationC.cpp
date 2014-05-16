#include "../worker/common.h"
#include "../worker/Communication.h"
#include <stdio.h>

int Communication::Init(int pid)
{
    wchar_t clientEventName[50];

}

int Communication::Init2()
{
	BOOL fConnected = ConnectNamedPipe(pipe_, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
	if (!fConnected)
	{
		return -1;
	}
	return 0;
}

int Communication::Read(void* buff, int size, DWORD* readBytes)
{
	int ret = ReadFile(pipe_, buff, size, readBytes, 0);
	return !ret;
}

int Communication::Write(void* buff, int size, DWORD* writtenBytes)
{
	int ret = WriteFile(pipe_, buff, size, writtenBytes, 0);
	return !ret;
}