#include "jsInterfaces.h"

#include <include/v8.h>
#include <assert.h>
#include <memory>
#include <algorithm>

#define BEA_ENGINE_STATIC
#include <beaengine/BeaEngine.h>

#include "dialog.h"
#include "Memory.h"
#include "patcher.h"
#include "misc.h"
#include "asm.h"
#include "worker.h"
#include "../gs/ToolFun.h"
#include "../ControllerCmd/Process.h"


using namespace v8;
using namespace std;

Persistent<Function, CopyablePersistentTraits<Function>>* g_cloneObjectMethod;

#define THROW_EXCEPTION(str) isolate->ThrowException(Exception::Error(NEW_CONST_STRINGU(str)))

#define CHECK_ARGS_COUNT(cnt) \
if (args.Length()!=cnt)\
{\
    THROW_EXCEPTION(L"need " L###cnt L" arguments!"); \
    return;\
}

#define CHECK_ARGS_MINCOUNT(cnt) \
if (args.Length() < cnt)\
{\
    THROW_EXCEPTION(L"need at least " L###cnt L" arguments!"); \
    return; \
}

static const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

static const wchar_t* ToWString(const v8::String::Value& value)
{
    auto s = (wchar_t*)*value;
    return s ? s : L"<string conversion failed>";
}

static vector<int> ConvertIntArray(Handle<Array> jsVal)
{
    auto jsArr = jsVal.As<Array>();
    vector<int> newVec;
    for (DWORD i = 0; i < jsArr->Length(); i++)
    {
        newVec.push_back(jsArr->Get(i)->Int32Value());
    }
    return newVec;
}



// static void SetProperty(const v8::FunctionCallbackInfo<v8::Value>& args)
// {
//     auto isolate = args.GetIsolate();
//     CHECK_ARGS_COUNT(4);
//     if (!(args[0]->IsObject() || args[0]->IsFunction()))
//     {
//         THROW_EXCEPTION(L"arg 1 must be an object!");
//         return;
//     }
// 
//     auto obj = args[0].As<Object>();
//     obj->ForceSet(args[1], args[2], (PropertyAttribute)args[3]->Uint32Value());
// }

static void GetMemoryBlocks(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    HandleScope handleScope(isolate);
    auto ctx = isolate->GetCurrentContext();
    auto array = Array::New(isolate, 100);
    auto hp = (HANDLE)args[0]->Int32Value(ctx).FromMaybe(0);
    vector<MEMORY_BASIC_INFORMATION> blocks;
    if (!GetMemoryBlocks(hp, blocks))
    {
        THROW_EXCEPTION(L"get mem failed");
        return;
    }

    for (DWORD i = 0; i < blocks.size(); i++)
    {
        auto obj = Object::New(isolate);
        obj->Set(NEW_CONST_STRING8("baseAddress"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].BaseAddress));
        obj->Set(NEW_CONST_STRING8("allocationBase"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].AllocationBase));
        obj->Set(NEW_CONST_STRING8("regionSize"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].RegionSize));
        obj->Set(NEW_CONST_STRING8("protect"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].Protect));
        obj->Set(NEW_CONST_STRING8("allocationProtect"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].AllocationProtect));

        array->Set(i, obj);
    }

    args.GetReturnValue().Set(array);
}

static void DumpMemory(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(4);

    auto ctx = isolate->GetCurrentContext();
    HANDLE processHandle = (HANDLE)args[0]->Uint32Value(ctx).FromMaybe(0);
    LPVOID startAddr = (LPVOID)args[1]->Uint32Value(ctx).FromMaybe(0);
    DWORD size = args[2]->Uint32Value(ctx).FromMaybe(0);
    String::Value fileName(args[3]);

    auto ret=DumpMemory(processHandle, startAddr, size, (wchar_t*)*fileName);
    args.GetReturnValue().Set(ret!=FALSE);
}

static void SuspendAllThreads(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(2);

    HandleScope scope(isolate);

    auto ctx = isolate->GetCurrentContext();
    auto processId = args[0]->Uint32Value(ctx).FromMaybe(0);
    if (!args[1]->IsArray())
    {
        THROW_EXCEPTION(L"arg 2 must be an array!");
        return;
    }
    auto ignoreIdList = ConvertIntArray(args[1].As<Array>());

    auto suspendedThreads = new vector<int>();
    
    auto ret=SuspendAllThreads(processId, ignoreIdList, *suspendedThreads);
    if (!ret)
    {
        delete suspendedThreads;
        THROW_EXCEPTION(L"Can't suspend threads!");
        return;
    }

    args.GetReturnValue().Set((uint32_t)suspendedThreads);
}

