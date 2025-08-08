/*
 * QR Scanner Page
 * Returns to login page when screen is touched
 */

// #include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "qr_scanner.h"
#include "../ui_components/tron_theme.h"
#include <esp_log.h>
#include <lvgl.h>
#include <esp_lcd_touch_gt911.h>
#include <quirc.h>
#include <esp_timer.h>

#define ALIGN_UP_BY(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#define CAMERA_INIT_TASK_WAIT_MS            (1000)
#define DETECT_NUM_MAX                      (10)
#define FPS_PRINT                           (1)

#define CAMERA_SCREEN_WIDTH                      (720)
#define CAMERA_SCREEN_HEIGHT                     (720)

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
static lv_obj_t *qr_result_label = NULL;
static void (*return_callback)(void) = NULL;

// QR decoder variables
static struct quirc *qr_decoder = NULL;
static uint8_t *grayscale_buffer = NULL;
static SemaphoreHandle_t qr_decode_mutex = NULL;
static TaskHandle_t qr_decode_task_handle = NULL;
static QueueHandle_t qr_frame_queue = NULL;

#define QR_FRAME_QUEUE_SIZE 2
#define QR_DECODE_TASK_STACK_SIZE 16384
#define QR_DECODE_TASK_PRIORITY 5

// Add downsampling parameters
#define QR_DECODE_SCALE_FACTOR 2  // Downsample by 2x (800x800 -> 400x400)
#define QR_DECODE_ROI_PERCENT 75  // Use center 75% of image

typedef struct {
    uint8_t *frame_data;
    uint32_t width;
    uint32_t height;
    size_t data_len;
} qr_frame_data_t;

// Video variables
// static size_t data_cache_line_size = 0; //Pipeline cache line size
int _camera_ctlr_handle = -1;
uint16_t hor_res;
uint16_t ver_res;
uint8_t *_cam_buffer[EXAMPLE_CAM_BUF_NUM];
size_t _cam_buffer_size[EXAMPLE_CAM_BUF_NUM];
lv_img_dsc_t _img_refresh_dsc;

// Double buffering for display to avoid tearing
static uint8_t *display_buffer = NULL;
static SemaphoreHandle_t display_buffer_mutex = NULL;

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

// Add these QR decoder function declarations
static void rgb565_to_grayscale(const uint8_t *rgb565_data, uint8_t *gray_data, 
                               uint32_t width, uint32_t height);
