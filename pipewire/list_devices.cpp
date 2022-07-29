#include "list_devices.h"

using std::string;

std::vector<device> devices{};

static void OnCoreDone(void *data, uint32_t id, int seq)
{
	auto d = static_cast<roundtrip_data *>(data);

	if (id == PW_ID_CORE && seq == d->pending)
		pw_main_loop_quit(d->loop);
}

static int Roundtrip(struct pw_core *core, struct pw_main_loop *loop)
{
	static const struct pw_core_events core_events = {
		.version = PW_VERSION_CORE_EVENTS,
		.done = OnCoreDone,
	};
	struct roundtrip_data d = { .loop = loop };
	struct spa_hook core_listener;

	pw_core_add_listener(core, &core_listener,
				 &core_events, &d);

	d.pending = pw_core_sync(core, PW_ID_CORE, 0);

	pw_main_loop_run(loop);

	spa_hook_remove(&core_listener);
	return 0;
}

void RegistryEventGlobal(void *data, uint32_t id,
		uint32_t permissions, const char *type, uint32_t version,
		const struct spa_dict *props)
{
	const char* temp;
	const string media_class = (temp = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS)) ? string{temp} : "";
	const string node_description = (temp = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)) ? string{temp} : "";
	const string node_name = (temp = spa_dict_lookup(props, PW_KEY_NODE_NAME)) ? string{temp} : "";


	deviceType device_type = *(static_cast<deviceType*>(data));

	switch(device_type) {
		case deviceType::audio_input:
		if (media_class == "Audio/Source" || media_class == "Audio/Source/Virtual" || media_class == "Audio/Duplex") {
				devices.push_back({node_description, node_name});
		}
		break;
		case deviceType::audio_output:
		if (media_class == "Audio/Sink" || media_class == "Audio/Duplex") {
				devices.push_back({node_description, node_name});
		}
		break;
		case deviceType::video_input:
		if (media_class == "Video/Source") {
				devices.push_back({node_description, node_name});
		}
		break;
	}
}

static const struct pw_registry_events registry_events_list = {
		.version = PW_VERSION_REGISTRY_EVENTS,
		.global = RegistryEventGlobal,
};

std::vector<device> ListDevices(deviceType device_type)
{
		struct pw_main_loop *loop;
		struct pw_context *context;
		struct pw_core *core;
		struct pw_registry *registry;
		struct spa_hook registry_listener;

		pw_init(NULL, NULL);

		devices.clear();

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
								&registry_events_list, &device_type);

		Roundtrip(core, loop);

		pw_proxy_destroy((struct pw_proxy*)registry);
		pw_core_disconnect(core);
		pw_context_destroy(context);
		pw_main_loop_destroy(loop);

		return devices;
}