static void ResumeAllThreads(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto ctx = isolate->GetCurrentContext();
    auto suspendThreads = (vector<int>*)args[0]->Uint32Value(ctx).FromMaybe(0);

    if (suspendThreads == nullptr)
    {
        THROW_EXCEPTION(L"arg 0 must be a pointer!");
        return;
    }

    ResumeAllThreads(*suspendThreads);

    delete suspendThreads;
}

// 确保调用者拥有HandleScope
static bool ConvertElement(Isolate* isolate, void* elem, MaybeLocal<Value> jsMbVal, vector<void*>& pointers)
{
    auto rslt = true;
    auto ctx = isolate->GetCurrentContext();

    Local<Value> jsVal;
    if (!jsMbVal.ToLocal(&jsVal))
    {
        rslt = false;
    }
    else if (jsVal->IsBoolean() || jsVal->IsBooleanObject())
    {
        *(bool*)elem = jsVal->BooleanValue(ctx).FromMaybe(false);
    }
    else if (jsVal->IsNumber() || jsVal->IsNumberObject())
    {
        *(DWORD*)elem = jsVal->Uint32Value(ctx).FromMaybe(0);
    }
    else if (jsVal->IsString() || jsVal->IsStringObject())
    {
        Local<String> ss;
        if (!jsVal->ToString(ctx).ToLocal(&ss))
        {
            rslt = false;
        }
        else
        {
            //String::Utf8Value str(jsVal);
            auto strBuff = new BYTE[ss->Length() + 1];
            //memcpy(strBuff, *str, str.length());
            //strBuff[str.length()] = 0;
            auto wroteBytes = ss->WriteOneByte(strBuff);
            //DBGOUTHEX((strBuff, wroteBytes));
            pointers.push_back(strBuff);
            *(BYTE**)elem = strBuff;
        }
    }
    else if (jsVal->IsArray())
    {
        auto jsArr = jsVal.As<Array>();
        auto arrBuff = new void*[jsArr->Length()];
        pointers.push_back(arrBuff);
        for (DWORD i = 0; i < jsArr->Length(); i++)
        {
            arrBuff[i] = 0;
            auto cvtRslt = ConvertElement(isolate, &arrBuff[i], jsArr->Get(ctx, i), pointers);
            if (!cvtRslt)
                rslt = false;
        }
        *(void**)elem = arrBuff;
    }
    else
    {
        rslt = false;
    }
    return rslt;
}

static void CallFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_MINCOUNT(2);
    HandleScope handleScope(isolate);

    auto ctx = isolate->GetCurrentContext();
    auto funcAddr = args[0]->Uint32Value(ctx).FromMaybe(0);
    auto callType = (FunctionCallType)args[1]->Uint32Value(ctx).FromMaybe(0);
    Registers regs;
    DWORD regFlags = 0;
    vector<DWORD> arguments;
    vector<void*> pointers;

    if (args.Length() >= 3)
    {
        if (!args[2]->IsObject())
        {
            THROW_EXCEPTION(L"arg3 must be a object");
            return;
        }
        auto regsObj = args[2].As<Object>();
        for (int i = 0; i < 8; i++)
        {
            if (regsObj->Has(ctx, i).FromMaybe(false))
            {
                regFlags |= (1 << i);
                *((DWORD*)&regs + (i + 1)) = regsObj->Get(ctx, i).ToLocalChecked()->Uint32Value(ctx).FromMaybe(0);
            }
        }
    }
    if (args.Length() >= 4)
    {
        if (!args[3]->IsArray())
        {
            THROW_EXCEPTION(L"arg4 must be an array");
            return;
        }
        auto argsArray = args[3].As<Array>();
        for (DWORD i = 0; i < argsArray->Length(); i++)
        {
            DWORD arg = 0;
            auto cvtRslt = ConvertElement(isolate, &arg, argsArray->Get(i), pointers);
            if (!cvtRslt)
            {
                DBGOUT(("arg %d cvt failed.", i));
            }
            arguments.push_back(arg);
        }
    }
    ReturnValues callRetVal = { 0 };
    auto callRslt = CallFunction(funcAddr, callType, arguments, regFlags, &regs, &callRetVal);
    for (int i = pointers.size() - 1; i >= 0; i--)
    {
        delete[](char*)pointers[i];
    }
    if (!callRslt)
    {
        THROW_EXCEPTION(L"exception occured.");
    }
    else
    {
        auto retArr = Array::New(isolate, 4);
        retArr->Set(0, Integer::NewFromUnsigned(isolate, callRetVal.eax));
        retArr->Set(1, Integer::NewFromUnsigned(isolate, callRetVal.edx));
        retArr->Set(2, Number::New(isolate, (double)callRetVal.st0));
        retArr->Set(3, Integer::NewFromUnsigned(isolate, callRetVal.xmm0));
        args.GetReturnValue().Set(retArr);
    }
}

