# include "video.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_video_init.h"

static const char *TAG = "video";

#define MAX_BUFFER_COUNT                (6)
#define MIN_BUFFER_COUNT                (2)
#define VIDEO_TASK_STACK_SIZE           (4 * 1024)
#define VIDEO_TASK_PRIORITY             (3)

typedef enum {
    VIDEO_TASK_DELETE = BIT(0),
    VIDEO_TASK_DELETE_DONE = BIT(1),
} video_event_id_t;

typedef struct {
    uint8_t *camera_buffer[MAX_BUFFER_COUNT];
    size_t camera_buf_size;
    uint32_t camera_buf_hes;
    uint32_t camera_buf_ves;
    struct v4l2_buffer v4l2_buf;
    uint8_t camera_mem_mode;
    int video_fd;  // Store the video file descriptor
    app_video_frame_operation_cb_t user_camera_video_frame_operation_cb;
    TaskHandle_t video_stream_task_handle;
    EventGroupHandle_t video_event_group;
} app_video_t;

static app_video_t app_camera_video = {
    .video_fd = -1,
};

esp_err_t app_video_main(i2c_master_bus_handle_t i2c_bus_handle)
{
    esp_video_init_csi_config_t csi_config[] = {
        {
            .sccb_config = {
                .init_sccb = true,
                .i2c_config = {
                    .port      = 0,
                    .scl_pin   = BSP_I2C_SCL,
                    .sda_pin   = BSP_I2C_SDA,
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

    int fd = open(dev, O_RDWR);
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

esp_err_t app_video_set_bufs(int video_fd, uint32_t fb_num, const void **fb)
{
    if (fb_num > MAX_BUFFER_COUNT) {
        ESP_LOGE(TAG, "buffer num is too large");
        return ESP_FAIL;
    } else if (fb_num < MIN_BUFFER_COUNT) {
        ESP_LOGE(TAG, "At least two buffers are required");
        return ESP_FAIL;
    }

    struct v4l2_requestbuffers req;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memset(&req, 0, sizeof(req));
    req.count = fb_num;
    req.type = type;

    app_camera_video.camera_mem_mode = req.memory = fb ? V4L2_MEMORY_USERPTR : V4L2_MEMORY_MMAP;

    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "req bufs failed");
        goto errout_req_bufs;
    }
    for (int i = 0; i < fb_num; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = type;
        buf.memory = req.memory;
        buf.index = i;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "query buf failed");
            goto errout_req_bufs;
        }

        if (req.memory == V4L2_MEMORY_MMAP) {
            app_camera_video.camera_buffer[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
            if (app_camera_video.camera_buffer[i] == (void *)-1) {
                ESP_LOGE(TAG, "mmap failed, errno: %d (%s)", errno, strerror(errno));
                goto errout_req_bufs;
            }
        } else {
            if (!fb[i]) {
                ESP_LOGE(TAG, "frame buffer is NULL");
                goto errout_req_bufs;
            }
            buf.m.userptr = (unsigned long)fb[i];
            app_camera_video.camera_buffer[i] = (uint8_t *)fb[i];
        }

        app_camera_video.camera_buf_size = buf.length;

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "queue frame buffer failed");
            goto errout_req_bufs;
        }
    }

    ESP_LOGI(TAG, "Video buffers setup successfully, fd: %d", video_fd);
    return ESP_OK;

errout_req_bufs:
    ESP_LOGE(TAG, "Buffer setup failed, closing video_fd: %d", video_fd);
    close(video_fd);
    return ESP_FAIL;
}

esp_err_t app_video_get_bufs(int fb_num, void **fb)
{
    if (fb_num > MAX_BUFFER_COUNT) {
        ESP_LOGE(TAG, "buffer num is too large");
        return ESP_FAIL;
    } else if (fb_num < MIN_BUFFER_COUNT) {
        ESP_LOGE(TAG, "At least two buffers are required");
        return ESP_FAIL;
    }

    for (int i = 0; i < fb_num; i++) {
        if (app_camera_video.camera_buffer[i] != NULL) {
            fb[i] = app_camera_video.camera_buffer[i];
        } else {
            ESP_LOGE(TAG, "frame buffer is NULL");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

uint32_t app_video_get_buf_size(void)
{
    uint32_t buf_size = app_camera_video.camera_buf_hes * app_camera_video.camera_buf_ves * (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3);

    return buf_size;
}

esp_err_t app_video_get_resolution(uint32_t *width, uint32_t *height)
{
    if (!width || !height) {
        ESP_LOGE(TAG, "Width or height pointer is NULL");
        return ESP_FAIL;
    }

    *width = app_camera_video.camera_buf_hes;
    *height = app_camera_video.camera_buf_ves;

    return ESP_OK;
}

static inline esp_err_t video_receive_video_frame(int video_fd)
{
    memset(&app_camera_video.v4l2_buf, 0, sizeof(app_camera_video.v4l2_buf));
    app_camera_video.v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    app_camera_video.v4l2_buf.memory = app_camera_video.camera_mem_mode;

    int res = ioctl(video_fd, VIDIOC_DQBUF, &(app_camera_video.v4l2_buf));
    if (res != 0) {
        ESP_LOGE(TAG, "failed to receive video frame");
        goto errout;
    }

    return ESP_OK;

errout:
    return ESP_FAIL;
}

static inline void video_operation_video_frame(int video_fd)
{
    app_camera_video.v4l2_buf.m.userptr = (unsigned long)app_camera_video.camera_buffer[app_camera_video.v4l2_buf.index];
    app_camera_video.v4l2_buf.length = app_camera_video.camera_buf_size;

    uint8_t buf_index = app_camera_video.v4l2_buf.index;

    app_camera_video.user_camera_video_frame_operation_cb(
                        app_camera_video.camera_buffer[buf_index],
                        buf_index,
                        app_camera_video.camera_buf_hes,
                        app_camera_video.camera_buf_ves,
                        app_camera_video.camera_buf_size
                    );
}

static inline esp_err_t video_free_video_frame(int video_fd)
{
    if (ioctl(video_fd, VIDIOC_QBUF, &(app_camera_video.v4l2_buf)) != 0) {
        ESP_LOGE(TAG, "failed to free video frame");
        goto errout;
    }

    return ESP_OK;

errout:
    return ESP_FAIL;
}

static inline esp_err_t video_stream_start(int video_fd)
{
    ESP_LOGI(TAG, "Video Stream Start");

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type)) {
        ESP_LOGE(TAG, "failed to start stream, errno: %d (%s)", errno, strerror(errno));
        goto errout;
    }

    struct v4l2_format format = {0};
    format.type = type;
    if (ioctl(video_fd, VIDIOC_G_FMT, &format) != 0) {
        ESP_LOGE(TAG, "get fmt failed");
        goto errout;
    }

    return ESP_OK;

errout:
    return ESP_FAIL;
}

static inline esp_err_t video_stream_stop(int video_fd)
{
    ESP_LOGI(TAG, "Video Stream Stop");

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMOFF, &type)) {
        ESP_LOGE(TAG, "failed to stop stream");
        goto errout;
    }

    xEventGroupSetBits(app_camera_video.video_event_group, VIDEO_TASK_DELETE_DONE);

    return ESP_OK;

