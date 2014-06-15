#include <v8.h>
#include <assert.h>
#include "dialog.h"
#include <memory>
#include "Memory.h"
#include "jsInterfaces.h"
#include "patcher.h"

using namespace v8;
using namespace std;

const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

const wchar_t* ToWString(const v8::String::Value& value)
{
    auto s = (wchar_t*)*value;
    return s ? s : L"<string conversion failed>";
}

static void GetMemoryBlocks(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    HandleScope handleScope(isolate);
    auto array = Array::New(isolate, 100);
    vector<MEMORY_BASIC_INFORMATION> blocks;
    if (!GetMemoryBlocks(blocks))
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
        obj->Set(String::NewFromUtf8(isolate, "allocationBase"), Integer::NewFromUnsigned(isolate, (DWORD)blocks[i].AllocationBase));

        array->Set(i, obj);
    }
    
    args.GetReturnValue().Set(array);
}

static void Mread(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length()!=2 || !args[0]->IsUint32() || !args[1]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto start=args[0]->Uint32Value();
    auto len=args[1]->Uint32Value();
    Local<String> str;
    __try
    {
        str=String::NewFromOneByte(args.GetIsolate(),(uint8_t*)start,String::kNormalString,len);
        args.GetReturnValue().Set(str);
    }
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH)
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Mem access violation!"));
    }
    return;
}

static void CheckInfoHook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length()!=1 || !args[0]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto addr=args[0]->Uint32Value();
    if(!CheckInfoHook((PVOID)addr))
    {
        args.GetReturnValue().Set(false);
    }
    else
    {
        args.GetReturnValue().Set(true);
    }
    return;
}

static void Unhook(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length()!=1 || !args[0]->IsUint32())
    {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters"));
        return;
    }

    auto addr=args[0]->Uint32Value();
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
    chars[size+1] = '\0';
    for (int i = 0; i < size;) {
        int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
        i += read;
    }
    fclose(file);

    v8::Handle<v8::String> result;
    if (size >= 3 && !memcmp(chars, "\xef\xbb\xbf", 3))
    {
        result=v8::String::NewFromUtf8(isolate, chars+3, v8::String::kNormalString, size-3);
    }
    else if (size >= 2 && !memcmp(chars, "\xff\xfe", 2))
    {
        result = v8::String::NewFromTwoByte(isolate, (uint16_t*)(chars + 2), v8::String::kNormalString, size/2 - 1);
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
    OutputWriter::OutputInfo(L"\n");
}
// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
static void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
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


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
static void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
            false)) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file"));
            return;
        }
    }
}

Handle<Context> InitV8()
{
    Isolate* isolate = Isolate::GetCurrent();


    Handle<ObjectTemplate> global = ObjectTemplate::New(isolate);

    global->Set(v8::String::NewFromUtf8(isolate, "print"), v8::FunctionTemplate::New(isolate, Print));
    global->Set(v8::String::NewFromUtf8(isolate, "load"), v8::FunctionTemplate::New(isolate, Load));
    global->Set(v8::String::NewFromUtf8(isolate, "read"), v8::FunctionTemplate::New(isolate, Read));

    global->Set(v8::String::NewFromUtf8(isolate, "_Mread"), v8::FunctionTemplate::New(isolate, Mread));
    global->Set(v8::String::NewFromUtf8(isolate, "_GetMemoryBlocks"),v8::FunctionTemplate::New(isolate, GetMemoryBlocks));
    global->Set(v8::String::NewFromUtf8(isolate, "_CheckInfoHook"), v8::FunctionTemplate::New(isolate, CheckInfoHook));
    global->Set(v8::String::NewFromUtf8(isolate, "_Unhook"), v8::FunctionTemplate::New(isolate, Unhook));

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
    v8::TryCatch try_catch;
    v8::ScriptOrigin origin(name);
    v8::Handle<v8::Script> script = v8::Script::Compile(source, &origin);
    if (script.IsEmpty()) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ReportException(isolate, &try_catch);
        return false;
    }
    else {
        v8::Handle<v8::Value> result = script->Run();
        if (result.IsEmpty()) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ReportException(isolate, &try_catch);
            return false;
        }
        else {
            assert(!try_catch.HasCaught());
            if (print_result && !result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                v8::String::Value str(result);
                auto cstr = ToWString(str);
                OutputWriter::OutputInfo(L"%s\n", cstr);
            }
            return true;
        }
    }
}
