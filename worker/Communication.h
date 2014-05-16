#pragma once
#include <Windows.h>

class Communication
{
protected:
	HANDLE hServerReady;
    HANDLE hClientReady;
    HANDLE hMemoryMap;
    BYTE*  pSharedMemory;
public:
	Communication(){}
	int Init(int);
	int Init2();
	int Write(void* buff,int size,DWORD* writtenBytes);
	int Read(void* buff, int size, DWORD* readBytes);
};

extern Communication Communicator;