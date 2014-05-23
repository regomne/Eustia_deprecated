#pragma once
#include <Windows.h>

class Communication
{
    static const int BuffSize = 0x1000;
protected:
    HANDLE hPipe_;


public:
	Communication(){}
	int Init(int);
	int Init2();
	int Write(void* buff,int size,DWORD* writtenBytes);
	int Read(void* buff, int size, DWORD* readBytes);
};

extern Communication Communicator;