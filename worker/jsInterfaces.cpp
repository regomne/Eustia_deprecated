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

Persistent<ObjectTemplate, CopyablePersistentTraits<ObjectTemplate>> g_globalTemplate;

#define THROW_EXCEPTION(str) isolate->ThrowException(String::NewFromTwoByte(isolate, (uint16_t*)str))

#define CHECK_ARGS_COUNT(cnt) \
if (args.Length()!=cnt)\
{\
    isolate->ThrowException(String::NewFromTwoByte(isolate, (uint16_t*)(L"need " L###cnt L" arguments!")));\
    return;\
}

#define CHECK_ARGS_MINCOUNT(cnt) \
if (args.Length() < cnt)\
{\
    isolate->ThrowException(String::NewFromTwoByte(isolate, (uint16_t*)(L"need at least " L###cnt L" arguments!"))); \
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



static void SetProperty(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(4);
    if (!(args[0]->IsObject() || args[0]->IsFunction()))
    {
        THROW_EXCEPTION(L"arg 1 must be an object!");
        return;
    }

    auto obj = args[0].As<Object>();
    obj->ForceSet(args[1], args[2], (PropertyAttribute)args[3]->Uint32Value());
}

static void GetMemoryBlocks(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    HandleScope handleScope(isolate);
    auto array = Array::New(isolate, 100);
    auto hp = (HANDLE)args[0]->Int32Value();
    vector<MEMORY_BASIC_INFORMATION> blocks;
    if (!GetMemoryBlocks(hp, blocks))
    {
        isolate->ThrowException(String::NewFromTwoByte(isolate, (uint16_t*)L"get mem failed"));
        return;
    }

    for (DWORD i = 0; i < blocks.size(); i++)
    {
        auto obj = Object::New(isolate);
        obj->Set(String::NewFromUtf8(isolate, "baseAddress"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].BaseAddress));
        obj->Set(String::NewFromUtf8(isolate, "allocationBase"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].AllocationBase));
        obj->Set(String::NewFromUtf8(isolate, "regionSize"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].RegionSize));
        obj->Set(String::NewFromUtf8(isolate, "protect"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].Protect));
        obj->Set(String::NewFromUtf8(isolate, "allocationProtect"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].AllocationProtect));

        array->Set(i, obj);
    }

    args.GetReturnValue().Set(array);
}

static void DumpMemory(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(4);

    HANDLE processHandle = (HANDLE)args[0]->Uint32Value();
    LPVOID startAddr = (LPVOID)args[1]->Uint32Value();
    DWORD size = args[2]->Uint32Value();
    String::Value fileName(args[3]);

    auto ret=DumpMemory(processHandle, startAddr, size, (wchar_t*)*fileName);
    args.GetReturnValue().Set(ret!=FALSE);
}

static void SuspendAllThreads(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(2);

    HandleScope scope(isolate);

    auto processId = args[0]->Uint32Value();
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

    auto suspendThreads = (vector<int>*)args[0]->Uint32Value();

    ResumeAllThreads(*suspendThreads);

    delete suspendThreads;
}

static bool ConvertElement(void* elem, Handle<Value> jsVal, vector<void*>& pointers)
{
    auto rslt = true;
    if (jsVal->IsBoolean() || jsVal->IsBooleanObject())
    {
        *(bool*)elem = jsVal->BooleanValue();
    }
    else if (jsVal->IsInt32() || jsVal->IsUint32() || jsVal->IsNumber() || jsVal->IsNumberObject())
    {
        *(DWORD*)elem = jsVal->Uint32Value();
    }
    else if (jsVal->IsString() || jsVal->IsStringObject())
    {
        auto ss = jsVal->ToString();
        
        //String::Utf8Value str(jsVal);
        auto strBuff = new BYTE[ss->Length() + 1];
        //memcpy(strBuff, *str, str.length());
        //strBuff[str.length()] = 0;
        auto wroteBytes=ss->WriteOneByte(strBuff);
        //DBGOUTHEX((strBuff, wroteBytes));
        pointers.push_back(strBuff);
        *(BYTE**)elem = strBuff;
    }
    else if (jsVal->IsArray())
    {
        auto jsArr = jsVal.As<Array>();
        auto arrBuff = new void*[jsArr->Length()];
        pointers.push_back(arrBuff);
        for (DWORD i = 0; i < jsArr->Length(); i++)
        {
            arrBuff[i] = 0;
            auto cvtRslt = ConvertElement(&arrBuff[i], jsArr->Get(i), pointers);
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
    HandleScope handleScope(isolate);

    if (args.Length() < 2)
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "At least 2 parameters"));
        return;
    }

    auto funcAddr = args[0]->Uint32Value();
    auto callType = (FunctionCallType)args[1]->Uint32Value();
    Registers regs;
    DWORD regFlags = 0;
    vector<DWORD> arguments;
    vector<void*> pointers;

    if (args.Length() >= 3)
    {
        if (!args[2]->IsObject())
        {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "arg3 must be a object"));
            return;
        }
        auto regsObj = args[2].As<Object>();
        for (int i = 0; i < 8; i++)
        {
            if (regsObj->Has(i))
            {
                regFlags |= (1 << i);
                *((DWORD*)&regs + (i + 1)) = regsObj->Get(i)->Uint32Value();
            }
        }
    }
    if (args.Length() >= 4)
    {
        if (!args[3]->IsArray())
        {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "arg4 must be an array"));
            return;
        }
        auto argsArray = args[3].As<Array>();
        for (DWORD i = 0; i < argsArray->Length(); i++)
        {
            DWORD arg = 0;
            auto cvtRslt = ConvertElement(&arg, argsArray->Get(i), pointers);
            if (!cvtRslt)
            {
                DBGOUT(("arg %d cvt failed.", i));
            }
            arguments.push_back(arg);
        }
    }
    DWORD callRetVal = 0;
    auto callRslt = CallFunction(funcAddr, callType, arguments, regFlags, &regs, &callRetVal);
    for (int i = pointers.size() - 1; i >= 0; i--)
    {
        delete[](char*)pointers[i];
    }
    if (!callRslt)
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "exception occured."));
    }
    else
    {
        args.GetReturnValue().Set((uint32_t)callRetVal);
    }
}