static void GetAPIAddress(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();

    HandleScope handleScope(isolate);

    CHECK_ARGS_COUNT(1);

    String::Value str(args[0]);
    auto pstr = (wchar_t *)*str;
    auto p = wcsrchr(pstr, L'.');
    PVOID addr = 0;
    if (!p)
    {
        auto len = wcslen(pstr);
        shared_ptr<char> cs(new char[(len + 1) * 2], CharDeleter);
        WideCharToMultiByte(CP_ACP, 0, pstr, -1, cs.get(), (len + 1) * 2, 0, 0);

        auto rslt = GetAPIAddress(0, cs.get(), &addr);
        if (!rslt)
        {
            addr = 0;
        }
    }
    else
    {
        auto len = wcslen(p + 1);
        shared_ptr<char> funcName(new char[(len + 1) * 2], CharDeleter);
        WideCharToMultiByte(CP_ACP, 0, p + 1, -1, funcName.get(), (len + 1) * 2, 0, 0);
        funcName.get()[len] = '\0';

        len = p - pstr;
        shared_ptr<wchar_t> modName(new wchar_t[len + 1], WcharDeleter);
        memcpy(modName.get(), pstr, len * 2);
        modName.get()[len] = L'\0';

        auto rslt = GetAPIAddress(modName.get(), funcName.get(), &addr);
        if (!rslt)
        {
            addr = 0;
        }
    }

    args.GetReturnValue().Set((uint32_t)addr);
}

static void Mread(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();

    auto ctx = isolate->GetCurrentContext();
    if (args.Length() != 2 || !args[0]->IsUint32() || !args[1]->IsUint32())
    {
        THROW_EXCEPTION(L"Bad parameters");
        return;
    }

    auto start = args[0]->Uint32Value(ctx).FromMaybe(0);
    auto len = args[1]->Uint32Value(ctx).FromMaybe(0);
    Local<String> str;
    __try
    {
        auto mbStr = String::NewFromOneByte(args.GetIsolate(), (uint8_t*)start, NewStringType::kNormal, len);
        if (!mbStr.ToLocal(&str))
        {
            THROW_EXCEPTION(L"Can't create string with mem!");
        }
        else
            args.GetReturnValue().Set(str);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        DBGOUT(("Mread: err: start: 0x%x, len: 0x%x", start, len));
        THROW_EXCEPTION(L"Mem access violation!");
    }
    return;
}

static void Mwrite(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    if (args.Length() != 4 || !args[0]->IsUint32())
    {
        THROW_EXCEPTION(L"Bad parameters");
        return;
    }

    auto ctx = isolate->GetCurrentContext();
    auto addr = args[0]->Uint32Value(ctx).FromMaybe(0);
    auto str = args[1]->ToString(ctx).ToLocalChecked();
    auto start = args[2]->Uint32Value(ctx).FromMaybe(0);
    auto len = args[3]->Uint32Value(ctx).FromMaybe(0);
    if ((int)start > str->Length())
    {
        start = str->Length();
    }
    if ((int)(start + len) > str->Length())
    {
        len = str->Length() - start;
    }

    __try
    {
        auto wroteBytes = str->WriteOneByte((uint8_t*)addr, start, len, String::NO_NULL_TERMINATION);
        args.GetReturnValue().Set(wroteBytes);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DBGOUT(("Mwrite:err: start: 0x%x, len: 0x%x", start, len));
        THROW_EXCEPTION(L"Can't write memory");
    }
}

