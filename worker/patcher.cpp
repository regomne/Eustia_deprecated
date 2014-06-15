#include "common.h"
#include "Communication.h"
#include <windows.h>
#include <ilhook.h>
#include <TlHelp32.h>
#include <vector>
#include <list>
#include <memory>
#include "ConcurrentQueue.h"
#include "dialog.h"
#include "worker.h"
#include "../gs/toolfun.h"
#include "patcher.h"


using namespace std;

list<HookSrcObject> g_HookList;

BOOL SuspendAllThreadExpectSelf(vector<int>& theadIdStack)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return(FALSE);

    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hThreadSnap, &te32))
    {
        DBGOUT(("Thread32First Failed"));  // Show cause of failure
        CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
        return(FALSE);
    }

    int owner = GetCurrentProcessId();
    int selfId = GetCurrentThreadId();
    do
    {
        if (te32.th32OwnerProcessID == owner && te32.th32ThreadID != selfId)
        {
            auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
            if (ht == NULL)
            {
                //log
                DBGOUT(("%d thread cant be open.", te32.th32ThreadID));
                continue;
            }
            auto ret = SuspendThread(ht);
            if (ret != -1)
                theadIdStack.push_back(te32.th32ThreadID);
            else
            {
                DBGOUT(("%d thead cant be suspended.", te32.th32ThreadID));
            }

            CloseHandle(ht);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return(TRUE);
}

BOOL ResumeAllThread(vector<int>& threadIdStack)
{
    for (DWORD i = 0; i < threadIdStack.size(); i++)
    {
        auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadIdStack[i]);
        if (ht == NULL)
        {
            DBGOUT(("%d thread cant be open while resume.", threadIdStack[i]));
            continue;
        }
        auto ret = ResumeThread(ht);
        if (ret == -1)
        {
            DBGOUT(("%d thread can't be resumed.", threadIdStack[i]));
        }
        CloseHandle(ht);
    }
    return TRUE;
}

void HOOKFUNC MyGetInfo(Registers* regs,PVOID srcAddr)
{
    JSCommand cmd={
        shared_ptr<wchar_t>(new wchar_t[100],[](wchar_t* p){delete[] p;}),
        0
    };

    swprintf_s(cmd.text.get(),100,L"Hooker.dispatchCheckFunction(%d,%d);",regs,srcAddr);
    auto compFlag=(HANDLE)TlsGetValue(g_CompFlagIndex);
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
