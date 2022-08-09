#pragma once

#include <list>
#include <atomic>
#include <mutex>
#include <spa/param/video/format-utils.h>
#include <libyuv.h>
#include <sys/mman.h>
#include <pipewire/pipewire.h>

struct callback_executor {
  void (*executor)(void*);
  void* callback;
};

struct device_with_id {
  std::string description;
  std::string name;
  uint32_t id;
};

struct video_data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;
        struct spa_video_info format;
};

int CaptureFrames();

int DeviceChange();

void SetExecutor(callback_executor* new_executor);

callback_executor* GetExecutor();

std::list<device_with_id> GetAudioInputDevices();

std::list<device_with_id> GetAudioOutputDevices();

std::list<device_with_id> GetVideoDevices();
