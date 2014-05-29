#pragma once

#include <Windows.h>
#include "..\gs\ToolFun.h"

#define MYFAILED(x) ((x)!=0)
#define MYSUCCESS(x) ((x)==0)

#define PIPE_NAME (L"\\\\.\\pipe\\ControllerPipe%d")

#define DLL_NAME L"wnmkey.dll"


enum class InstructionType
{
	Failed=-1,
	Success = 0,
	ExcuteJavaScript,
    ReturnOfJavaScript,
};

struct InstructionHeader
{
	InstructionType instType;
	DWORD instLen;
};


struct InstructionPack
{
	InstructionHeader hdr;
	BYTE* inst;
	InstructionPack& operator=(volatile InstructionPack& b)
	{
		if (this == &b)
		{
			return *this;
		}
		hdr.instType = b.hdr.instType;
		hdr.instLen = b.hdr.instLen;
        inst = b.inst; //直接指针复制！
		return *this;
	}
};

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG           Length;
    HANDLE          RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG           Attributes;
    PVOID           SecurityDescriptor;
    PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;