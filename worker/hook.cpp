#include <windows.h>
#include <v8.h>
#include <Commctrl.h>
#include <memory>

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


using namespace v8;
using namespace std;

HINSTANCE g_hModule;

ConcurrentQueue<InstructionPack> SendingQueue;
ConcurrentQueue<shared_ptr<wchar_t>> CommandQueue;

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
        DWORD readBytes, writtenBytes;
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

}



int WINAPI WindowThread(LPARAM)
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
    DisableThreadLibraryCalls((HMODULE)hDllHandle);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
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
        //    MessageBox(0, L"�޷�hook����4", 0, 0);
        //    return FALSE;
        //}

        //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WaitingProc, 0, 0, 0);
        //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SendingProc, 0, 0, 0);
    }
    return TRUE;
}

int WINAPI KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    static bool installed=false;
    if (!installed && code>=0 && wParam == VK_F12)
    {
        installed = true;;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WindowThread, 0, 0, 0);
        return TRUE;
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}