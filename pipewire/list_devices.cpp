#include <pipewire/pipewire.h>
#include <string>
#include <vector>
#include "list_devices.h"

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
		PW_VERSION_CORE_EVENTS,
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
	const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
	const char *node_description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
	const char *node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);


	int deviceType = *( (int*) data);

	switch(deviceType) {
	case AUDIO_INPUT:
		if (media_class != NULL && (strcmp(media_class, "Audio/Source") == 0 || strcmp(media_class, "Audio/Source/Virtual") == 0 || strcmp(media_class, "Audio/Duplex") == 0)) {
				devices.push_back({node_description, node_name});
		}
		break;
	case AUDIO_OUTPUT:
		if (media_class != NULL && (strcmp(media_class, "Audio/Sink") == 0 || strcmp(media_class, "Audio/Duplex") == 0)) {
				devices.push_back({node_description, node_name});
		}
		break;
	case VIDEO_INPUT:
		if (media_class != NULL && (strcmp(media_class, "Video/Source") == 0)) {
				devices.push_back({node_description, node_name});
		}
		break;
	}
}

std::vector<device> ListDevices(int deviceType)
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
								&registry_events_list, &deviceType);

		Roundtrip(core, loop);

		pw_proxy_destroy((struct pw_proxy*)registry);
		pw_core_disconnect(core);
		pw_context_destroy(context);
		pw_main_loop_destroy(loop);

		return devices;
}
