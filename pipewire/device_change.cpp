#include "device_change.h"

using std::string;

std::vector<device_with_id> audioInputDevices{};
std::vector<device_with_id> audioOutputDevices{};
std::vector<device_with_id> videoInputDevices{};

static void registry_event_global(void *data, uint32_t id,
	uint32_t permissions, const char *type, uint32_t version,
					const struct spa_dict *props)
{
	callback_executor ce = *static_cast<callback_executor*>(data);

	const char* temp;
	const string media_class = (temp = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS)) ? string{temp} : "";
	const string node_description = (temp = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)) ? string{temp} : "";
	const string node_name = (temp = spa_dict_lookup(props, PW_KEY_NODE_NAME)) ? string{temp} : "";

	if (media_class == "Audio/Source" || media_class == "Audio/Source/Virtual" || media_class == "Audio/Duplex") {
		audioInputDevices.push_back({node_description, node_name, id});
		(*(ce.executor))(ce.callback, audioInputDevices, audioOutputDevices, videoInputDevices);
	}
	if (media_class == "Audio/Sink" || media_class == "Audio/Duplex") {
		audioOutputDevices.push_back({node_description, node_name, id});
		(*(ce.executor))(ce.callback, audioInputDevices, audioOutputDevices, videoInputDevices);
	}
	if (media_class == "Video/Source") {
		videoInputDevices.push_back({node_description, node_name, id});
		(*(ce.executor))(ce.callback, audioInputDevices, audioOutputDevices, videoInputDevices);
	}
}

static void registry_event_global_remove(void *data, uint32_t id)
{
	callback_executor cb = *static_cast<callback_executor*>(data);
	auto pred = [id] (device_with_id device) {return device.id == id;};

	bool audioInputDevicesRemoved = std::erase_if(audioInputDevices, pred) > 0;
	bool audioOutputDevicesRemoved = std::erase_if(audioOutputDevices, pred) > 0;
	bool videoInputDevicesRemoved = std::erase_if(videoInputDevices, pred) > 0;

	if(audioInputDevicesRemoved || audioOutputDevicesRemoved || videoInputDevicesRemoved) {
		(*(cb.executor))(cb.callback, audioInputDevices, audioOutputDevices, videoInputDevices);
	}
}

static const struct pw_registry_events registry_events_change = {
	.version = PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove
};

int DeviceChange(callback_executor ce)
{
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_core *core;
	struct pw_registry *registry;
	struct spa_hook registry_listener;


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
				&registry_events_change, &ce);

	pw_main_loop_run(loop);

	pw_proxy_destroy((struct pw_proxy*)registry);
	pw_core_disconnect(core);
	pw_context_destroy(context);
	pw_main_loop_destroy(loop);

	return 0;
}
