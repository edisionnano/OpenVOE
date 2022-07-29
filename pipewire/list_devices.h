#pragma once

#include <string>
#include <vector>

#include <pipewire/pipewire.h>

enum deviceType {
	audio_input,
	audio_output,
	video_input
};

struct device {
	std::string description;
	std::string name;
};

struct roundtrip_data {
	int pending;
	struct pw_main_loop *loop;
};

std::vector<device> ListDevices(deviceType device_type);
