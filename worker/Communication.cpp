#include "Communication.h"
#include "common.h"

Communication Communicator;

int Communication::Init(int __)
{
	wchar_t pipeName[50];
	wsprintf(pipeName,PIPE_NAME,GetCurrentProcessId());

	pipe_ = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (pipe_ == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	DWORD dwMode = PIPE_READMODE_BYTE;
	if (!SetNamedPipeHandleState(pipe_, &dwMode, NULL, NULL))
	{
		CloseHandle(pipe_);
		return -1;
	}

	return 0;
}

int Communication::Read(void* buff, int size, DWORD* readBytes)
{
	int ret = ReadFile(pipe_, buff, size, readBytes, 0);
	//if (ret == 0)
	//{
	//	wchar_t temp[20];
	//	wsprintf(temp, L"%d", GetLastError());
	//	MessageBox(0, temp, 0, 0);
	//}
	return !ret;
}

int Communication::Write(void* buff, int size, DWORD* writtenBytes)
{
	return !WriteFile(pipe_, buff, size, writtenBytes, 0);
}