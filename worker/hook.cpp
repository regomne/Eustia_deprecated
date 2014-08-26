#include <windows.h>
#include <v8.h>
#include <Commctrl.h>
#include <memory>
#include <string>

#include "ilhook.h"
#include "common.h"
#include "Communication.h"
#include "../ControllerCmd/Process.h"
#include "ConcurrentQueue.h"
#include "patcher.h"
#include "../gs/ToolFun.h"
#include "jsInterfaces.h"
#include "resource.h"
#include "dialog.h"
#include "misc.h"

using namespace v8;
using namespace std;

HINSTANCE g_hModule;
DWORD g_CompFlagIndex;
DWORD g_myWindowThreadId;
DWORD g_hookWindowThreadId;
DWORD g_UIThreadId;

wstring g_dllPath;

ConcurrentQueue<InstructionPack> SendingQueue;

typedef BOOL(WINAPI *CreateProcessWRoutine)(
    HANDLE hToken,
    _In_opt_     LPCTSTR lpApplicationName,
    _Inout_opt_  LPTSTR lpCommandLine,
    _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_         BOOL bInheritHandles,
    _In_         DWORD dwCreationFlags,
    _In_opt_     LPVOID lpEnvironment,
    _In_opt_     LPCTSTR lpCurrentDirectory,
    _In_         LPSTARTUPINFO lpStartupInfo,
    _Out_        LPPROCESS_INFORMATION lpProcessInformation,
    PHANDLE newToken
    );


BOOL WINAPI MyCreateProcessW(
    CreateProcessWRoutine func,
    HANDLE token,
    _In_opt_     LPCTSTR lpApplicationName,
    _Inout_opt_  LPTSTR lpCommandLine,
    _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_         BOOL bInheritHandles,
    _In_         DWORD dwCreationFlags,
    _In_opt_     LPVOID lpEnvironment,
    _In_opt_     LPCTSTR lpCurrentDirectory,
    _In_         LPSTARTUPINFO lpStartupInfo,
    _Out_        LPPROCESS_INFORMATION lpProcessInformation,
    PHANDLE newToken
    )
{
    int hasSus = (dwCreationFlags & CREATE_SUSPENDED);
    dwCreationFlags |= CREATE_SUSPENDED;
    BOOL ret = func(token, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, newToken);
    if (ret)
    {
        HMODULE hm = GetModuleHandle(L"worker.dll");
        if (hm)
        {
            int pathLen = 256;
            wchar_t* dllPath = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, pathLen * 2);
            int retlen = GetModuleFileName(hm, dllPath, pathLen);
            while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                HeapFree(GetProcessHeap(), 0, dllPath);
                pathLen *= 2;
                dllPath = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, pathLen * 2);
                retlen = GetModuleFileName(0, dllPath, pathLen);
            };
            InjectStartingProcess(lpProcessInformation->hProcess, lpProcessInformation->hThread, dllPath);
            HeapFree(GetProcessHeap(), 0, dllPath);
        }
        if (!hasSus)
            ResumeThread(lpProcessInformation->hThread);
    }
    return ret;
}

int DispatchInstruction(Handle<Context> context, InstructionHeader* instHdr, BYTE* inst, InstructionPack* retPack)
{
    switch (instHdr->instType)
    {
    case InstructionType::ExcuteJavaScript:
        break;
    }
    return 0;
}

DWORD WINAPI WaitingProc(LPARAM param)
{

    int rslt = Communicator.Init(0); //Waiting for start.
    if (MYFAILED(rslt))
    {
        //mbox?
        DBGOUT(("no init"));
        return 0;
    }

    auto isolate = Isolate::New();
    isolate->Enter();

    HandleScope handle_scope(isolate);

    auto context = InitV8();
    context->Enter();


    DBGOUT(("Inited"));

    while (true)
    {
        DWORD readBytes;
        InstructionHeader instHdr;
        BYTE* inst;
        rslt = Communicator.Read(&instHdr, sizeof(instHdr), &readBytes);
        if (MYFAILED(rslt) || readBytes != sizeof(instHdr)) //Wait for an instruction
        {
            if (GetLastError() != ERROR_BROKEN_PIPE)
            {
                DBGOUT(("no read1"));
            }
            else
            {
                DBGOUT(("server closed the pipe."));
            }
            break;
        }

        DBGOUT(("Instruction received. Length: %d", instHdr.instLen));
        if (instHdr.instLen == 0)
        {
            inst = 0;
        }
        else
        {
            inst = new BYTE[instHdr.instLen];
            if (!inst)
            {
                //mbox
                DBGOUT(("no mem"));
                break;
            }
            rslt = Communicator.Read(inst, instHdr.instLen, &readBytes);
            if (MYFAILED(rslt))
            {
                //mbox?
                DBGOUT(("no read 2"));
                break;
            }
        }

        InstructionPack retPack;
        DBGOUT(("Dispatching..."));
        rslt = DispatchInstruction(context, &instHdr, inst, &retPack);
        if (rslt)
        {
            delete[] inst;
        }
        if (MYFAILED(rslt))
        {
            //mbox?
            DBGOUT(("no dispatcth"));
            break;
        }

        DBGOUT(("ret..."));
        SendingQueue.Enqueue(retPack);
        //rslt = Communicator.Write(&retHdr, sizeof(retHdr), &writtenBytes);
        //if (retHdr.instLen != 0)
        //{
        //    rslt = Communicator.Write(ret, retHdr.instLen, &writtenBytes);
        //}
        //if (MYFAILED(rslt))
        //{
        //    //mbox?
        //    DBGOUT(("no write"));
        //    break;
        //}

    }
    return 0;
}

