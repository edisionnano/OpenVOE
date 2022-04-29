#include <napi.h>
#include <iostream>

void Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return;
  }

  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return;
  }

  double arg = info[0].As<Napi::Number>().DoubleValue();
  std::cout << arg;
  return;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "initialize"), Napi::Function::New(env, Initialize));
  return exports;
}

NODE_API_MODULE(addon, Init)
