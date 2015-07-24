#pragma once
#include <include/v8.h>
#include <memory.h>
#include <stdlib.h>

class MyArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
    virtual void* Allocate(size_t length) {
        void* data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }
    virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void* data, size_t) { free(data); }
};

v8::Handle<v8::Context> InitV8();
bool ExecuteString(v8::Isolate* isolate,
    v8::Handle<v8::String> source,
    v8::Handle<v8::Value> name,
    bool print_result,
    bool report_exceptions);
v8::Handle<v8::Value> ExecuteStringWithRet(v8::Isolate* isolate,
    v8::Handle<v8::String> source,
    v8::Handle<v8::Value> name,
    bool report_exceptions);
v8::Handle<v8::String> ReadJSFile(v8::Isolate* isolate, const wchar_t* name);