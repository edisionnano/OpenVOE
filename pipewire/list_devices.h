#pragma once

#include <pipewire/pipewire.h>

#define AUDIO_INPUT  0x00001
#define AUDIO_OUTPUT 0x00002
#define VIDEO_INPUT  0x00003

struct device {
	std::string description;
	std::string name;
};

struct roundtrip_data {
	int pending;
	struct pw_main_loop *loop;
};

void RegistryEventGlobal(void *data, uint32_t id,
				uint32_t permissions, const char *type, uint32_t version,
				const struct spa_dict *props);

static const struct pw_registry_events registry_events = {
		PW_VERSION_REGISTRY_EVENTS,
		.global = RegistryEventGlobal,
};

std::vector<device> ListDevices(int deviceType);