static void GetAPIAddress(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();

    HandleScope handleScope(isolate);

    CHECK_ARGS_COUNT(1);

    String::Value str(args[0]);
    auto pstr = (wchar_t *)*str;
    auto p = wcschr(pstr, L'.');
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

    args.GetReturnValue().Set(Integer::NewFromUnsigned(isolate, (uint32_t)addr));
}

static void Mread(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 2 || !args[0]->IsUint32() || !args[1]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto start = args[0]->Uint32Value();
    auto len = args[1]->Uint32Value();
    Local<String> str;
    __try
    {
        str = String::NewFromOneByte(args.GetIsolate(), (uint8_t*)start, String::kNormalString, len);
        args.GetReturnValue().Set(str);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Mem access violation!"));
    }
    return;
}

static void Mwrite(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    if (args.Length() != 4 || !args[0]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto addr = args[0]->Uint32Value();
    auto str = args[1]->ToString();
    auto start = args[2]->Uint32Value();
    auto len = args[3]->Uint32Value();
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
        THROW_EXCEPTION(L"Can't write memory");
    }
}

static void Disassmble(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);


    auto eip = args[0]->Uint32Value();
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
    obj->Set(String::NewFromOneByte(isolate, (uint8_t*)"length"), Integer::NewFromUnsigned(isolate, len));
    obj->Set(String::NewFromOneByte(isolate, (uint8_t*)"string"), String::NewFromOneByte(isolate, (uint8_t*)disasm.CompleteInstr));
    args.GetReturnValue().Set(obj);
}

static void CheckInfoHook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1 || !args[0]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto addr = args[0]->Uint32Value();
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

    auto funcId = args[0]->Uint32Value();
    auto argsCnt = args[1]->Uint32Value();
    auto funcType = (FunctionCallType)args[2]->Uint32Value();

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

static void DeleteMem(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto mem = (wchar_t*)args[0]->Uint32Value();
    delete[] mem;
}

static void Unhook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1 || !args[0]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto addr = args[0]->Uint32Value();
    RemoveHook((PVOID)addr);
}


// Reads a file into a v8 string.
v8::Handle<v8::String> ReadJSFile(v8::Isolate* isolate, const wchar_t* name) {
    FILE* file = _wfopen(name, L"rb");
    if (file == NULL) return v8::Handle<v8::String>();

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

    v8::Handle<v8::String> result;
    if (size >= 3 && !memcmp(chars, "\xef\xbb\xbf", 3))
    {
        result = v8::String::NewFromUtf8(isolate, chars + 3, v8::String::kNormalString, size - 3);
    }
    else if (size >= 2 && !memcmp(chars, "\xff\xfe", 2))
    {
        result = v8::String::NewFromTwoByte(isolate, (uint16_t*)(chars + 2), v8::String::kNormalString, size / 2 - 1);
    }
    else
    {
        result = v8::String::NewFromUtf8(isolate, chars, v8::String::kNormalString, size);
    }
    delete[] chars;
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
    if (args.Length() != 1) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }
    v8::String::Value file(args[0]);
    if (*file == NULL) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Error loading filename"));
        return;
    }
    v8::Handle<v8::String> source = ReadJSFile(args.GetIsolate(), (wchar_t*)*file);
    if (source.IsEmpty()) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file"));
        return;
    }
    args.GetReturnValue().Set(source);
}

