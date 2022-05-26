#include <napi.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <poll.h>
#include "registry/registry.hpp"

//We store all callbacks as global variables
//so that we can access them from every function
Napi::Function handleVoiceActivity;
Napi::Function handleDeviceChange;
Napi::Function allocatorCallback;
Napi::Function handleVolumeChange;

bool aecDump{false};

//JSON.stringify imported from the JavaScript engine
//Will accept an Object and convert it to a string
std::string JsonStringify(Napi::Object input, Napi::Env env) {
	Napi::Object json{env.Global().Get("JSON").As<Napi::Object>()};
	Napi::Function stringify{json.Get("stringify").As<Napi::Function>()};
	Napi::Value jsonObject{stringify.Call(json, { input })};
	std::string jsonString{jsonObject.ToString().Utf8Value()};
	return jsonString;
}

//Called by index.js in order to start the main loop
//which polls for devices etc., also gets provided with some options
void Initialize(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsObject()) {
		Napi::TypeError::New(env, "Wrong argument type, Object expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Object options{info[0].As<Napi::Object>()};

	std::cout << JsonStringify(options, env) << std::endl;
}

//This callback doesn't appear to be utilized
void SetOnVoiceCallback(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function voiceCallback{info[0].As<Napi::Function>()};

	handleVoiceActivity = voiceCallback;
}

//This function is provided with a callback
//that we call each time there's a change
//to the audio input, video input and audio output devices
//that we initially provided discord with
void SetDeviceChangeCallback(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function deviceCallback{info[0].As<Napi::Function>()};

	handleDeviceChange = deviceCallback;
}

void GetInputDevices(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function deviceCallback{info[0].As<Napi::Function>()};

	Napi::Array audioInputDevicesArray{Napi::Array::New(env)};

	auto mainLoop{pipewire::main_loop()};
	auto context{pipewire::context(mainLoop)};
	auto core{pipewire::core(context)};
	auto reg{pipewire::registry(core)};

	int audioInputDeviceCount;

	auto regListener{reg.listen<pipewire::registry_listener>()};
	regListener.on<pipewire::registry_event::global>([&](const pipewire::global &global) {
		auto props{global.props};

		if (props["media.class"] == "Audio/Source" || props["media.class"] == "Audio/Source/Virtual") {
			std::string audioInputDeviceName{props["node.description"]};

			if(audioInputDeviceName == "") {
				audioInputDeviceName = props["node.name"];
			}

			Napi::Object audioInputDevice{Napi::Object::New(env)};
        		audioInputDevice.Set("name", audioInputDeviceName);
        		audioInputDevice.Set("guid", "");
        		audioInputDevice.Set("index", audioInputDeviceCount);
			audioInputDevicesArray[audioInputDeviceCount] = audioInputDevice;
			audioInputDeviceCount++;
		}
	});

	core.sync();

	deviceCallback.Call(env.Global(), {audioInputDevicesArray});
}

//Called by index.js, its purpose is to store
//the allocator callback that will be used at a later phase
void SetImageDataAllocator(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function allocator{info[0].As<Napi::Function>()};

	allocatorCallback = allocator;
}

//Stubbed in the blob but Discord won't boot if we don't expose it
void SetVolumeChangeCallback(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function callback{info[0].As<Napi::Function>()};

	handleVolumeChange = callback;
}

//This is an optional function
//If we export it's called with the ouput of Chromium's Console
void ConsoleLog(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 2) {
		Napi::TypeError::New(env, "Wrong number of arguments, 2 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsString() || !info[1].IsString()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::String level{info[0].As<Napi::String>()};

	Napi::String json{info[1].As<Napi::String>()};

	std::cout << level.Utf8Value() << ": " << json.Utf8Value() <<std::endl;
	return;
}

//This is an optional function; it's only called if we export it
//It changes depending on the position of the Diagnostic
//Audio Recording toggle in Discord's Voice & Video settings
//While we don't plan to support this functionality,
//one extra toggle is always useful for configuration
void SetAecDump(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 1) {
		Napi::TypeError::New(env, "Wrong number of arguments, 1 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsBoolean()) {
		Napi::TypeError::New(env, "Wrong argument type, Boolean expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Boolean enable{info[0].As<Napi::Boolean>()};

	aecDump = enable.Value();
}

//Called sometimes in order to rank the WebRTC regions by latency
//The way we handle this is by connecting once to each server
//then measuring the roundtrip time of connect and averaging
//them out to find the average rtt of each region
//All IPs are pinged at once
//Credits to @jsimmons for the multithreaded socket connect logic
void RankRtcRegions(const Napi::CallbackInfo& info) {
	Napi::Env env{info.Env()};

	if (info.Length() != 2) {
		Napi::TypeError::New(env, "Wrong number of arguments, 2 expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[0].IsObject()) {
		Napi::TypeError::New(env, "Wrong argument type, Object expected").ThrowAsJavaScriptException();
		return;
	}

	if (!info[1].IsFunction()) {
		Napi::TypeError::New(env, "Wrong argument type, Callback expected").ThrowAsJavaScriptException();
		return;
	}

	Napi::Object regionsIps{info[0].As<Napi::Object>()};

	Napi::Function rankRtcRegionsCallback{info[1].As<Napi::Function>()};

	struct endpoint {
		std::string ip;
		int fd{0};
		int rtt{-1};
	};

	struct region {
		std::vector<endpoint> endpoints;
		std::string name;
		int rttTime;

		bool operator<(region& other) {return rttTime - other.rttTime > 0;}
	};

	std::vector<region> regions(regionsIps.GetPropertyNames().Length());

	Napi::Array rankedRegions{Napi::Array::New(env)};

	for (int i{0}; i < regions.size(); i++) {
		auto regionIps{regionsIps.Get(i).ToObject().Get("ips").ToObject()};
		regions[i].name = regionsIps.Get(i).ToObject().Get("region").ToString().Utf8Value();
		regions[i].endpoints.resize(regionIps.GetPropertyNames().Length());
		for (int j{0}; j < regions[i].endpoints.size(); j++) {
			regions[i].endpoints[j].ip = regionIps.Get(j).ToString().Utf8Value();
			regions[i].endpoints[j].fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (regions[i].endpoints[j].fd == -1) {
				perror("socket create");
				return;
			}

			sockaddr_in sin;
			sin.sin_family = AF_INET;
			sin.sin_port = htons(80);
			if (inet_aton(regions[i].endpoints[j].ip.c_str(), &sin.sin_addr) == 0) {
				perror("inet aton");
				return;
			}

			//If we succeeded first try without waiting then calculate the rtt
			if (connect(regions[i].endpoints[j].fd, reinterpret_cast<sockaddr *>(&sin), sizeof(sin) ) == 0) {
				tcp_info info;
				socklen_t tcp_info_length{sizeof info};
				getsockopt(regions[i].endpoints[j].fd, IPPROTO_TCP, TCP_INFO, &info, &tcp_info_length);
				close(regions[i].endpoints[j].fd);
				regions[i].endpoints[j].rtt = info.tcpi_rtt;
				regions[i].endpoints[j].fd = -1;
			}

			if (errno != EINPROGRESS) {
				perror("connect");
				regions[i].endpoints[j].fd = -1;
			}

		}
	}

	for (const auto current_region : regions) {
		for(auto current_endpoint : current_region.endpoints) {
			if (current_endpoint.fd == -1) {
				continue;
			}

			//Wait for the socket connect to complete
			{
				pollfd pollfd;
				pollfd.fd = current_endpoint.fd;
				pollfd.events = POLLOUT;
				if (poll(&pollfd, 1, -1) < 0) {
					perror("poll");
					current_endpoint.fd = -1;
					continue;
				}
			}

			//Check whether we succeeded in connecting, or errored out.
			int ret;
			socklen_t ret_len{sizeof(ret)};
			if (getsockopt(current_endpoint.fd, SOL_SOCKET, SO_ERROR, &ret, &ret_len) < 0) {
				perror("so_error");
				current_endpoint.fd = -1;
				continue;
			}

			//If we succeeded then calculate the rtt
			if (ret == 0) {
				tcp_info info;
				socklen_t tcp_info_length{sizeof info};
				getsockopt(current_endpoint.fd, IPPROTO_TCP, TCP_INFO, &info, &tcp_info_length);
				close(current_endpoint.fd);
				current_endpoint.rtt = info.tcpi_rtt;
				current_endpoint.fd = -1;
				continue;
			}

			if (ret != EINPROGRESS) {
				fprintf(stderr, "socket error: %s\n", strerror(ret));
				current_endpoint.fd = -1;
				continue;
			}
		}
	}

	for (auto current_region : regions) {
		int avgRtt{0};

		for (auto current_endpoint : current_region.endpoints) {
			avgRtt += current_endpoint.rtt;
		}

		current_region.rttTime = avgRtt / current_region.endpoints.size();
	}


	std::sort(regions.begin(), regions.end());

	for (int i{0}; i < regions.size(); i++) {
		rankedRegions[i] = regions[i].name.c_str();
	}

	rankRtcRegionsCallback.Call(env.Global(), {rankedRegions});
}

//This handles the objects that are exported to node
Napi::Object Init(Napi::Env env, Napi::Object exports) {
	//Construct the Degradation Preference Object
	//This is basically our preference as to how bad connections are handled
	//Do we care more about the resolution, the framerate, both or nothing?
	Napi::Object DegradationPreference{Napi::Object::New(env)};
	DegradationPreference.Set("MAINTAIN_RESOLUTION", 0);
	DegradationPreference.Set("MAINTAIN_FRAMERATE", 1);
	DegradationPreference.Set("BALANCED", 2);
	DegradationPreference.Set("DISABLED", 3);

	exports.Set("DegradationPreference", DegradationPreference);
	exports.Set("initialize", Napi::Function::New(env, Initialize));
	exports.Set("setOnVoiceCallback", Napi::Function::New(env, SetOnVoiceCallback));
	exports.Set("setDeviceChangeCallback", Napi::Function::New(env, SetDeviceChangeCallback));
	exports.Set("getInputDevices", Napi::Function::New(env, GetInputDevices));
	exports.Set("setImageDataAllocator", Napi::Function::New(env, SetImageDataAllocator));
	exports.Set("setVolumeChangeCallback", Napi::Function::New(env, SetVolumeChangeCallback));
	exports.Set("consoleLog", Napi::Function::New(env, ConsoleLog));
	exports.Set("setAecDump", Napi::Function::New(env, SetAecDump));
	exports.Set("rankRtcRegions", Napi::Function::New(env, RankRtcRegions));
	return exports;
}

NODE_API_MODULE(addon, Init)
