#if 0
#include "ThreadData.h"
#include "../gs/ToolFun.h"
using namespace v8;

int ThreadData::tlsIndex_;



bool ThreadData::Init()
{
    tlsIndex_ = TlsAlloc();
    if (tlsIndex_ == TLS_OUT_OF_INDEXES)
    {
        DBGOUT(("tls alloc faild, error: %d", GetLastError()));
        return false;
    }
}

HANDLE ThreadData::GetSyncFlag()
{
    auto data = (Data1*)TlsGetValue(tlsIndex_);
    if (data == 0)
    {
        data = new Data1;
        InitData1(data);
        TlsSetValue(tlsIndex_, data);
    }

    return data->syncFlag_;
}

Isolate* ThreadData::GetIsolate()
{
    auto data = (Data1*)TlsGetValue(tlsIndex_);
    if (data == 0)
    {
        data = new Data1;
        InitData1(data);
        TlsSetValue(tlsIndex_, data);
    }
}
#endif