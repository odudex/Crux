/*
 * QR Scanner Page - Displays a 480x480 frame buffer that changes colors
 * Returns to login page when screen is touched
 */

// #include <stdint.h>
#include <string.h>
#include "qr_scanner.h"
#include "../ui_components/tron_theme.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_private/esp_cache_private.h"
#include "driver/ppa.h"

#define ALIGN_UP_BY(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#define CAMERA_INIT_TASK_WAIT_MS            (1000)
#define DETECT_NUM_MAX                      (10)
#define FPS_PRINT                           (1)

typedef enum {
    CAMERA_EVENT_TASK_RUN = BIT(0),
    CAMERA_EVENT_DELETE = BIT(1),
    CAMERA_EVENT_PED_DETECT = BIT(2),
    CAMERA_EVENT_HUMAN_DETECT = BIT(3),
} camera_event_id_t;

static const char *TAG = "QR_SCANNER";

// Global variables for the QR scanner page
static lv_obj_t *qr_scanner_screen = NULL;
static lv_obj_t *frame_buffer = NULL;
static lv_timer_t *color_timer = NULL;
static void (*return_callback)(void) = NULL;

// Video variables
static size_t data_cache_line_size = 0;
int _camera_ctlr_handle;
uint16_t hor_res;
uint16_t ver_res;
uint8_t *_cam_buffer[EXAMPLE_CAM_BUF_NUM];
size_t _cam_buffer_size[EXAMPLE_CAM_BUF_NUM];
lv_img_dsc_t _img_refresh_dsc;

// Other variables
static ppa_client_handle_t ppa_client_srm_handle = NULL;
static EventGroupHandle_t camera_event_group;
SemaphoreHandle_t _camera_init_sem = NULL;

bool camera_run(void);
void camera_init(void);

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, 
                                       uint32_t camera_buf_hes, uint32_t camera_buf_ves, 
                                       size_t camera_buf_len);

// static bool ppa_trans_done_cb(ppa_client_handle_t ppa_client, ppa_event_data_t *event_data, void *user_data);

// Color array for cycling through different colors
static const lv_color_t colors[] = {
    LV_COLOR_MAKE(255, 0, 0),     // Red
    LV_COLOR_MAKE(0, 255, 0),     // Green  
    LV_COLOR_MAKE(0, 0, 255),     // Blue
    LV_COLOR_MAKE(255, 255, 0),   // Yellow
    LV_COLOR_MAKE(255, 0, 255),   // Magenta
    LV_COLOR_MAKE(0, 255, 255),   // Cyan
    LV_COLOR_MAKE(255, 128, 0),   // Orange
    LV_COLOR_MAKE(128, 0, 255),   // Purple
};

static uint8_t current_color_index = 0;

// Touch event handler - returns to login when screen is touched
static void touch_event_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Screen touched, returning to previous page");
    
    if (return_callback) {
        return_callback();
    }
}

// Timer callback to change colors every second
static void color_timer_cb(lv_timer_t *timer)
{
    if (!frame_buffer) {
        return;
    }
    
    // Cycle through colors
    current_color_index = (current_color_index + 1) % (sizeof(colors) / sizeof(colors[0]));
    
    // Apply new color to frame buffer
    lv_obj_set_style_bg_color(frame_buffer, colors[current_color_index], 0);
    
    ESP_LOGI(TAG, "Frame buffer color changed to index %d", current_color_index);
}

