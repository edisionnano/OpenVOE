#include "device_change.h"

#include <atomic>
#include <mutex>

std::list<device_with_id> audioInputDevices{};
std::list<device_with_id> audioOutputDevices{};
std::list<device_with_id> videoInputDevices{};
std::mutex devices_mutex{};

std::atomic<callback_executor*> current_executor{nullptr};

static void registry_event_global(void*,
                                  uint32_t id,
                                  uint32_t permissions,
                                  const char* type,
                                  uint32_t version,
                                  const struct spa_dict* props) {
  callback_executor* ce = current_executor.load(std::memory_order_acquire);
  const char* temp;
  const std::string media_class =
      (temp = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS)) ? std::string{temp}
                                                          : "";
  const std::string node_description =
      (temp = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION))
          ? std::string{temp}
          : "";
  const std::string node_name =
      (temp = spa_dict_lookup(props, PW_KEY_NODE_NAME)) ? std::string{temp}
                                                        : "";

  bool deviceAdded{};
  {
    std::scoped_lock lock{devices_mutex};
    if (media_class == "Audio/Source" ||
        media_class == "Audio/Source/Virtual" ||
        media_class == "Audio/Duplex") {
      audioInputDevices.push_back({node_description, node_name, id});
      deviceAdded = true;
    }
    if (media_class == "Audio/Sink" || media_class == "Audio/Duplex") {
      audioOutputDevices.push_back({node_description, node_name, id});
      deviceAdded = true;
    }
    if (media_class == "Video/Source") {
      videoInputDevices.push_back({node_description, node_name, id});
      deviceAdded = true;
    }
  }

  if (deviceAdded && ce) {
    (*(ce->executor))(ce->callback);
  }
}

static void registry_event_global_remove(void* data, uint32_t id) {
  callback_executor* ce = current_executor.load(std::memory_order_acquire);
  bool deviceRemoved{};
  {
    std::scoped_lock lock{devices_mutex};
    auto pred = [id, &deviceRemoved](device_with_id device) {
      if (device.id == id) {
        deviceRemoved = true;
        return true;
      } else {
        return false;
      }
    };

    std::erase_if(audioInputDevices, pred);
    std::erase_if(audioOutputDevices, pred);
    std::erase_if(videoInputDevices, pred);
  }
  if (deviceRemoved && ce) {
    (*(ce->executor))(ce->callback);
  }
}

static const struct pw_registry_events registry_events_change = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = registry_event_global_remove};

int DeviceChange() {
  struct pw_main_loop* loop;
  struct pw_context* context;
  struct pw_core* core;
  struct pw_registry* registry;
  struct spa_hook registry_listener;

  pw_init(NULL, NULL);

  loop = pw_main_loop_new(NULL /* properties */);
  context = pw_context_new(pw_main_loop_get_loop(loop),
                           NULL /* properties */,
                           0 /* user_data size */);

  core = pw_context_connect(
      context, NULL /* properties */, 0 /* user_data size */);

  registry =
      pw_core_get_registry(core, PW_VERSION_REGISTRY, 0 /* user_data size */);

  spa_zero(registry_listener);
  pw_registry_add_listener(
      registry, &registry_listener, &registry_events_change, nullptr);

  pw_main_loop_run(loop);

  pw_proxy_destroy((struct pw_proxy*)registry);
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_main_loop_destroy(loop);

  return 0;
}

void SetExecutor(callback_executor* new_executor) {
  current_executor.store(new_executor, std::memory_order_release);
}

callback_executor* GetExecutor() {
  return current_executor.load(std::memory_order_acquire);
}

std::list<device_with_id> GetAudioInputDevices() {
  std::scoped_lock lock{devices_mutex};
  return std::list<device_with_id>{audioInputDevices};
}

std::list<device_with_id> GetAudioOutputDevices() {
  std::scoped_lock lock{devices_mutex};
  return std::list<device_with_id>{audioOutputDevices};
}

std::list<device_with_id> GetVideoDevices() {
  std::scoped_lock lock{devices_mutex};
  return std::list<device_with_id>{videoInputDevices};
}
