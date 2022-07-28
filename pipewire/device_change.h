#pragma once

#include <pipewire/pipewire.h>

struct callback {
	Napi::Function handleDeviceChange;
	Napi::Env env;
};

struct device_with_id {
	std::string description;
	std::string name;
	uint32_t id;
};

static void registry_event_global(void *data, uint32_t id,
		uint32_t permissions, const char *type, uint32_t version,
		const struct spa_dict *props);

static void registry_event_global_remove(void *data, uint32_t id);

static const struct pw_registry_events registry_events_change = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove
};

int DeviceChange(Napi::Function handleDeviceChange, Napi::Env env);
