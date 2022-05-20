#include <napi.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

//We store all callbacks as global variables so that we can access them from every function
Napi::Function handleVoiceActivity;
Napi::Function handleDeviceChange;
Napi::Function allocatorCallback;
Napi::Function handleVolumeChange;

//We import the JSON.stringify function from the JavaScript engine
std::string json_stringify(Napi::Object input, Napi::Env env) {
  Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
  Napi::Function stringify = json.Get("stringify").As<Napi::Function>();
  Napi::Value json_object = stringify.Call(json, { input });
  std::string json_string = json_object.ToString().Utf8Value();
  return json_string;
}

int roundtrip(const char* ip) {
        struct tcp_info info;
        //We create a struct for the server info
        struct sockaddr_in sin;
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        //We fill the struct with the address family
        sin.sin_family = AF_INET;
        //the port
        sin.sin_port = htons(80);
        //and the ip address
        inet_aton(ip, &sin.sin_addr);

        //We connect to the server
        connect(sock, (struct sockaddr *) &sin, sizeof(sin));
        socklen_t tcp_info_length = sizeof info;
        //We get the round trip time in microseconds
        int rtt = getsockopt(sock, SOL_TCP, TCP_INFO, &info, &tcp_info_length);
        //We close the socket since it's no longer needed
        close(sock);
        return rtt;
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

  //This is how we retrieve arguments passed to the function
  Napi::Object options = info[0].As<Napi::Object>();
  return;
}

//This callback doesn't appear to be utilized
void SetOnVoiceCallback(const Napi::CallbackInfo& info) {
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

  Napi::Function voiceCallback = info[0].As<Napi::Function>();
  //We store the callback at a global variable to access it from another function later
  handleVoiceActivity = voiceCallback;
  return;
}

//This function is provided with a callback that we call each time there's a change
//to the audio input, video input and audio output devices we initially provided discord with
void SetDeviceChangeCallback(const Napi::CallbackInfo& info) {
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

  Napi::Function deviceCallback = info[0].As<Napi::Function>();
  handleDeviceChange = deviceCallback;
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

//Stubbed in the blob but Discord won't boot if we don't advertise it
void SetVolumeChangeCallback(const Napi::CallbackInfo& info) {
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

  Napi::Function callback = info[0].As<Napi::Function>();
  handleVolumeChange = callback;
  return;
}

//This handles which objects are exported to node
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  //Construct the Degradation Preference Object
  //This is basically our preference as to how bad connections are handled
  //Do we care more about the resolution, the framerate, both or nothing?
  Napi::Object DegradationPreference = Napi::Object::New(env);
  DegradationPreference.Set("MAINTAIN_RESOLUTION", 0);
  DegradationPreference.Set("MAINTAIN_FRAMERATE", 1);
  DegradationPreference.Set("BALANCED", 2);
  DegradationPreference.Set("DISABLED", 3);

  exports.Set("DegradationPreference", DegradationPreference);
  exports.Set("initialize", Napi::Function::New(env, Initialize));
  exports.Set("setOnVoiceCallback", Napi::Function::New(env, SetOnVoiceCallback));
  exports.Set("setDeviceChangeCallback", Napi::Function::New(env, SetDeviceChangeCallback));
  exports.Set("setImageDataAllocator", Napi::Function::New(env, SetImageDataAllocator));
  exports.Set("setVolumeChangeCallback", Napi::Function::New(env, SetVolumeChangeCallback));
  return exports;
}

NODE_API_MODULE(addon, Init)
