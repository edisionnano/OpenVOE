#include <napi.h>
#include <iostream>

Napi::Function allocatorCallback;

std::string json_stringify(Napi::Object input, Napi::Env env) {
  Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
  Napi::Function stringify = json.Get("stringify").As<Napi::Function>();
  Napi::Value json_object = stringify.Call(json, { input });
  std::string json_string = json_object.ToString().Utf8Value();
  return json_string;
}

//Called by index.js in order to start the main loop which polls for devices etc., also gets provided with some options
void Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  //We have to handle input data validation within the function
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments, 1 expected")
        .ThrowAsJavaScriptException();
    return;
  }

  if (!info[0].IsObject()) {
    Napi::TypeError::New(env, "Wrong argument type, Object expected").ThrowAsJavaScriptException();
    return;
  }

  Napi::Object options = info[0].As<Napi::Object>();
  return;
}

//Called by index.js, its purpose is to store the allocator callback that will be used at a later phase
void SetImageDataAllocator(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments, 1 expected")
        .ThrowAsJavaScriptException();
    return;
  }

  if (!info[0].IsFunction()) {
    Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
    return;
  }

  Napi::Function allocator = info[0].As<Napi::Function>();
  allocatorCallback = allocator;
  return;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "initialize"), Napi::Function::New(env, Initialize));
  exports.Set(Napi::String::New(env, "setImageDataAllocator"), Napi::Function::New(env, SetImageDataAllocator));
  return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "initialize"), Napi::Function::New(env, Initialize));
  exports.Set(Napi::String::New(env, "setImageDataAllocator"), Napi::Function::New(env, SetImageDataAllocator));
  return exports;
}

NODE_API_MODULE(addon, Init)