void qr_scanner_page_create(lv_obj_t *parent, void (*return_cb)(void))
{
    if (!parent) {
        ESP_LOGE(TAG, "Invalid parent object for QR scanner page");
        return;
    }
    
    ESP_LOGI(TAG, "Creating QR scanner page");
    
    // Store the return callback
    return_callback = return_cb;
    
    // Create screen container
    qr_scanner_screen = lv_obj_create(parent);
    lv_obj_set_size(qr_scanner_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(qr_scanner_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Remove container borders and padding
    lv_obj_set_style_border_width(qr_scanner_screen, 0, 0);
    lv_obj_set_style_pad_all(qr_scanner_screen, 0, 0);
    lv_obj_set_style_radius(qr_scanner_screen, 0, 0);
    lv_obj_set_style_shadow_width(qr_scanner_screen, 0, 0);
    lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add touch event to the main screen
    lv_obj_add_event_cb(qr_scanner_screen, touch_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create title label
    lv_obj_t *title_label = tron_theme_create_label(qr_scanner_screen, "QR Scanner", false);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create 480x480 frame buffer container
    frame_buffer = lv_obj_create(qr_scanner_screen);
    lv_obj_set_size(frame_buffer, 480, 480);
    lv_obj_center(frame_buffer);
    
    // Style the frame buffer
    lv_obj_set_style_bg_color(frame_buffer, colors[current_color_index], 0);
    lv_obj_set_style_border_color(frame_buffer, lv_color_white(), 0);
    lv_obj_set_style_border_width(frame_buffer, 2, 0);
    lv_obj_set_style_radius(frame_buffer, 10, 0);
    lv_obj_clear_flag(frame_buffer, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add touch event to frame buffer as well
    lv_obj_add_event_cb(frame_buffer, touch_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create instruction label
    lv_obj_t *instruction_label = tron_theme_create_label(qr_scanner_screen, "Touch screen to return", false);
    lv_obj_align(instruction_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Create timer to change colors every second (1000ms)
    color_timer = lv_timer_create(color_timer_cb, 1000, NULL);

    if(!camera_run())
    {
        ESP_LOGE(TAG, "Failed to initialize camera");
        return;
    }
    
    
    ESP_LOGI(TAG, "QR scanner page created successfully");
}

bool camera_run(void)
{
    // if (_camera_init_sem == NULL) {
    //     _camera_init_sem = xSemaphoreCreateBinary();
    //     assert(_camera_init_sem != NULL);

    //     xTaskCreatePinnedToCore((TaskFunction_t)taskCameraInit, "Camera Init", 4096, this, 2, NULL, 0);
    //     if (xSemaphoreTake(_camera_init_sem, pdMS_TO_TICKS(CAMERA_INIT_TASK_WAIT_MS)) != pdTRUE) {
    //         ESP_LOGE(TAG, "Camera init timeout");
    //         return false;
    //     }
    //     free(_camera_init_sem);
    //     _camera_init_sem = NULL;
    // }
    camera_init();
    ESP_LOGI(TAG, "Camera initialized successfully");
    return true;
}

void camera_init(void)
{
    // Initialize camera
    i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
    if (i2c_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle");
        return;
    }
    ESP_LOGI(TAG, "I2C bus handle obtained successfully");
    
    esp_err_t err = app_video_main(i2c_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize camera: %s", esp_err_to_name(err));
        return;
    }

    // Open the video device
    _camera_ctlr_handle = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT_RGB565);
    if (_camera_ctlr_handle < 0) {
        ESP_LOGE(TAG, "video cam open failed");

        if (ESP_OK == i2c_master_probe(i2c_handle, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 100) || ESP_OK == i2c_master_probe(i2c_handle, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, 100)) {
            ESP_LOGI(TAG, "gt911 touch found");
        } else {
            ESP_LOGE(TAG, "Touch not found");
        }
    }

    hor_res = 1280;
    ver_res = 720;
    ESP_ERROR_CHECK(esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size));
    for (int i = 0; i < EXAMPLE_CAM_BUF_NUM; i++) {
        _cam_buffer[i] = (uint8_t *)heap_caps_aligned_alloc(data_cache_line_size, hor_res * ver_res * BSP_LCD_BITS_PER_PIXEL / 8, MALLOC_CAP_SPIRAM);
        _cam_buffer_size[i] = hor_res * ver_res * BSP_LCD_BITS_PER_PIXEL / 8;
    }

    // Register the video frame operation callback
    ESP_ERROR_CHECK(app_video_register_frame_operation_cb(camera_video_frame_operation));

    lv_img_dsc_t img_dsc = {
        .header = {
            .cf = LV_COLOR_FORMAT_RGB565,
            // .always_zero = 0,
            // .reserved = 0,
            .w = hor_res,
            .h = ver_res,
        },
        .data_size = _cam_buffer_size[0],
        .data = (const uint8_t *)_cam_buffer[0],
    };

    memcpy(&_img_refresh_dsc, &img_dsc, sizeof(lv_img_dsc_t));

    // Pipeline for image processing/detection
    
    // size_t detect_buf_size = ALIGN_UP_BY(hor_res * ver_res * BSP_LCD_BITS_PER_PIXEL / 8, data_cache_line_size);

    // ppa_client_config_t srm_config =  {
    //     .oper_type = PPA_OPERATION_SRM,
    // };
    // ESP_ERROR_CHECK(ppa_register_client(&srm_config, &ppa_client_srm_handle));

    // ppa_event_callbacks_t cbs = {
    //     .on_trans_done = ppa_trans_done_cb,
    // };
    // ppa_client_register_event_callbacks(ppa_client_srm_handle, &cbs);
}

void qr_scanner_page_show(void)
{
    if (qr_scanner_screen) {
        lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "QR scanner page shown");
        
        // Resume the color timer
        if (color_timer) {
            lv_timer_resume(color_timer);
        }
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_hide(void)
{
    if (qr_scanner_screen) {
        lv_obj_add_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "QR scanner page hidden");
        
        // Pause the color timer
        if (color_timer) {
            lv_timer_pause(color_timer);
        }
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_destroy(void)
{
    // Destroy the color timer
    if (color_timer) {
        lv_timer_del(color_timer);
        color_timer = NULL;
        ESP_LOGI(TAG, "Color timer destroyed");
    }
    
    // Destroy the screen and all its children
    if (qr_scanner_screen) {
        lv_obj_del(qr_scanner_screen);
        qr_scanner_screen = NULL;
        frame_buffer = NULL; // Will be deleted with parent
        ESP_LOGI(TAG, "QR scanner screen destroyed");
    }
    
    // Clear the return callback
    return_callback = NULL;
}

// static bool ppa_trans_done_cb(ppa_client_handle_t ppa_client, ppa_event_data_t *event_data, void *user_data)
// {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;

//     camera_pipeline_buffer_element *p = (camera_pipeline_buffer_element *)user_data;
//     camera_pipeline_done_element(feed_pipeline, p);

//     return (xHigherPriorityTaskWoken == pdTRUE);
// }

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, 
                                       uint32_t camera_buf_hes, uint32_t camera_buf_ves, 
                                       size_t camera_buf_len)
{
    // Wait for task run event
    xEventGroupWaitBits(camera_event_group, CAMERA_EVENT_TASK_RUN, pdFALSE, pdTRUE, portMAX_DELAY);

    EventBits_t current_bits = xEventGroupGetBits(camera_event_group);
    
    // Update display if not in delete state
    if (!(current_bits & CAMERA_EVENT_DELETE) && bsp_display_lock(100)) {
        // if (ui_ImageCameraShotImage) {
        //     lv_canvas_set_buffer(ui_ImageCameraShotImage, camera_buf, 
        //                        camera_buf_hes, camera_buf_ves, 
        //                        LV_IMG_CF_TRUE_COLOR);
        // }
        lv_refr_now(NULL);
        bsp_display_unlock();
    }

// #if FPS_PRINT
//     static int count = 0;
//     if (count % 10 == 0) {
//         perfmon_start(0, "PFS", "camera");
//     } else if (count % 10 == 9) {
//         perfmon_end(0, 10);
//     }
//     count++;
// #endif
}