#pragma once
#include <v8.h>

v8::Handle<v8::Context> InitV8();
bool ExecuteString(v8::Isolate* isolate,
    v8::Handle<v8::String> source,
    v8::Handle<v8::Value> name,
    bool print_result,
    bool report_exceptions);