static void qr_decode_task(void *pvParameters);
static bool qr_decoder_init(uint32_t width, uint32_t height);
static void qr_decoder_cleanup(void);

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

    // Create frame buffer container
    frame_buffer = lv_obj_create(qr_scanner_screen);
    lv_obj_set_size(frame_buffer, 800, 800);
    lv_obj_center(frame_buffer);

    // Remove frame buffer styling - no borders or background
    lv_obj_set_style_bg_opa(frame_buffer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame_buffer, 0, 0);
    lv_obj_set_style_radius(frame_buffer, 0, 0);
    lv_obj_clear_flag(frame_buffer, LV_OBJ_FLAG_SCROLLABLE);

    // Add touch event to frame buffer as well
    lv_obj_add_event_cb(frame_buffer, touch_event_cb, LV_EVENT_CLICKED, NULL);

    // Create camera image object inside frame buffer
    camera_img = lv_img_create(frame_buffer);
    lv_obj_set_size(camera_img, CAMERA_SCREEN_WIDTH, CAMERA_SCREEN_HEIGHT);
    lv_obj_center(camera_img);
    lv_obj_clear_flag(camera_img, LV_OBJ_FLAG_SCROLLABLE);

    // Set white background so we can see the camera area
    lv_obj_set_style_bg_color(camera_img, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(camera_img, LV_OPA_COVER, 0);

    // Create title label
    lv_obj_t *title_label = tron_theme_create_label(qr_scanner_screen, "QR Scanner", false);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create QR result label
    qr_result_label = tron_theme_create_label(qr_scanner_screen, "Point camera at QR code", false);
    lv_obj_set_style_text_font(qr_result_label, &lv_font_montserrat_14, 0);
    lv_obj_align(qr_result_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_align(qr_result_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(qr_result_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(qr_result_label, LV_PCT(90));

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

    // Get actual camera resolution from video system
    uint32_t actual_width, actual_height;
    esp_err_t res_err = app_video_get_resolution(&actual_width, &actual_height);
    if (res_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get video resolution, using defaults");
        hor_res = 640;  // Default fallback based on our configured format
        ver_res = 480;
    } else {
        hor_res = actual_width;
        ver_res = actual_height;
        ESP_LOGI(TAG, "Using actual video resolution: %dx%d", hor_res, ver_res);
    }

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

    // Allocate display buffer for double buffering
    size_t buffer_size = hor_res * ver_res * 2; // RGB565 = 2 bytes per pixel
    display_buffer = malloc(buffer_size);
    if (!display_buffer) {
        ESP_LOGE(TAG, "Failed to allocate display buffer");
        return;
    }
    ESP_LOGI(TAG, "Allocated display buffer: %zu bytes", buffer_size);

    // Create mutex for display buffer protection
    display_buffer_mutex = xSemaphoreCreateMutex();
    if (!display_buffer_mutex) {
        ESP_LOGE(TAG, "Failed to create display buffer mutex");
        free(display_buffer);
        display_buffer = NULL;
        return;
    }

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

    // Initialize QR decoder after camera is ready
    if (!qr_decoder_init(hor_res, ver_res)) {
        ESP_LOGE(TAG, "Failed to initialize QR decoder");
        return;
    }

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

    // Cleanup QR decoder first
    qr_decoder_cleanup();

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

    // Clean up display buffer and mutex
    if (display_buffer_mutex) {
        vSemaphoreDelete(display_buffer_mutex);
        display_buffer_mutex = NULL;
    }

    if (display_buffer) {
        free(display_buffer);
        display_buffer = NULL;
        ESP_LOGI(TAG, "Display buffer freed");
    }

    // Destroy the screen and all its children
    if (qr_scanner_screen) {
        lv_obj_del(qr_scanner_screen);
        qr_scanner_screen = NULL;
        frame_buffer = NULL; // Will be deleted with parent
        camera_img = NULL; // Will be deleted with parent
        qr_result_label = NULL; // Will be deleted with parent
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
    if ((current_bits & CAMERA_EVENT_TASK_RUN) && !(current_bits & CAMERA_EVENT_DELETE)) {
        // Copy to display buffer with mutex protection to avoid tearing
        if (display_buffer && display_buffer_mutex && xSemaphoreTake(display_buffer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            memcpy(display_buffer, camera_buf, camera_buf_len);

            xSemaphoreGive(display_buffer_mutex);

            // Now update display with the stable buffer
            if (bsp_display_lock(100)) {
                // Update the image descriptor with display buffer
                _img_refresh_dsc.data = (const uint8_t *)display_buffer;
                _img_refresh_dsc.header.w = camera_buf_hes;
                _img_refresh_dsc.header.h = camera_buf_ves;
                _img_refresh_dsc.data_size = camera_buf_len;

                // Set the new image source
                lv_img_set_src(camera_img, &_img_refresh_dsc);

                lv_refr_now(NULL);
                bsp_display_unlock();
            }
        }

        // Send frame for QR code analysis (throttled to avoid overloading)
        static int64_t last_qr_analysis = 0;
        int64_t current_time = esp_timer_get_time();
        if (qr_frame_queue && current_time - last_qr_analysis > 1000000) { // Increased to 1 second
            last_qr_analysis = current_time;

            // Allocate memory for frame copy
            uint8_t *frame_copy = malloc(camera_buf_len);
            if (frame_copy) {
                memcpy(frame_copy, camera_buf, camera_buf_len);

                qr_frame_data_t frame_data = {
                    .frame_data = frame_copy,
                    .width = camera_buf_hes,
                    .height = camera_buf_ves,
                    .data_len = camera_buf_len
                };

                // Try to send to queue, free memory if queue is full
                if (xQueueSend(qr_frame_queue, &frame_data, 0) != pdTRUE) {
                    free(frame_copy);
                    ESP_LOGD(TAG, "QR decode queue full, skipping frame");
                }
            }
        }
    }
}

// Convert RGB565 to grayscale for QR decoding
static void rgb565_to_grayscale(const uint8_t *rgb565_data, uint8_t *gray_data, 
                               uint32_t width, uint32_t height)
{
    const uint16_t *pixels = (const uint16_t *)rgb565_data;

    for (uint32_t i = 0; i < width * height; i++) {
        // Read pixel with proper byte order
        uint16_t pixel = pixels[i];

        // Extract RGB components from RGB565 (assuming little-endian)
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;

        // Convert to 8-bit values
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;

        // Calculate grayscale using luminance formula
        gray_data[i] = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    }
}

// Convert RGB565 to grayscale with optional downsampling
static void rgb565_to_grayscale_downsample(const uint8_t *rgb565_data, uint8_t *gray_data, 
                                          uint32_t src_width, uint32_t src_height,
                                          uint32_t dst_width, uint32_t dst_height,
                                          uint32_t roi_x, uint32_t roi_y,
                                          uint32_t roi_width, uint32_t roi_height)
{
    const uint16_t *pixels = (const uint16_t *)rgb565_data;
    float x_scale = (float)roi_width / dst_width;
    float y_scale = (float)roi_height / dst_height;

    for (uint32_t dst_y = 0; dst_y < dst_height; dst_y++) {
        for (uint32_t dst_x = 0; dst_x < dst_width; dst_x++) {
            // Map destination pixel to source ROI
            uint32_t src_x = roi_x + (uint32_t)(dst_x * x_scale);
            uint32_t src_y = roi_y + (uint32_t)(dst_y * y_scale);

            // Ensure we're within bounds
            if (src_x >= src_width) src_x = src_width - 1;
            if (src_y >= src_height) src_y = src_height - 1;

            uint32_t src_idx = src_y * src_width + src_x;
            uint16_t pixel = pixels[src_idx];

            // Extract RGB components from RGB565
            uint8_t r = (pixel >> 11) & 0x1F;
            uint8_t g = (pixel >> 5) & 0x3F;
            uint8_t b = pixel & 0x1F;

            // Convert to 8-bit values
            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;

            // Calculate grayscale using luminance formula
            gray_data[dst_y * dst_width + dst_x] = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }
}

// QR decode task
static void qr_decode_task(void *pvParameters)
{
    qr_frame_data_t frame_data;

    ESP_LOGI(TAG, "QR decode task started");

    // Allocate quirc_data on heap to avoid stack overflow
    struct quirc_data *data = malloc(sizeof(struct quirc_data));
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate memory for quirc_data");
        vTaskDelete(NULL);
        return;
    }

    while (xQueueReceive(qr_frame_queue, &frame_data, portMAX_DELAY) == pdTRUE) {
        if (!qr_decoder || !grayscale_buffer) {
            free(frame_data.frame_data);
            continue;
        }

        // Calculate ROI (center portion of image)
        uint32_t roi_size_percent = QR_DECODE_ROI_PERCENT;
        uint32_t roi_width = (frame_data.width * roi_size_percent) / 100;
        uint32_t roi_height = (frame_data.height * roi_size_percent) / 100;
        uint32_t roi_x = (frame_data.width - roi_width) / 2;
        uint32_t roi_y = (frame_data.height - roi_height) / 2;

        // Calculate downsampled dimensions
        uint32_t decode_width = roi_width / QR_DECODE_SCALE_FACTOR;
        uint32_t decode_height = roi_height / QR_DECODE_SCALE_FACTOR;

        // Convert RGB565 to grayscale with downsampling
        rgb565_to_grayscale_downsample(frame_data.frame_data, grayscale_buffer, 
                                      frame_data.width, frame_data.height,
                                      decode_width, decode_height,
                                      roi_x, roi_y, roi_width, roi_height);

        // Get quirc image buffer
        uint8_t *quirc_buf;
        int quirc_width, quirc_height;
        quirc_buf = quirc_begin(qr_decoder, &quirc_width, &quirc_height);

        if (quirc_buf && quirc_width == decode_width && quirc_height == decode_height) {
            // Direct copy when dimensions match
            memcpy(quirc_buf, grayscale_buffer, decode_width * decode_height);

            // End quirc processing
            quirc_end(qr_decoder);

            // Identify QR codes
            int num_codes = quirc_count(qr_decoder);
            if (num_codes > 0) {
                ESP_LOGD(TAG, "Found %d QR codes", num_codes);
            }

            for (int i = 0; i < num_codes; i++) {
                struct quirc_code code;

                quirc_extract(qr_decoder, i, &code);
                quirc_decode_error_t err = quirc_decode(&code, data);

                if (err == QUIRC_SUCCESS) {
                    ESP_LOGI(TAG, "QR Code decoded: %s", data->payload);

                    // Update UI on main thread
                    if (bsp_display_lock(100)) {
                        if (qr_result_label) {
                            char result_text[256];
                            snprintf(result_text, sizeof(result_text), "QR: %.200s", (char*)data->payload);
                            lv_label_set_text(qr_result_label, result_text);
                        }
                        bsp_display_unlock();
                    }
                } else {
                    ESP_LOGD(TAG, "QR decode error: %d", err);
                }
            }
        } else {
            ESP_LOGW(TAG, "Quirc buffer size mismatch: expected %dx%d, got %dx%d",
                     decode_width, decode_height, quirc_width, quirc_height);
        }

        free(frame_data.frame_data);

        // Yield to watchdog
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Clean up allocated memory
    free(data);

    ESP_LOGI(TAG, "QR decode task ending");
    vTaskDelete(NULL);
}

// Initialize QR decoder
static bool qr_decoder_init(uint32_t width, uint32_t height)
{
    // Calculate actual decode dimensions based on ROI and downsampling
    uint32_t roi_width = (width * QR_DECODE_ROI_PERCENT) / 100;
    uint32_t roi_height = (height * QR_DECODE_ROI_PERCENT) / 100;
    uint32_t decode_width = roi_width / QR_DECODE_SCALE_FACTOR;
    uint32_t decode_height = roi_height / QR_DECODE_SCALE_FACTOR;

    ESP_LOGI(TAG, "Initializing QR decoder: source %dx%d -> decode %dx%d", 
             width, height, decode_width, decode_height);

    // Create quirc decoder instance
    qr_decoder = quirc_new();
    if (!qr_decoder) {
        ESP_LOGE(TAG, "Failed to create quirc decoder");
        return false;
    }

    // Resize quirc to match decode resolution (smaller)
    if (quirc_resize(qr_decoder, decode_width, decode_height) < 0) {
        ESP_LOGE(TAG, "Failed to resize quirc decoder to %dx%d", decode_width, decode_height);
        quirc_destroy(qr_decoder);
        qr_decoder = NULL;
        return false;
    }

    // Allocate grayscale conversion buffer for downsampled size
    size_t gray_buffer_size = decode_width * decode_height;
    grayscale_buffer = malloc(gray_buffer_size);
    if (!grayscale_buffer) {
        ESP_LOGE(TAG, "Failed to allocate grayscale buffer");
        quirc_destroy(qr_decoder);
        qr_decoder = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Allocated grayscale buffer: %zu bytes", gray_buffer_size);

    // Create frame queue for QR processing
    qr_frame_queue = xQueueCreate(QR_FRAME_QUEUE_SIZE, sizeof(qr_frame_data_t));
    if (!qr_frame_queue) {
        ESP_LOGE(TAG, "Failed to create QR frame queue");
        free(grayscale_buffer);
        quirc_destroy(qr_decoder);
        grayscale_buffer = NULL;
        qr_decoder = NULL;
        return false;
    }

    // Create QR decode mutex
    qr_decode_mutex = xSemaphoreCreateMutex();
    if (!qr_decode_mutex) {
        ESP_LOGE(TAG, "Failed to create QR decode mutex");
        vQueueDelete(qr_frame_queue);
        free(grayscale_buffer);
        quirc_destroy(qr_decoder);
        qr_frame_queue = NULL;
        grayscale_buffer = NULL;
        qr_decoder = NULL;
        return false;
    }

    // Create QR decode task with increased stack
    BaseType_t task_result = xTaskCreate(qr_decode_task, "qr_decode", 
                                        QR_DECODE_TASK_STACK_SIZE, NULL, 
                                        QR_DECODE_TASK_PRIORITY, &qr_decode_task_handle);
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create QR decode task");
        vSemaphoreDelete(qr_decode_mutex);
        vQueueDelete(qr_frame_queue);
        free(grayscale_buffer);
        quirc_destroy(qr_decoder);
        qr_decode_mutex = NULL;
        qr_frame_queue = NULL;
        grayscale_buffer = NULL;
        qr_decoder = NULL;
        return false;
    }

    ESP_LOGI(TAG, "QR decoder initialized successfully");
    return true;
}

// Cleanup QR decoder
static void qr_decoder_cleanup(void)
{
    // Stop decode task
    if (qr_decode_task_handle) {
        vTaskDelete(qr_decode_task_handle);
        qr_decode_task_handle = NULL;
    }

    // Clean up queue
    if (qr_frame_queue) {
        // Drain queue
        qr_frame_data_t frame_data;
        while (xQueueReceive(qr_frame_queue, &frame_data, 0) == pdTRUE) {
            free(frame_data.frame_data);
        }
        vQueueDelete(qr_frame_queue);
        qr_frame_queue = NULL;
    }

    // Clean up mutex
    if (qr_decode_mutex) {
        vSemaphoreDelete(qr_decode_mutex);
        qr_decode_mutex = NULL;
    }

    // Clean up buffers and decoder
    if (grayscale_buffer) {
        free(grayscale_buffer);
        grayscale_buffer = NULL;
    }

    if (qr_decoder) {
        quirc_destroy(qr_decoder);
        qr_decoder = NULL;
    }

    ESP_LOGI(TAG, "QR decoder cleanup completed");
}
