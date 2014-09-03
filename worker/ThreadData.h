
#pragma once

#include <windows.h>
#include "../gs/ToolFun.h"

class ThreadData
{
    struct Data1
    {
        int enterFlag;
    };

    static int tlsIndex_;

    static Data1* GetData()
    {
        auto data = (Data1*)TlsGetValue(tlsIndex_);
        if (data == 0)
        {
            data = new Data1;

            data->enterFlag = 0;

            if (TlsSetValue(tlsIndex_, data) == 0)
            {
                DBGOUT(("Can't set tls value, err: %d", GetLastError()));
                delete data;
                data = 0;
            }
        }
        return data;
    }

    static void ReleaseData()
    {
        auto data = (Data1*)TlsGetValue(tlsIndex_);
        if (data)
        {
            delete data;
            TlsSetValue(tlsIndex_, 0);
        }
    }

public:

    static bool Init()
    {
        tlsIndex_ = TlsAlloc();
        if (tlsIndex_ == TLS_OUT_OF_INDEXES)
        {
            DBGOUT(("tls alloc faild, error: %d", GetLastError()));
            return false;
        }
        return true;
    }

    static void EnterThread()
    {
        GetData();
    }

    static void ExitThread()
    {
        auto data = (Data1*)TlsGetValue(tlsIndex_);
        if (data)
        {
            delete data;
        }
    }

    static void Release()
    {
        TlsFree(tlsIndex_);
    }

    static int GetEnterFlag()
    {
        auto data = GetData();

        return data->enterFlag;
    }

    static bool SetEnterFlag(int flag)
    {
        auto data = GetData();
        data->enterFlag = flag;
        if (!TlsSetValue(tlsIndex_, data))
        {
            DBGOUT(("Can't set tls value, err:%d", GetLastError()));
            return false;
        }
        return true;
    }
};
