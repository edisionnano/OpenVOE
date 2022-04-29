#include <napi.h>
#include <iostream>

void Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

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

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "initialize"), Napi::Function::New(env, Initialize));
  return exports;
}

NODE_API_MODULE(addon, Init)
