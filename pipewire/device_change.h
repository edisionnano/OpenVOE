#pragma once

#include <list>
#include <string>
#include <vector>

#include <pipewire/pipewire.h>

struct device_with_id {
  std::string description;
  std::string name;
  uint32_t id;
};

struct callback_executor {
  void (*executor)(void*);
  void* callback;
};

int DeviceChange();

void SetExecutor(callback_executor* new_executor);

callback_executor* GetExecutor();

std::list<device_with_id> GetAudioInputDevices();

std::list<device_with_id> GetAudioOutputDevices();

std::list<device_with_id> GetVideoDevices();
