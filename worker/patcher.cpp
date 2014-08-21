#include "common.h"
#include "Communication.h"
#include <windows.h>
#include <intrin.h>
#include <ilhook.h>
#include <vector>
#include <list>
#include <memory>
#include "ConcurrentQueue.h"
#include "dialog.h"
#include "worker.h"
#include "../gs/toolfun.h"
#include "patcher.h"
#include "jsInterfaces.h"


using namespace std;
using namespace v8;

list<HookSrcObject> g_HookList;
CRITICAL_SECTION g_GetInfoLock;

void HOOKFUNC MyGetInfo(Registers* regs,PVOID srcAddr)
{
    JSCommand cmd={
        shared_ptr<wchar_t>(new wchar_t[100],[](wchar_t* p){delete[] p;}),
        0
    };

    swprintf_s(cmd.text.get(),100,L"Hooker.dispatchCheckFunction(%d,%d);",regs,srcAddr);
    auto compFlag=(HANDLE)TlsGetValue(g_CompFlagIndex);
    if (GetLastError() != ERROR_SUCCESS)
    {
        DBGOUT(("can't retrieve the tls value!"));
        return;
    }
    if (compFlag == 0)
    {
        compFlag = CreateEvent(0,0,0,0);
        TlsSetValue(g_CompFlagIndex, compFlag);
    }
    cmd.compFlag=compFlag;

    CommandQueue.Enqueue(cmd);
    auto rslt=WaitForSingleObject(cmd.compFlag,1000);
    if(rslt!=WAIT_OBJECT_0)
    {
        DBGOUT(("wait failed. ret: %d",rslt));
    }
}

BOOL CheckInfoHook(PVOID srcAddress)
{
    HookSrcObject srcObj;
    HookStubObject stubObj;

    auto stubBuff=new BYTE[100];
    DWORD oldProt;
    auto rslt=VirtualProtect(stubBuff,100,PAGE_EXECUTE_READWRITE,&oldProt);
    if(!rslt)
    {
        DBGOUT(("Can't change mem attr while hook %x.",srcAddress));
        return FALSE;
    }

    if(!InitializeHookSrcObject(&srcObj,srcAddress) ||
        !InitializeStubObject(&stubObj,stubBuff,100) ||
        !Hook32(&srcObj,0,&stubObj,MyGetInfo,"rs"))
    {
        DBGOUT(("Hook failed in %x",srcAddress));
        return FALSE;
    }

    g_HookList.push_back(srcObj);

    return TRUE;
}

void HOOKFUNC MyGetInfo2(Registers* regs, PVOID srcAddr)
{
    EnterCriticalSection(&g_GetInfoLock);

    int curId = GetCurrentThreadId();
    if (curId != g_hookWindowThreadId)
    {
        g_mainIsolate->Enter();
    }

    HandleScope scope(g_mainIsolate);
    auto cmd = new wchar_t[100];
    swprintf_s(cmd, 100, L"Hooker.dispatchCheckFunction(%d,%d);", regs, srcAddr);

    auto source = String::NewFromTwoByte(g_mainIsolate, (uint16_t*)cmd);
    delete[] cmd;
    auto name = String::NewFromUtf8(g_mainIsolate, "hooker");
    ExecuteString(g_mainIsolate, source, name, true, true);

    if (curId != g_hookWindowThreadId)
    {
        g_mainIsolate->Exit();
    }

    LeaveCriticalSection(&g_GetInfoLock);
}

BOOL CheckInfoHook2(PVOID srcAddress)
{
    HookSrcObject srcObj;
    HookStubObject stubObj;

    auto stubBuff = new BYTE[100];
    DWORD oldProt;
    auto rslt = VirtualProtect(stubBuff, 100, PAGE_EXECUTE_READWRITE, &oldProt);
    if (!rslt)
    {
        DBGOUT(("Can't change mem attr while hook %x.", srcAddress));
        return FALSE;
    }

    if (!InitializeHookSrcObject(&srcObj, srcAddress) ||
        !InitializeStubObject(&stubObj, stubBuff, 100) ||
        !Hook32(&srcObj, 0, &stubObj, MyGetInfo2, "rs"))
    {
        DBGOUT(("Hook failed in %x", srcAddress));
        return FALSE;
    }

    g_HookList.push_back(srcObj);

    return TRUE;
}

void RemoveHook(PVOID srcAddress)
{
    g_HookList.remove_if([&](HookSrcObject& src)
    {
        if(src.addr==srcAddress)
        {
            UnHook32(&src);
            return true;
        }
        return false;
    });
}

void RemoveAllHooks()
{
    g_HookList.remove_if([](HookSrcObject& src)
    {
        UnHook32(&src);
        return true;
    });
}
