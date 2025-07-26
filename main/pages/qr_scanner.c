/*
 * QR Scanner Page
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
// static size_t data_cache_line_size = 0; //Pipeline cache line size
int _camera_ctlr_handle = -1;
uint16_t hor_res;
uint16_t ver_res;
uint8_t *_cam_buffer[EXAMPLE_CAM_BUF_NUM];
size_t _cam_buffer_size[EXAMPLE_CAM_BUF_NUM];
lv_img_dsc_t _img_refresh_dsc;

// Other variables
// static ppa_client_handle_t ppa_client_srm_handle = NULL;  //pipeline SRM handle
static EventGroupHandle_t camera_event_group;
SemaphoreHandle_t _camera_init_sem = NULL;
static bool video_system_initialized = false;

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
    
    // Create 600x600 frame buffer container
    frame_buffer = lv_obj_create(qr_scanner_screen);
    lv_obj_set_size(frame_buffer, 600, 600);
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
    lv_obj_set_size(camera_img, 600, 600);
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
    // Check if camera is already running
    if (_camera_ctlr_handle >= 0 && video_system_initialized) {
        ESP_LOGI(TAG, "Camera already running, skipping initialization");
        return true;
    }
    
    camera_init();
    ESP_LOGI(TAG, "Camera initialized successfully");
    return true;
}

void camera_init(void)
{
    ESP_LOGI(TAG, "Initializing camera");
    
    // Check if video system is already initialized
    if (video_system_initialized) {
        ESP_LOGI(TAG, "Video system already initialized, skipping init");
        return;
    }
    
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
    
    video_system_initialized = true;
    ESP_LOGI(TAG, "Video system initialized successfully");

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

    // Get camera resolution from video system
    hor_res = 1280;  // Will be updated by video system
    ver_res = 720;

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
    
    // Set up video buffers - let video system manage its own buffers
    ESP_LOGI(TAG, "Setting up video buffers");
    ESP_ERROR_CHECK(app_video_set_bufs(_camera_ctlr_handle, EXAMPLE_CAM_BUF_NUM, NULL));

    ESP_LOGI(TAG, "Starting camera stream task");
    
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
    ESP_LOGI(TAG, "Destroying QR scanner page");
    
    // Stop video stream task and close video device
    if (_camera_ctlr_handle >= 0) {
        ESP_LOGI(TAG, "Stopping video stream and closing device");
        esp_err_t close_err = app_video_close(_camera_ctlr_handle);
        if (close_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to close video device: %s", esp_err_to_name(close_err));
        }
        _camera_ctlr_handle = -1;
    }
    
    // Deinitialize video system
    esp_err_t deinit_err = app_video_deinit();
    if (deinit_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize video system: %s", esp_err_to_name(deinit_err));
    } else {
        video_system_initialized = false;
        ESP_LOGI(TAG, "Video system deinitialized successfully");
    }
    
    // Clean up camera event group (if not already cleaned by app_video_close)
    if (camera_event_group) {
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
    
    // Reset video-related variables
    hor_res = 0;
    ver_res = 0;
    memset(_cam_buffer, 0, sizeof(_cam_buffer));
    memset(_cam_buffer_size, 0, sizeof(_cam_buffer_size));
    
    ESP_LOGI(TAG, "QR scanner page cleanup completed");
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
 
    
    // Check if camera event group exists and components are ready
    if (!camera_event_group || !camera_img) {
        return;
    }

    EventBits_t current_bits = xEventGroupGetBits(camera_event_group);
    
    // Update display if not in delete state
    if ((current_bits & CAMERA_EVENT_TASK_RUN) && !(current_bits & CAMERA_EVENT_DELETE) && bsp_display_lock(100)) {
        // Update the image descriptor with new frame data
        _img_refresh_dsc.data = (const uint8_t *)camera_buf;
        _img_refresh_dsc.header.w = camera_buf_hes;
        _img_refresh_dsc.header.h = camera_buf_ves;
        _img_refresh_dsc.data_size = camera_buf_len;
        
        // Set the new image source
        lv_img_set_src(camera_img, &_img_refresh_dsc);
        
        lv_refr_now(NULL);
        bsp_display_unlock();
    }
}