errout:
    return ESP_FAIL;
}

static void video_stream_task(void *arg)
{
    int video_fd = app_camera_video.video_fd;
    
    ESP_LOGI(TAG, "Video stream task starting with fd: %d", video_fd);

    // Start the video stream now that buffers are set up
    ESP_ERROR_CHECK(video_stream_start(video_fd));

    while (1) {
        ESP_ERROR_CHECK(video_receive_video_frame(video_fd));

        video_operation_video_frame(video_fd);

        ESP_ERROR_CHECK(video_free_video_frame(video_fd));

        if(xEventGroupGetBits(app_camera_video.video_event_group) & VIDEO_TASK_DELETE) {
            xEventGroupClearBits(app_camera_video.video_event_group, VIDEO_TASK_DELETE);
            ESP_ERROR_CHECK(video_stream_stop(video_fd));
            vTaskDelete(NULL);
        }
    }
    vTaskDelete(NULL);
}

esp_err_t app_video_stream_task_start(int video_fd, int core_id)
{
    if(app_camera_video.video_event_group == NULL) {
        app_camera_video.video_event_group = xEventGroupCreate();
    }
    xEventGroupClearBits(app_camera_video.video_event_group, VIDEO_TASK_DELETE_DONE);

    // Store the file descriptor in the global structure
    app_camera_video.video_fd = video_fd;

    // Don't start video stream here - it should be started after buffers are set up
    // video_stream_start(video_fd);

    BaseType_t result = xTaskCreatePinnedToCore(video_stream_task, "video stream task", VIDEO_TASK_STACK_SIZE, NULL, VIDEO_TASK_PRIORITY, &app_camera_video.video_stream_task_handle, core_id);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "failed to create video stream task");
        goto errout;
    }

    return ESP_OK;

errout:
    // video_stream_stop(video_fd);
    return ESP_FAIL;
}

esp_err_t app_video_stream_task_stop(int video_fd)
{
    xEventGroupSetBits(app_camera_video.video_event_group, VIDEO_TASK_DELETE);

    return ESP_OK;
}

esp_err_t app_video_register_frame_operation_cb(app_video_frame_operation_cb_t operation_cb)
{
    app_camera_video.user_camera_video_frame_operation_cb = operation_cb;

    return ESP_OK;
}

esp_err_t app_video_close(int video_fd)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Closing video device, fd: %d", video_fd);
    
    // Stop the video stream task first
    esp_err_t stop_err = app_video_stream_task_stop(video_fd);
    if (stop_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop video stream task: %s", esp_err_to_name(stop_err));
        ret = ESP_FAIL;
    }
    
    // Wait for task to complete cleanup
    if (app_camera_video.video_event_group) {
        xEventGroupWaitBits(app_camera_video.video_event_group, 
                           VIDEO_TASK_DELETE_DONE, 
                           pdFALSE, 
                           pdFALSE, 
                           pdMS_TO_TICKS(1000));
    }
    
    // Close the video file descriptor
    if (video_fd >= 0) {
        if (close(video_fd) != 0) {
            ESP_LOGE(TAG, "Failed to close video device: %s", strerror(errno));
            ret = ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "Video device closed successfully");
        }
    }
    
    // Clean up event group
    if (app_camera_video.video_event_group) {
        vEventGroupDelete(app_camera_video.video_event_group);
        app_camera_video.video_event_group = NULL;
    }
    
    // Reset global state
    memset(&app_camera_video, 0, sizeof(app_camera_video));
    app_camera_video.video_fd = -1;
    
    return ret;
}

esp_err_t app_video_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing video system");
    
    esp_err_t ret = esp_video_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize video system: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Video system deinitialized successfully");
    return ESP_OK;
}