static void Disassmble(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto ctx = isolate->GetCurrentContext();
    auto eip = args[0]->Uint32Value(ctx).FromMaybe(0);
    //auto addr = args[1]->Uint32Value();

    DISASM disasm = { 0 };
    disasm.Options = Tabulation | PrefixedNumeral;
    disasm.EIP = (UIntPtr)eip;
    int len;
    __try
    {
        len = Disasm(&disasm);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        THROW_EXCEPTION(L"Exception occured!");
        return;
    }
    if (len == UNKNOWN_OPCODE)
    {
        THROW_EXCEPTION(L"Unknown opcode");
        return;
    }

    auto obj = Object::New(isolate);
    Local<String> ss;
    obj->Set(NEW_CONST_STRING8("length"), Integer::NewFromUnsigned(isolate, len));
    bool bCvt = String::NewFromOneByte(isolate, (uint8_t*)disasm.CompleteInstr, NewStringType::kNormal, -1).ToLocal(&ss);
    if (!bCvt)
    {
        THROW_EXCEPTION(L"Can't cvt bytes to string!");
        return;
    }
    obj->Set(NEW_CONST_STRING8("string"), ss);
    args.GetReturnValue().Set(obj);
}

static void CheckInfoHook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    if (args.Length() != 1 || !args[0]->IsUint32())
    {
        THROW_EXCEPTION(L"Bad parameters");
        return;
    }

    auto ctx = isolate->GetCurrentContext();
    auto addr = args[0]->Uint32Value(ctx).FromMaybe(0);
    if (!CheckInfoHook2((PVOID)addr))
    {
        args.GetReturnValue().Set(false);
    }
    else
    {
        args.GetReturnValue().Set(true);
    }
    return;
}

static void NewCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(3);

    auto ctx = isolate->GetCurrentContext();
    auto funcId = args[0]->Uint32Value(ctx).FromMaybe(-1);
    auto argsCnt = args[1]->Uint32Value(ctx).FromMaybe(0);
    auto funcType = (FunctionCallType)args[2]->Uint32Value(ctx).FromMaybe(0);

    void* newFunc;
    if (!CreateNewFunction(funcId, argsCnt, funcType, &newFunc))
    {
        THROW_EXCEPTION(L"Can't create function.");
    }
    else
    {
        args.GetReturnValue().Set((uint32_t)newFunc);
    }
}

static void NewMem(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto ctx = isolate->GetCurrentContext();
    auto size = args[0]->Uint32Value(ctx).FromMaybe(0);
    void* mem = nullptr;
    try
    {
        mem = new char[size];
    }
    catch (...)
    { /// no mem	
    }
    args.GetReturnValue().Set((int)mem);
}

static void DeleteMem(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto ctx = isolate->GetCurrentContext();
    auto mem = (wchar_t*)args[0]->Uint32Value(ctx).FromMaybe(0);
    delete[] mem;
}

static void Unhook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    if (args.Length() != 1 || !args[0]->IsUint32())
    {
        THROW_EXCEPTION(L"Bad parameters");
        return;
    }

    auto addr = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
    RemoveHook((PVOID)addr);
}


// Reads a file into a v8 string.
v8::Local<v8::String> ReadJSFile(v8::Isolate* isolate, const wchar_t* name) {
    FILE* file = _wfopen(name, L"rb");
    if (file == NULL) return v8::Local<v8::String>();

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    char* chars = new char[size + 2];
    chars[size] = '\0';
    chars[size + 1] = '\0';
    for (int i = 0; i < size;) {
        int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
        i += read;
    }
    fclose(file);

    Local<String> result;
    bool ret;
    if (size >= 3 && !memcmp(chars, "\xef\xbb\xbf", 3))
    {
        ret = String::NewFromUtf8(isolate, chars + 3, NewStringType::kNormal, size - 3).ToLocal(&result);
    }
    else if (size >= 2 && !memcmp(chars, "\xff\xfe", 2))
    {
        ret = String::NewFromTwoByte(isolate, (uint16_t*)(chars + 2), NewStringType::kNormal, size / 2 - 1).ToLocal(&result);
    }
    else
    {
        ret = String::NewFromUtf8(isolate, chars, NewStringType::kNormal, size).ToLocal(&result);
    }
    delete[] chars;
    if (!ret)
        return Local<String>();

    return result;
}

