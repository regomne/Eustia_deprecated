#include <v8.h>

using namespace v8;

Handle<Context> InitV8()
{
    Isolate* isolate = Isolate::GetCurrent();


    Handle<ObjectTemplate> global = ObjectTemplate::New(isolate);

    //global->Set(v8::String::NewFromUtf8(isolate, "fread"),
    //    v8::FunctionTemplate::New(isolate, myFread));

    return Context::New(isolate, NULL, global);

}