static void WriteText(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(3);

    String::Value fname(args[0]);
    auto buff = args[1]->ToString();
    auto isTwoByte = args[2]->Uint32Value();
    
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
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        v8::String::Value file(args[i]);
        if (*file == NULL) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading filename"));
            return;
        }
        v8::Handle<v8::String> source = ReadJSFile(args.GetIsolate(), (wchar_t*)*file);
        if (source.IsEmpty()) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file"));
            return;
        }
        if (!ExecuteString(args.GetIsolate(),
            source,
            v8::String::NewFromTwoByte(args.GetIsolate(), *file),
            false,
            true)) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file"));
            return;
        }
    }
}


void CloneObject(Isolate* isolate, Handle<Value> source, Handle<Value> target)
{
    HandleScope scope(isolate);

    Handle<Value> args[] = { source, target };
    static Persistent<Function,CopyablePersistentTraits<Function>> g_cloneObjectMethod;

    // Init
    if (g_cloneObjectMethod.IsEmpty()) {
        Handle<Function> cloneObjectMethod_ = Handle<Function>::Cast(
            Script::Compile(String::NewFromUtf8(isolate,
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
        g_cloneObjectMethod = Persistent<Function, CopyablePersistentTraits<Function>>(isolate,cloneObjectMethod_);
    }

    auto recv_ = Object::New(isolate);
    auto clone = Local<Function>::New(isolate, g_cloneObjectMethod);
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

    auto val = args[0]->Uint32Value();
    args.GetReturnValue().Set(Number::New(isolate, (double)(*(float*)&val)));
}
static void ToFloatp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto val = args[0]->Uint32Value();
    args.GetReturnValue().Set(Number::New(isolate, (double)*(float*)val));
}
static void ToDoublep(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    CHECK_ARGS_COUNT(1);

    auto val = args[0]->Uint32Value();
    args.GetReturnValue().Set(Number::New(isolate, *(double*)val));
}

static void OutputStateGetter(Local<String> prop, const PropertyCallbackInfo<Value>& info)
{
    info.GetReturnValue().Set(Integer::New(info.GetIsolate(), (int)OutputWriter::GetOutputStream()));
}

static void OutputStateSetter(Local<String> prop, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
    auto st = value->Uint32Value();
    OutputWriter::ChangeOutputStream((OutputWriter::DispState)st);
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

        { "_CheckInfoHook", CheckInfoHook },
        { "_Unhook", Unhook },
        { "_GetAPIAddress", GetAPIAddress },
        { "_CallFunction", CallFunction },
        { "_NewCallback", NewCallback },
        { "_DeleteMem", DeleteMem },

        { "_Disassemble", Disassmble },

        { "_ToFloat", ToFloat},
        { "_ToFloatp", ToFloatp },
        { "_ToDoublep", ToDoublep },

    };

    for (int i = 0; i < sizeof(funcList) / sizeof(FuncList); i++)
    {
        global->Set(String::NewFromUtf8(isolate, funcList[i].funcName), FunctionTemplate::New(isolate, funcList[i].callBack));
    }

    global->Set(v8::String::NewFromUtf8(isolate, "_DllPath"), v8::String::NewFromTwoByte(isolate, (uint16_t*)g_dllPath.c_str()));
    global->Set(v8::String::NewFromUtf8(isolate, "_MyWindowThreadId"), Integer::NewFromUnsigned(isolate,g_myWindowThreadId));
    global->SetAccessor(String::NewFromUtf8(isolate, "_OutputState"), OutputStateGetter, OutputStateSetter);
    
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
        v8::String::Value filename(message->GetScriptResourceName());
        auto filename_string = ToWString(filename);
        int linenum = message->GetLineNumber();
        OutputWriter::OutputInfo(L"%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        v8::String::Value sourceline(message->GetSourceLine());
        auto sourceline_string = ToWString(sourceline);
        OutputWriter::OutputInfo(L"%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn();
        for (int i = 0; i < start; i++) {
            OutputWriter::OutputInfo(L" ");
        }
        int end = message->GetEndColumn();
        for (int i = start; i < end; i++) {
            OutputWriter::OutputInfo(L"^");
        }
        OutputWriter::OutputInfo(L"\n");
        v8::String::Value stack_trace(try_catch->StackTrace());
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
