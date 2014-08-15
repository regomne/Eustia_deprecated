#if 0
#pragma once

#include <windows.h>
#include <v8.h>

class ThreadData
{
    struct Data1
    {
        HANDLE syncFlag_;
        v8::Isolate* v8isolate_;
    };

    static int tlsIndex_;

    static void InitData1(Data1* data)
    {
        data->syncFlag_ = CreateEvent(0, 0, 0, 0);
        data->v8isolate_ = 0;
    }

    static void ReleaseData1(Data1* data)
    {
        CloseHandle(data->syncFlag_);
        if (data->v8isolate_)
        {
            auto isolate = data->v8isolate_;
            v8::HandleScope scope(isolate);
            auto context = isolate->GetCurrentContext();

            context->Exit();
            isolate->Exit();
            isolate->Dispose();
        }
    }

public:

    static bool Init();

    static HANDLE GetSyncFlag();
    static v8::Isolate* GetIsolate();
};
#endif