DWORD WINAPI SendingProc(LPARAM param)
{
    //InstructionPack instpack;
    ////Sleep(10000);
    ////__asm int 3
    //while (true)
    //{
    //    if (SendingQueue.Dequeue(&instpack))
    //    {
    //        DBGOUT(("A Sending request."));
    //        DWORD temp;
    //        Communicator.Write(&instpack.hdr, sizeof(InstructionHeader), &temp);
    //        if (instpack.hdr.instLen != 0)
    //        {
    //            Communicator.Write(instpack.inst, instpack.hdr.instLen, &temp);
    //            delete[] instpack.inst;
    //        }
    //        DBGOUT(("Sent"));
    //    }
    //    else
    //    {
    //        Sleep(1);
    //    }
    //}
    return 0;
}



int WINAPI WindowThread(LPARAM _)
{
    /*INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);*/
    InitCommonControls();
    auto ret=DialogBoxParam(g_hModule, (LPCWSTR)IDD_CONSOLE, NULL, (DLGPROC)WndProc, 0);
    return 0;
}

int WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    g_hModule = (HINSTANCE)hDllHandle;
    //HANDLE val;
    //DisableThreadLibraryCalls((HMODULE)hDllHandle);

    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        //DBGOUT(("Entering dllmain"));
        //BYTE* buff = (BYTE*)VirtualAlloc(0, 1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        //HookSrcObject src;
        //HookStubObject stub;


        //auto hm = LoadLibrary(L"kernel32.dll");
        //auto func = GetProcAddress(hm, "CreateProcessInternalW");
        //if (!InitializeHookSrcObject(&src, func) ||
        //    !InitializeStubObject(&stub, buff, 100, 48, STUB_DIRECTLYRETURN | STUB_OVERRIDEEAX) ||
        //    !Hook32(&src, 0, &stub, MyCreateProcessW, "f123456789ABC"))
        //{
        //    MessageBox(0, L"ÎÞ·¨hookº¯Êý4", 0, 0);
        //    return FALSE;
        //}

        //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WaitingProc, 0, 0, 0);
        //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SendingProc, 0, 0, 0);
        g_dllPath = GetDllPath(g_hModule);
        if (g_dllPath.length() == 0)
        {
            MessageBox(0, L"Can't get dll path", 0, 0);
        }

        //g_CompFlagIndex = TlsAlloc();
        //if(g_CompFlagIndex==TLS_OUT_OF_INDEXES)
        //{
        //    DBGOUT(("tls alloc faild, error: %d",GetLastError()));
        //    return FALSE;
        //}

        // fall through        
    case DLL_THREAD_ATTACH:
        //val=TlsGetValue(g_CompFlagIndex);
        //if(val==0)
        //{
        //    val = CreateEvent(0, 0, 0, 0);
        //    //DBGOUT(("thread entered. tid:%d, eve: %x",GetCurrentThreadId(),val));
        //    TlsSetValue(g_CompFlagIndex,val);
        //}
        break;
    case DLL_THREAD_DETACH:
        //val=(long*)TlsGetValue(g_CompFlagIndex);
        //if(val)
        //{
        //    //DBGOUT(("thread exit. tid:%d",GetCurrentThreadId()));
        //    CloseHandle(val);
        //}
        //if (GetCurrentThreadId() == g_hookWindowThreadId)
        //{
        //    MessageBox(0, L"main thread exited!", 0, 0);
        //}
        break;
    case DLL_PROCESS_DETACH:
        //TlsFree(g_CompFlagIndex);
        RemoveAllHooks();
        break;
    }
    
    return TRUE;
}

int WINAPI KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    static bool installed=false;
    //DBGOUT(("captured, key: %x", wParam));
    if (!installed && code>=0 && wParam == VK_F11)
    {
        installed = true;;
        g_hookWindowThreadId = GetCurrentThreadId();
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WindowThread, 0, 0, &g_myWindowThreadId);
        
        return TRUE;
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

int WINAPI GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
    static bool installed = false;
    auto msg = (MSG*)lParam;
    //DBGOUT(("captured, msg: %x, wp: %x, lp: %x", cwp->message, cwp->wParam, cwp->lParam));

    if (code >= 0 && CHECK_JSENGINE_MSG(msg->wParam, msg->lParam))
    {
        ProcessEngineMsg(msg);
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}