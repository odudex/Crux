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
static lv_obj_t *camera_img = NULL;
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

// Touch event handler - returns to login when screen is touched
static void touch_event_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Screen touched, returning to previous page");
    
    if (return_callback) {
        return_callback();
    }
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
    
    // Style the frame buffer - transparent background with white border
    lv_obj_set_style_bg_opa(frame_buffer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(frame_buffer, lv_color_white(), 0);
    lv_obj_set_style_border_width(frame_buffer, 2, 0);
    lv_obj_set_style_radius(frame_buffer, 10, 0);
    lv_obj_clear_flag(frame_buffer, LV_OBJ_FLAG_SCROLLABLE);
    
    // Add touch event to frame buffer as well
    lv_obj_add_event_cb(frame_buffer, touch_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create camera image object inside frame buffer
    camera_img = lv_img_create(frame_buffer);
    lv_obj_set_size(camera_img, 480, 480);
    lv_obj_center(camera_img);
    lv_obj_clear_flag(camera_img, LV_OBJ_FLAG_SCROLLABLE);
    
    // Set white background so we can see the camera area
    lv_obj_set_style_bg_color(camera_img, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(camera_img, LV_OPA_COVER, 0);
    
    // Create instruction label
    lv_obj_t *instruction_label = tron_theme_create_label(qr_scanner_screen, "Touch screen to return", false);
    lv_obj_align(instruction_label, LV_ALIGN_BOTTOM_MID, 0, -20);

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
    // Create camera event group
    camera_event_group = xEventGroupCreate();
    if (camera_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create camera event group");
        return;
    }
    
    // Set the task run event
    xEventGroupSetBits(camera_event_group, CAMERA_EVENT_TASK_RUN);
    
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
        return;
    }
    
    ESP_LOGI(TAG, "Camera device opened successfully, handle: %d", _camera_ctlr_handle);

    // Register the video frame operation callback
    ESP_ERROR_CHECK(app_video_register_frame_operation_cb(camera_video_frame_operation));
    ESP_LOGI(TAG, "Frame operation callback registered");

    // Set up video buffers - let video system manage its own buffers
    ESP_LOGI(TAG, "Setting up video buffers");
    ESP_ERROR_CHECK(app_video_set_bufs(_camera_ctlr_handle, EXAMPLE_CAM_BUF_NUM, NULL));

    // Initialize image descriptor - dimensions will be updated by callback
    lv_img_dsc_t img_dsc = {
        .header = {
            .cf = LV_COLOR_FORMAT_RGB565,
            .w = hor_res,
            .h = ver_res,
        },
        .data_size = 0,  // Will be set by callback
        .data = NULL,    // Will be set by callback
    };

    memcpy(&_img_refresh_dsc, &img_dsc, sizeof(lv_img_dsc_t));
    
    // Start the camera stream task
    
    ESP_ERROR_CHECK(app_video_set_bufs(_camera_ctlr_handle, EXAMPLE_CAM_BUF_NUM, (const void **)_cam_buffer));

    ESP_LOGI(TAG, "Start camera stream task");
    
    esp_err_t start_err = app_video_stream_task_start(_camera_ctlr_handle, 0);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start camera stream task: %s", esp_err_to_name(start_err));
        return;
    }
    
    ESP_LOGI(TAG, "Camera stream task started successfully");

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
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_hide(void)
{
    if (qr_scanner_screen) {
        lv_obj_add_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "QR scanner page hidden");
    } else {
        ESP_LOGE(TAG, "QR scanner screen not initialized");
    }
}

void qr_scanner_page_destroy(void)
{
    // Clean up camera event group
    if (camera_event_group) {
        xEventGroupSetBits(camera_event_group, CAMERA_EVENT_DELETE);
        vEventGroupDelete(camera_event_group);
        camera_event_group = NULL;
    }
    
    // Destroy the screen and all its children
    if (qr_scanner_screen) {
        lv_obj_del(qr_scanner_screen);
        qr_scanner_screen = NULL;
        frame_buffer = NULL; // Will be deleted with parent
        camera_img = NULL; // Will be deleted with parent
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
    static int frame_count = 0;
    frame_count++;
    
    // Add debug logging every 30 frames to avoid spam
    // if (frame_count % 30 == 1) {
    //     ESP_LOGI(TAG, "Frame callback #%d: buf=%p, size=%dx%d, len=%zu", 
    //              frame_count, camera_buf, camera_buf_hes, camera_buf_ves, camera_buf_len);
    //     ESP_LOGI(TAG, "Event group=%p, camera_img=%p", camera_event_group, camera_img);
    // }
    
    // Check if camera event group exists and components are ready
    if (!camera_event_group || !camera_img) {
        // if (frame_count % 30 == 1) {
        //     ESP_LOGW(TAG, "Missing components - event_group=%p, camera_img=%p", camera_event_group, camera_img);
        // }
        return;
    }

    EventBits_t current_bits = xEventGroupGetBits(camera_event_group);
    
    // if (frame_count % 30 == 1) {
    //     ESP_LOGI(TAG, "Event bits: 0x%lx (RUN=%d, DELETE=%d)", 
    //              current_bits, 
    //              !!(current_bits & CAMERA_EVENT_TASK_RUN),
    //              !!(current_bits & CAMERA_EVENT_DELETE));
    // }
    
    // Update display if not in delete state
    if ((current_bits & CAMERA_EVENT_TASK_RUN) && !(current_bits & CAMERA_EVENT_DELETE) && bsp_display_lock(100)) {
        // Update the image descriptor with new frame data
        _img_refresh_dsc.data = (const uint8_t *)camera_buf;
        _img_refresh_dsc.header.w = camera_buf_hes;
        _img_refresh_dsc.header.h = camera_buf_ves;
        _img_refresh_dsc.data_size = camera_buf_len;
        
        // if (frame_count % 30 == 1) {
        //     ESP_LOGI(TAG, "Updating image: %dx%d, format=%d", camera_buf_hes, camera_buf_ves, _img_refresh_dsc.header.cf);
        // }
        
        // Set the new image source
        lv_img_set_src(camera_img, &_img_refresh_dsc);
        
        lv_refr_now(NULL);
        bsp_display_unlock();
        
        // if (frame_count % 30 == 1) {
        //     ESP_LOGI(TAG, "Frame displayed successfully");
        // }
    }
    // else {
    //     if (frame_count % 30 == 1) {
    //         ESP_LOGW(TAG, "Display update skipped - display_lock failed or wrong event state");
    //     }
    // }
}