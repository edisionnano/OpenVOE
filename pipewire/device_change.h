#pragma once

#include <string>
#include <vector>

#include <pipewire/pipewire.h>

struct device_with_id {
	std::string description;
	std::string name;
	uint32_t id;
};

struct callback_executor {
	void (*executor)(void*, std::vector<device_with_id>&, std::vector<device_with_id>&, std::vector<device_with_id>&);
	void* callback;
};

int DeviceChange(callback_executor ce);
