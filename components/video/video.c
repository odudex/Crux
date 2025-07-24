# include "video.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "video";

#define MAX_BUFFER_COUNT                (6)
#define MIN_BUFFER_COUNT                (2)
#define VIDEO_TASK_STACK_SIZE           (4 * 1024)
#define VIDEO_TASK_PRIORITY             (3)

typedef struct {
    uint8_t *camera_buffer[MAX_BUFFER_COUNT];
    size_t camera_buf_size;
    uint32_t camera_buf_hes;
    uint32_t camera_buf_ves;
    struct v4l2_buffer v4l2_buf;
    uint8_t camera_mem_mode;
    app_video_frame_operation_cb_t user_camera_video_frame_operation_cb;
    TaskHandle_t video_stream_task_handle;
    EventGroupHandle_t video_event_group;
} app_video_t;

static app_video_t app_camera_video;

esp_err_t app_video_main(i2c_master_bus_handle_t i2c_bus_handle)
{
    esp_video_init_csi_config_t csi_config[] = {
        {
            .sccb_config = {
                .init_sccb = true,
                .i2c_config = {
                    .port      = 0,
                    .scl_pin   = 8,
                    .sda_pin   = 7,
                },
                .freq      = 100000,
            },
            .reset_pin = -1,
            .pwdn_pin  = -1,
        },
    };

    if (i2c_bus_handle != NULL) {
        csi_config[0].sccb_config.init_sccb = false;
        csi_config[0].sccb_config.i2c_handle = i2c_bus_handle;
    }

    esp_video_init_config_t cam_config = {
        .csi      = csi_config,
    };

    return esp_video_init(&cam_config);
}

int app_video_open(char *dev, video_fmt_t init_fmt)
{
    struct v4l2_format default_format;
    struct v4l2_capability capability;
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#if CONFIG_EXAMPLE_ENABLE_CAM_SENSOR_PIC_VFLIP || CONFIG_EXAMPLE_ENABLE_CAM_SENSOR_PIC_HFLIP
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control control[1];
#endif

    int fd = open(dev, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Open video failed");
        return -1;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &capability)) {
        ESP_LOGE(TAG, "failed to get capability");
        goto exit_0;
    }

    ESP_LOGI(TAG, "version: %d.%d.%d", (uint16_t)(capability.version >> 16),
             (uint8_t)(capability.version >> 8),
             (uint8_t)capability.version);
    ESP_LOGI(TAG, "driver:  %s", capability.driver);
    ESP_LOGI(TAG, "card:    %s", capability.card);
    ESP_LOGI(TAG, "bus:     %s", capability.bus_info);

    memset(&default_format, 0, sizeof(struct v4l2_format));
    default_format.type = type;
    if (ioctl(fd, VIDIOC_G_FMT, &default_format) != 0) {
        ESP_LOGE(TAG, "failed to get format");
        goto exit_0;
    }

    ESP_LOGI(TAG, "width=%" PRIu32 " height=%" PRIu32, default_format.fmt.pix.width, default_format.fmt.pix.height);

    app_camera_video.camera_buf_hes = default_format.fmt.pix.width;
    app_camera_video.camera_buf_ves = default_format.fmt.pix.height;

    if (default_format.fmt.pix.pixelformat != init_fmt) {
        struct v4l2_format format = {
            .type = type,
            .fmt.pix.width = default_format.fmt.pix.width,
            .fmt.pix.height = default_format.fmt.pix.height,
            .fmt.pix.pixelformat = init_fmt,
        };

        if (ioctl(fd, VIDIOC_S_FMT, &format) != 0) {
            ESP_LOGE(TAG, "failed to set format");
            goto exit_0;
        }
    }

#if CONFIG_EXAMPLE_ENABLE_CAM_SENSOR_PIC_VFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_VFLIP;
    control[0].value    = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to mirror the frame horizontally and skip this step");
    }
#endif

#if CONFIG_EXAMPLE_ENABLE_CAM_SENSOR_PIC_HFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count      = 1;
    controls.controls   = control;
    control[0].id       = V4L2_CID_HFLIP;
    control[0].value    = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        ESP_LOGW(TAG, "failed to mirror the frame horizontally and skip this step");
    }
#endif

    return fd;
exit_0:
    close(fd);
    return -1;
}

esp_err_t app_video_register_frame_operation_cb(app_video_frame_operation_cb_t operation_cb)
{
    app_camera_video.user_camera_video_frame_operation_cb = operation_cb;

    return ESP_OK;
}