static void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        }
        else {
            OutputWriter::OutputInfo(L" ");
        }
        v8::String::Value str(args[i]);
        auto cstr = ToWString(str);
        OutputWriter::OutputInfo(L"%s", cstr);
    }
    OutputWriter::OutputInfo(L"\r\n");
}
// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
static void ReadText(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    if (args.Length() != 1) {
        THROW_EXCEPTION(L"Bad parameters");
        return;
    }
    v8::String::Value file(args[0]);
    if (*file == NULL) {
        THROW_EXCEPTION(L"Error loading filename");
        return;
    }
    v8::Local<v8::String> source = ReadJSFile(isolate, (wchar_t*)*file);
    if (source.IsEmpty()) {
        THROW_EXCEPTION(L"Error loading file");
        return;
    }
    args.GetReturnValue().Set(source);
}

static void WriteText(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(3);

    auto ctx = isolate->GetCurrentContext();
    String::Value fname(args[0]);
    auto buff = args[1]->ToString();
    auto isTwoByte = args[2]->Uint32Value(ctx).FromMaybe(0);
    
    if (*fname == NULL)
    {
        THROW_EXCEPTION(L"error file name");
        return;
    }

    auto fp=_wfopen((wchar_t*)*fname, L"wb");
    if (fp == NULL)
    {
        THROW_EXCEPTION(L"Error opening file");
        return;
    }

    BYTE tmp[0x1000];
    if (isTwoByte)
    {
        fwrite("\xff\xfe", 1, 2, fp);
        int left = buff->Length()*2;
        int start = 0;
        while (left>0)
        {
            int curWriteBytes = left > 0x1000 ? 0x1000 : left;
            buff->Write((uint16_t*)tmp, start/2, curWriteBytes/2);
            curWriteBytes = fwrite(tmp, 1, curWriteBytes, fp);
            start += curWriteBytes;
            left -= curWriteBytes;
        }
    }
    else
    {
        int left = buff->Length();
        int start = 0;
        while (left > 0)
        {
            int curWriteBytes = left > 0x1000 ? 0x1000 : left;
            buff->WriteOneByte(tmp, start, curWriteBytes);
            curWriteBytes = fwrite(tmp, 1, curWriteBytes, fp);
            start += curWriteBytes;
            left -= curWriteBytes;
        }
    }

    fclose(fp);
}

// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
static void LoadJS(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(isolate);
        v8::String::Value file(args[i]);
        if (*file == NULL) {
            THROW_EXCEPTION(L"Error loading filename");
            return;
        }
        v8::Local<v8::String> source = ReadJSFile(isolate, (wchar_t*)*file);
        if (source.IsEmpty()) {
            THROW_EXCEPTION(L"Error loading file");
            return;
        }
        //Local<String> fname
        if (!ExecuteString(args.GetIsolate(),
            source,
            args[i],
            false,
            true)) {
            THROW_EXCEPTION(L"Error executing file");
            return;
        }
    }
}

void CloneObject(Isolate* isolate, Handle<Value> source, Handle<Value> target)
{
    HandleScope scope(isolate);

    Handle<Value> args[] = { source, target };

    // Init
    if (g_cloneObjectMethod == nullptr) {
        Handle<Function> cloneObjectMethod_ = Handle<Function>::Cast(
            Script::Compile(NEW_CONST_STRING8(
            "(function(source, target) {\n\
            Object.getOwnPropertyNames(source).forEach(function(key) {\n\
            try {\n\
            var desc = Object.getOwnPropertyDescriptor(source, key);\n\
            if (desc.value === source) desc.value = target;\n\
            Object.defineProperty(target, key, desc);\n\
            } catch (e) {\n\
            // Catch sealed properties errors\n\
            }\n\
            });\n\
            })"), String::NewFromUtf8(isolate,"binding:script"))->Run());
        g_cloneObjectMethod = new Persistent<Function, CopyablePersistentTraits<Function>>(isolate,cloneObjectMethod_);
    }

    auto recv_ = Object::New(isolate);
    auto clone = Local<Function>::New(isolate, *g_cloneObjectMethod);
    clone->Call(recv_, 2, args);
}
//import(context,content,filename)
static void ImportJS(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(3);

    if (!args[0]->IsObject())
    {
        THROW_EXCEPTION(L"Context must be an object.");
        return;
    }
    
    HandleScope scope(isolate);

    auto sandbox = args[0].As<Object>();
    auto content = args[1]->ToString();
    auto filename = args[2]->ToString();
    
    //auto globalTmp = Local<ObjectTemplate>::New(isolate, g_globalTemplate);
    auto context = Context::New(isolate, 0);
    CloneObject(isolate, sandbox, context->Global()->GetPrototype());
    context->Enter();

    ExecuteString(isolate, content, filename, false, true);

    context->Exit();
    CloneObject(isolate, context->Global()->GetPrototype(), sandbox);
}

