#include "device_change.h"

std::list<device_with_id> audioInputDevices{};
std::list<device_with_id> audioOutputDevices{};
std::list<device_with_id> videoInputDevices{};
std::mutex devices_mutex{};

struct pw_main_loop* loop;

std::atomic<callback_executor*> current_executor{nullptr};

static void on_process(void *userdata)
{
        struct video_data *data = (video_data*)userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        uint8_t *map;
        uint8_t *sdata;

        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }

        buf = b->buffer;

        printf("got a frame of size %d and height %d\n", buf->datas[0].chunk->size, data->format.info.mjpg.size.height);

        if (buf->datas[0].type == SPA_DATA_MemFd ||
            buf->datas[0].type == SPA_DATA_DmaBuf) {
                map = (uint8_t*)mmap(NULL, buf->datas[0].maxsize + buf->datas[0].mapoffset, PROT_READ,
                           MAP_PRIVATE, buf->datas[0].fd, 0);
                sdata = (uint8_t*)SPA_PTROFF(map, buf->datas[0].mapoffset, uint8_t);
        } else if (buf->datas[0].type == SPA_DATA_MemPtr) {
                map = NULL;
                sdata = (uint8_t*)buf->datas[0].data;
	}

	uint8_t* dst_y = new uint8_t;
        uint8_t* dst_u = new uint8_t;
        uint8_t* dst_v = new uint8_t;

	int ret = libyuv::ConvertToI420(sdata, buf->datas[0].chunk->size,
					dst_y, data->format.info.mjpg.size.width, 
					dst_u, (data->format.info.mjpg.size.width +1) / 2,
					dst_v, (data->format.info.mjpg.size.width +1) / 2,
					0, 0, data->format.info.mjpg.size.width, data->format.info.mjpg.size.height,
					data->format.info.mjpg.size.width, data->format.info.mjpg.size.height, libyuv::kRotate0, libyuv::FOURCC_MJPG);

	delete(dst_y);
	delete(dst_u);
	delete(dst_v);

        pw_stream_queue_buffer(data->stream, b);
}

static void on_param_changed(void *userdata, uint32_t id, const struct spa_pod *param)
{
        struct video_data *data = (video_data*)userdata;

        if (param == NULL || id != SPA_PARAM_Format)
                return;

        if (spa_format_parse(param,
                        &data->format.media_type,
                        &data->format.media_subtype) < 0)
                return;

        if (data->format.media_type != SPA_MEDIA_TYPE_video ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_mjpg)
                return;

        if (spa_format_video_mjpg_parse(param, &data->format.info.mjpg) < 0)
                return;
}

static const struct pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .param_changed = on_param_changed,
        .process = on_process};

static void registry_event_global(void* data,
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

int CaptureFrames()
{
        struct video_data data = { 0, };
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        struct pw_properties *props;

        data.loop = loop;

        props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                        PW_KEY_MEDIA_CATEGORY, "Capture",
                        PW_KEY_MEDIA_ROLE, "Camera",
                        NULL);

        data.stream = pw_stream_new_simple(
                        pw_main_loop_get_loop(data.loop),
                        "video-capture",
                        props,
                        &stream_events,
                        &data);

        params[0] = (spa_pod*)spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
                SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
                SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_mjpg));

        pw_stream_connect(data.stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          static_cast<pw_stream_flags>(PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_AUTOCONNECT),
                          params, 1);

        pw_stream_destroy(data.stream);

        return 0;
}

int DeviceChange() {
  struct pw_context* context;
  struct pw_core* core;
  struct pw_registry* registry;
  struct spa_hook registry_listener;

  pw_init(NULL, NULL);

  loop = pw_main_loop_new(NULL);
  context = pw_context_new(pw_main_loop_get_loop(loop),
                           NULL, 0);

  core = pw_context_connect(
      context, NULL, 0);

  registry =
      pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);

  spa_zero(registry_listener);
  pw_registry_add_listener(
      registry, &registry_listener, &registry_events_change, nullptr);

  pw_main_loop_run(loop);

  pw_proxy_destroy((struct pw_proxy*)registry);
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_main_loop_destroy(loop);
  pw_deinit();

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
