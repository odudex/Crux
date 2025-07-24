#include "esp_err.h"
#include "esp_log.h"

#include "bsp/esp-bsp.h"

#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
// #include "app_video.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_VIDEO_FMT_RAW8 = V4L2_PIX_FMT_SBGGR8,
    APP_VIDEO_FMT_RAW10 = V4L2_PIX_FMT_SBGGR10,
    APP_VIDEO_FMT_GREY = V4L2_PIX_FMT_GREY,
    APP_VIDEO_FMT_RGB565 = V4L2_PIX_FMT_RGB565,
    APP_VIDEO_FMT_RGB888 = V4L2_PIX_FMT_RGB24,
    APP_VIDEO_FMT_YUV422 = V4L2_PIX_FMT_YUV422P,
    APP_VIDEO_FMT_YUV420 = V4L2_PIX_FMT_YUV420,
} video_fmt_t;

#define EXAMPLE_CAM_DEV_PATH                (ESP_VIDEO_MIPI_CSI_DEVICE_NAME)
#define EXAMPLE_CAM_BUF_NUM                 (4)

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB565
#define APP_VIDEO_FMT              (APP_VIDEO_FMT_RGB565)
#elif CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
#define APP_VIDEO_FMT              (APP_VIDEO_FMT_RGB888)
#endif

typedef void (*app_video_frame_operation_cb_t)(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len);

esp_err_t app_video_main(i2c_master_bus_handle_t i2c_bus_handle);

int app_video_open(char *dev, video_fmt_t init_fmt);

esp_err_t app_video_register_frame_operation_cb(app_video_frame_operation_cb_t operation_cb);

#ifdef __cplusplus
}
#endif

