#include "Communication.h"
#include "common.h"

Communication Communicator;

int Communication::Init(int __)
{
    wchar_t someName[50];
    wsprintf(someName, PIPE_NAME, GetCurrentProcessId());
    hPipe_ = CreateNamedPipe(
        someName,
        PIPE_ACCESS_DUPLEX,       // read/write access 
        PIPE_TYPE_MESSAGE |       // message type pipe 
        PIPE_READMODE_MESSAGE |   // message-read mode 
        PIPE_WAIT,                // blocking mode 
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        BuffSize,                  // output buffer size 
        BuffSize,                  // input buffer size 
        0,                        // client time-out 
        NULL);
    if (hPipe_ == INVALID_HANDLE_VALUE)
    {
        DBGOUT(("Create pipe failed. error code: %d", GetLastError()));
        return -1;
    }

	return 0;
}

int Communication::Read(void* buff, int size, DWORD* readBytes)
{
	//int ret = ReadFile(pipe_, buff, size, readBytes, 0);
	//if (ret == 0)
	//{
	//	wchar_t temp[20];
	//	wsprintf(temp, L"%d", GetLastError());
	//	MessageBox(0, temp, 0, 0);
	//}
	return 0;
}

int Communication::Write(void* buff, int size, DWORD* writtenBytes)
{
	//return !WriteFile(pipe_, buff, size, writtenBytes, 0);
    return 0;
}