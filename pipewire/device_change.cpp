#include <napi.h>
#include <pipewire/pipewire.h>
#include <string>
#include <vector>
#include "device_change.h"

std::vector<device_with_id> audioInputDevices{};
std::vector<device_with_id> audioOutputDevices{};
std::vector<device_with_id> videoInputDevices{};

void ExecuteCallback(callback c)
{
	Napi::Function handleDeviceChange = c.handleDeviceChange;
	Napi::Env env = c.env;

	Napi::Array audioInputDevicesArray{Napi::Array::New(env)};
	Napi::Array audioOutputDevicesArray{Napi::Array::New(env)};
	Napi::Array videoInputDevicesArray{Napi::Array::New(env)};

	for(int i{}; i < audioInputDevices.size(); i++) {
		Napi::Object audioInputDevice{Napi::Object::New(env)};
		audioInputDevice.Set("name", audioInputDevices[i].description);
		audioInputDevice.Set("guid", "");
		audioInputDevice.Set("index", i);
		audioInputDevicesArray[i] = audioInputDevice;
	}

	for(int i{}; i < audioOutputDevices.size(); i++) {
		Napi::Object audioOutputDevice{Napi::Object::New(env)};
		audioOutputDevice.Set("name", audioOutputDevices[i].description);
		audioOutputDevice.Set("guid", "");
		audioOutputDevice.Set("index", i);
		audioOutputDevicesArray[i] = audioOutputDevice;
	}

	for(int i{}; i < videoInputDevices.size(); i++) {
		Napi::Object videoInputDevice{Napi::Object::New(env)};
		videoInputDevice.Set("name", videoInputDevices[i].description);
		videoInputDevice.Set("guid", videoInputDevices[i].name);
		videoInputDevice.Set("index", i);
		videoInputDevice.Set("facing", "unknown");
		videoInputDevicesArray[i] = videoInputDevice;
	}

	handleDeviceChange.Call(env.Global(), {audioInputDevicesArray, audioOutputDevicesArray, videoInputDevicesArray});
}

static void registry_event_global(void *data, uint32_t id,
	uint32_t permissions, const char *type, uint32_t version,
					const struct spa_dict *props)
{
	callback c = *( (callback*) data);

	const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
	const char *node_description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
	const char *node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);

	if (media_class != NULL && (strcmp(media_class, "Audio/Source") == 0 || strcmp(media_class, "Audio/Source/Virtual") == 0 || strcmp(media_class, "Audio/Duplex") == 0)) {
		audioInputDevices.push_back({node_description, node_name, id});
		ExecuteCallback(c);
	} else if (media_class != NULL && (strcmp(media_class, "Audio/Sink") == 0 || strcmp(media_class, "Audio/Duplex") == 0)) {
		audioOutputDevices.push_back({node_description, node_name, id});
		ExecuteCallback(c);
	} else if (media_class != NULL && (strcmp(media_class, "Video/Source") == 0)) {
		videoInputDevices.push_back({node_description, node_name, id});
		ExecuteCallback(c);
	}
}

static void registry_event_global_remove(void *data, uint32_t id)
{
	callback c = *( (callback*) data);

	for(std::vector<device_with_id>::size_type i{}; i < audioInputDevices.size(); i++) {
		if (id == audioInputDevices[i].id) {
			audioInputDevices.erase(audioInputDevices.begin()+i);
			ExecuteCallback(c);
			break;
		}
	}
	for(std::vector<device_with_id>::size_type i{}; i < audioOutputDevices.size(); i++) {
			if (id == audioOutputDevices[i].id) {
			audioOutputDevices.erase(audioOutputDevices.begin()+i);
			ExecuteCallback(c);
			break;
		}
	}
	for(std::vector<device_with_id>::size_type i{}; i < videoInputDevices.size(); i++) {
			if (id == videoInputDevices[i].id) {
			videoInputDevices.erase(videoInputDevices.begin()+i);
			ExecuteCallback(c);
			break;
		}
	}
}

int DeviceChange(Napi::Function handleDeviceChange, Napi::Env env)
{
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_core *core;
	struct pw_registry *registry;
	struct spa_hook registry_listener;

	callback c{handleDeviceChange, env};

	pw_init(NULL, NULL);

	loop = pw_main_loop_new(NULL /* properties */);
	context = pw_context_new(pw_main_loop_get_loop(loop),
					NULL /* properties */,
					0 /* user_data size */);

	core = pw_context_connect(context,
		NULL /* properties */,
		0 /* user_data size */);

	registry = pw_core_get_registry(core, PW_VERSION_REGISTRY,
					0 /* user_data size */);

	spa_zero(registry_listener);
	pw_registry_add_listener(registry, &registry_listener,
				&registry_events_change, &c);

	pw_main_loop_run(loop);

	pw_proxy_destroy((struct pw_proxy*)registry);
	pw_core_disconnect(core);
	pw_context_destroy(context);
	pw_main_loop_destroy(loop);

	return 0;
}