static void ExistsFile(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    String::Value fname(args[0]->ToString());

    DWORD ret = GetFileAttributesW((wchar_t*)*fname);
    bool isFile = true;
    if (ret == INVALID_FILE_ATTRIBUTES || ret&FILE_ATTRIBUTE_DIRECTORY)
        isFile = false;
    args.GetReturnValue().Set(isFile);
}

static void ToFloat(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto val = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(Number::New(isolate, (double)(*(float*)&val)));
}
static void ToFloatp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto val = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(Number::New(isolate, (double)*(float*)val));
}
static void ToDoublep(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto val = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(Number::New(isolate, *(double*)val));
}

static void OutputStateGetter(Local<String> prop, const PropertyCallbackInfo<Value>& info)
{
    info.GetReturnValue().Set(Integer::New(info.GetIsolate(), (int)OutputWriter::GetOutputStream()));
}

static void OutputStateSetter(Local<String> prop, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
    auto st = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    OutputWriter::ChangeOutputStream((OutputWriter::DispState)st);
}

static void NamePropertyGetter(Local<String> prop, const PropertyCallbackInfo<Value>& info)
{
    auto isolate = info.GetIsolate();
    HandleScope scope(isolate);

    String::Utf8Value propStr(prop);
    if (!strcmp(*propStr, "__this__"))
    {
        info.GetReturnValue().Set(info.This()->GetInternalField(0));
        return;
    }

    auto func = info.Data().As<Function>();
    auto fakeObj = info.This()->GetInternalField(0);
    Handle<Value> args[] = { prop };
    auto ret = func->Call(fakeObj, 1, args);
    info.GetReturnValue().Set(ret);

}

static void NamedFunctionConstructor(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    auto fakeObj = Object::New(isolate);
    args.This()->SetInternalField(0, fakeObj);
}

static void NewFunctionWithNamedAccessor(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_MINCOUNT(1);

    if (!args[0]->IsFunction())
    {
        THROW_EXCEPTION(L"arg 1 must be a function!");
        return;
    }

    auto funcTemp = FunctionTemplate::New(isolate, NamedFunctionConstructor);
    auto objTemp = funcTemp->InstanceTemplate();

    objTemp->SetNamedPropertyHandler(
        NamePropertyGetter,
        (NamedPropertySetterCallback)0,
        (NamedPropertyQueryCallback)0,
        (NamedPropertyDeleterCallback)0,
        (NamedPropertyEnumeratorCallback)0,
        args[0]);
    objTemp->SetInternalFieldCount(1);

    auto func = funcTemp->GetFunction();
    args.GetReturnValue().Set(func);
}

static void CreateIntervalThread(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto param = args[0]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
    if (!param)
    {
        THROW_EXCEPTION(L"arg 1 must be a pointer!");
        return;
    }

    auto handle = CreateThread(0, 0, IntervalThread, (LPVOID)param, 0, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        THROW_EXCEPTION(L"Can't create the thread!");
        return;
    }

    args.GetReturnValue().Set((int)handle);
}

Handle<Context> InitV8()
{
    Isolate* isolate = Isolate::GetCurrent();

    //HandleScope scope(isolate);
    Handle<ObjectTemplate> global = ObjectTemplate::New(isolate);

    struct FuncList
    {
        char* funcName;
        FunctionCallback callBack;
    } funcList[] = {
        //{ "_SetProperty", SetProperty },
        { "_NewFunctionWithNamedAccessor", NewFunctionWithNamedAccessor },

        { "_Print", Print },
        { "_LoadJS", LoadJS },
        { "_ImportJS", ImportJS },
        { "_ExistsFile", ExistsFile },
        { "_ReadText", ReadText },
        { "_WriteText", WriteText },

        { "_Mread", Mread },
        { "_Mwrite", Mwrite },
        { "_GetMemoryBlocks", GetMemoryBlocks },
        { "_DumpMemory", DumpMemory },
        { "_SuspendAllThreads", SuspendAllThreads },
        { "_ResumeAllThreads", ResumeAllThreads },
        { "_CreateIntervalThread", CreateIntervalThread },

        { "_CheckInfoHook", CheckInfoHook },
        { "_Unhook", Unhook },
        { "_GetAPIAddress", GetAPIAddress },
        { "_CallFunction", CallFunction },
        { "_NewCallback", NewCallback },
        { "_NewMem", NewMem },
        { "_DeleteMem", DeleteMem },

        { "_Disassemble", Disassmble },

        { "_ToFloat", ToFloat},
        { "_ToFloatp", ToFloatp },
        { "_ToDoublep", ToDoublep },

    };

    for (int i = 0; i < sizeof(funcList) / sizeof(FuncList); i++)
    {
        global->Set(String::NewFromUtf8(isolate, funcList[i].funcName, NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(isolate, funcList[i].callBack));
    }

    global->Set(NEW_CONST_STRING8("_DllPath"), String::NewFromTwoByte(isolate, (uint16_t*)g_dllPath.c_str(), NewStringType::kNormal).ToLocalChecked());
    global->Set(NEW_CONST_STRING8("_MyWindowThreadId"), Integer::NewFromUnsigned(isolate,g_myWindowThreadId));
    global->SetAccessor(NEW_CONST_STRING8("_OutputState"), OutputStateGetter, OutputStateSetter);
    
    //g_globalTemplate = Persistent<ObjectTemplate, CopyablePersistentTraits<ObjectTemplate>>(isolate, global);
    //auto gl = Local<ObjectTemplate>::New(isolate, g_globalTemplate);
    return Context::New(isolate, NULL, global);
}

void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
    v8::HandleScope handle_scope(isolate);
    v8::String::Value exception(try_catch->Exception());
    auto exception_string = ToWString(exception);
    v8::Handle<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        OutputWriter::OutputInfo(L"%s\n", exception_string);
    }
    else {
        // Print (filename):(line number): (message).
        v8::String::Value filename(message->GetScriptOrigin().ResourceName());
        auto filename_string = ToWString(filename);
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        int linenum = message->GetLineNumber(context).FromJust();
        OutputWriter::OutputInfo(L"%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        v8::String::Value sourceline(message->GetSourceLine(context).ToLocalChecked());
        auto sourceline_string = ToWString(sourceline);
        OutputWriter::OutputInfo(L"%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            OutputWriter::OutputInfo(L" ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            OutputWriter::OutputInfo(L"^");
        }
        OutputWriter::OutputInfo(L"\n");
        v8::String::Value stack_trace(try_catch->StackTrace(context).ToLocalChecked());
        if (stack_trace.length() > 0) {
            auto stack_trace_string = ToWString(stack_trace);
            OutputWriter::OutputInfo(L"%s\n", stack_trace_string);
        }
    }
}

bool ExecuteString(v8::Isolate* isolate,
    v8::Handle<v8::String> source,
    v8::Handle<v8::Value> name,
    bool print_result,
    bool report_exceptions)
{
    v8::HandleScope handle_scope(isolate);
    auto result = ExecuteStringWithRet(isolate, source, name, report_exceptions);
    if (result.IsEmpty())
    {
        return false;
    }
    if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Value str(result);
        auto cstr = ToWString(str);
        OutputWriter::OutputInfo(L"%s\r\n", cstr);
    }
    return true;
}

Handle<Value> ExecuteStringWithRet(v8::Isolate* isolate,
    v8::Handle<v8::String> source,
    v8::Handle<v8::Value> name,
    bool report_exceptions)
{
    v8::Handle<v8::Value> result;
    //v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch;
    v8::ScriptOrigin origin(name);
    v8::Handle<v8::Script> script = v8::Script::Compile(source, &origin);
    if (script.IsEmpty()) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ReportException(isolate, &try_catch);
        return result;
    }
    else {
        result = script->Run();
        if (result.IsEmpty()) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ReportException(isolate, &try_catch);
            return result;
        }
        else {
            assert(!try_catch.HasCaught());
            return result;
        }
    }
}
