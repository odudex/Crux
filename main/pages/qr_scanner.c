/*
 * QR Scanner Page
 * Captures camera frames and decodes QR codes in real-time
 */

#include "qr_scanner.h"
#include "../ui_components/tron_theme.h"
#include <esp_lcd_touch_gt911.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>
#include <quirc.h>
#include <stdlib.h>
#include <string.h>

#define CAMERA_SCREEN_WIDTH 720
#define CAMERA_SCREEN_HEIGHT 640
#define QR_FRAME_QUEUE_SIZE 1
#define QR_DECODE_INTERVAL_US 100000
#define QR_DECODE_TASK_STACK_SIZE 16384
#define QR_DECODE_TASK_PRIORITY 5
#define QR_DECODE_SCALE_FACTOR 2

// RGB565 bit depth constants
#define RGB565_RED_BITS 5
#define RGB565_GREEN_BITS 6
#define RGB565_BLUE_BITS 5
#define RGB565_RED_LEVELS (1 << RGB565_RED_BITS)     // 32
#define RGB565_GREEN_LEVELS (1 << RGB565_GREEN_BITS) // 64
#define RGB565_BLUE_LEVELS (1 << RGB565_BLUE_BITS)   // 32

typedef enum {
  CAMERA_EVENT_TASK_RUN = BIT(0),
  CAMERA_EVENT_DELETE = BIT(1),
} camera_event_id_t;

typedef struct {
  uint8_t *frame_data;
  uint32_t width;
  uint32_t height;
  size_t data_len;
} qr_frame_data_t;

static const char *TAG = "QR_SCANNER";

// UI components
static lv_obj_t *qr_scanner_screen = NULL;
static lv_obj_t *camera_img = NULL;
static lv_obj_t *qr_result_label = NULL;
static void (*return_callback)(void) = NULL;

// Video system
static int _camera_ctlr_handle = -1;
static lv_img_dsc_t _img_refresh_dsc;
static bool video_system_initialized = false;
static EventGroupHandle_t camera_event_group = NULL;

// Display buffers
static uint8_t *display_buffer_a = NULL;
static uint8_t *display_buffer_b = NULL;
static uint8_t *current_display_buffer = NULL;
static size_t display_buffer_size = 0;
static volatile bool buffer_swap_needed = false;

// QR decoder
static struct quirc *qr_decoder = NULL;
static TaskHandle_t qr_decode_task_handle = NULL;
static QueueHandle_t qr_frame_queue = NULL;

// RGB565 to grayscale conversion tables (const arrays in flash)
static const uint8_t r5_to_gray[RGB565_RED_LEVELS] = {
    0,  2,  4,  7,  9,  12, 14, 17, 19, 22, 24, 27, 29, 31, 34, 36,
    39, 41, 44, 46, 49, 51, 53, 56, 58, 61, 63, 66, 68, 71, 73, 76};

static const uint8_t g6_to_gray[RGB565_GREEN_LEVELS] = {
    0,   2,   4,   7,   9,   11,  14,  16,  18,  21,  23,  25,  28,
    30,  32,  35,  37,  39,  42,  44,  46,  49,  51,  53,  56,  58,
    60,  63,  65,  67,  70,  72,  74,  77,  79,  81,  84,  86,  88,
    91,  93,  95,  98,  100, 102, 105, 107, 109, 112, 114, 116, 119,
    121, 123, 126, 128, 130, 133, 135, 137, 140, 142, 144, 147};

static const uint8_t b5_to_gray[RGB565_BLUE_LEVELS] = {
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29};

// Add after existing static variables
static volatile bool closing = false;

// Function declarations
static void touch_event_cb(lv_event_t *e);
static void camera_video_frame_operation(uint8_t *camera_buf,
                                         uint8_t camera_buf_index,
                                         uint32_t camera_buf_hes,
                                         uint32_t camera_buf_ves,
                                         size_t camera_buf_len);
static void horizontal_crop_cam_to_display(const uint8_t *camera_buf,
                                           uint8_t *display_buf,
                                           uint32_t camera_width,
                                           uint32_t camera_height,
                                           uint32_t display_width);
static bool allocate_display_buffers(uint32_t width, uint32_t height);
static void free_display_buffers(void);
static void rgb565_to_grayscale_downsample(const uint8_t *rgb565_data,
                                           uint8_t *gray_data,
                                           uint32_t src_width,
                                           uint32_t src_height);
static void qr_decode_task(void *pvParameters);
static bool qr_decoder_init(uint32_t width, uint32_t height);
static void qr_decoder_cleanup(void);
static bool camera_run(void);
static void camera_init(void);

// Utility functions
static void safe_free_and_null(uint8_t **ptr) {
  if (*ptr) {
    free(*ptr);
    *ptr = NULL;
  }
}

// UI Event handlers
static void touch_event_cb(lv_event_t *e) {
  if (closing)
    return;
  else
    closing = true;
  ESP_LOGI(TAG, "Screen touched, returning to previous page");
  if (return_callback) {
    return_callback();
  }
}

// Display buffer management
static uint8_t *allocate_buffer_with_fallback(size_t size) {
  uint8_t *buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!buffer) {
    buffer = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  return buffer;
}

static bool allocate_display_buffers(uint32_t width, uint32_t height) {
  display_buffer_size = width * height * 2;

  display_buffer_a = allocate_buffer_with_fallback(display_buffer_size);
  if (!display_buffer_a) {
    ESP_LOGE(TAG, "Failed to allocate display buffer A");
    display_buffer_size = 0;
    return false;
  }

  display_buffer_b = allocate_buffer_with_fallback(display_buffer_size);
  if (!display_buffer_b) {
    ESP_LOGE(TAG, "Failed to allocate display buffer B");
    safe_free_and_null(&display_buffer_a);
    display_buffer_size = 0;
    return false;
  }

  return true;
}

static void free_display_buffers(void) {
  current_display_buffer = NULL;
  safe_free_and_null(&display_buffer_a);
  safe_free_and_null(&display_buffer_b);
  display_buffer_size = 0;
}

// RGB565 to grayscale conversion with downsampling
static void rgb565_to_grayscale_downsample(const uint8_t *rgb565_data,
                                           uint8_t *gray_data,
                                           uint32_t src_width,
                                           uint32_t src_height) {

  // TODO: Evaluate hardware accelerated downscale and color conversion
  // Could Pixel-Processing Accelerator (PPA) scaling output directly to YUV,
  // where 8 bit grayscale buffer could be obtained directly?
  const uint16_t *pixels = (const uint16_t *)rgb565_data;
  uint32_t dst_width = src_width / QR_DECODE_SCALE_FACTOR;
  uint32_t dst_height = src_height / QR_DECODE_SCALE_FACTOR;

  for (uint32_t dst_y = 0; dst_y < dst_height; dst_y++) {
    for (uint32_t dst_x = 0; dst_x < dst_width; dst_x++) {
      uint32_t src_x = dst_x * QR_DECODE_SCALE_FACTOR;
      uint32_t src_y = dst_y * QR_DECODE_SCALE_FACTOR;

      if (src_x >= src_width)
        src_x = src_width - 1;
      if (src_y >= src_height)
        src_y = src_height - 1;

      uint32_t src_idx = src_y * src_width + src_x;
      uint16_t pixel = pixels[src_idx];

      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5) & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      gray_data[dst_y * dst_width + dst_x] =
          r5_to_gray[r5] + g6_to_gray[g6] + b5_to_gray[b5];
    }
  }
}

// QR decode task
static void qr_decode_task(void *pvParameters) {
  qr_frame_data_t frame_data;
  struct quirc_data *data = malloc(sizeof(struct quirc_data));

  if (!data) {
    ESP_LOGE(TAG, "Failed to allocate memory for quirc_data");
    vTaskDelete(NULL);
    return;
  }

  while (xQueueReceive(qr_frame_queue, &frame_data, portMAX_DELAY) == pdTRUE) {
    if (closing) {
      break;
    }

    uint8_t *quirc_buf;
    int quirc_width, quirc_height;
    quirc_buf = quirc_begin(qr_decoder, &quirc_width, &quirc_height);

    if (quirc_buf) {
      // Convert RGB565 directly to quirc buffer
      rgb565_to_grayscale_downsample(frame_data.frame_data, quirc_buf,
                                     frame_data.width, frame_data.height);
      quirc_end(qr_decoder);

      int num_codes = quirc_count(qr_decoder);
      for (int i = 0; i < num_codes; i++) {
        struct quirc_code code;
        quirc_extract(qr_decoder, i, &code);
        quirc_decode_error_t err = quirc_decode(&code, data);

        if (err == QUIRC_SUCCESS) {
          ESP_LOGI(TAG, "QR Code decoded: %s", data->payload);

          if (bsp_display_lock(100)) {
            if (qr_result_label) {
              char result_text[256];
              snprintf(result_text, sizeof(result_text), "QR: %.200s",
                       (char *)data->payload);
              // lv_label_set_text(qr_result_label, result_text);
            }
            bsp_display_unlock();
          }
        }
      }
    }
  }

  free(data);
  vTaskDelete(NULL);
}

// QR decoder initialization and cleanup
static bool qr_decoder_init(uint32_t width, uint32_t height) {
  uint32_t decode_width = width / QR_DECODE_SCALE_FACTOR;
  uint32_t decode_height = height / QR_DECODE_SCALE_FACTOR;

  qr_decoder = quirc_new();
  if (!qr_decoder) {
    ESP_LOGE(TAG, "Failed to create quirc decoder");
    return false;
  }

  if (quirc_resize(qr_decoder, decode_width, decode_height) < 0) {
    ESP_LOGE(TAG, "Failed to resize quirc decoder");
    quirc_destroy(qr_decoder);
    qr_decoder = NULL;
    return false;
  }

  qr_frame_queue = xQueueCreate(QR_FRAME_QUEUE_SIZE, sizeof(qr_frame_data_t));
  if (!qr_frame_queue) {
    ESP_LOGE(TAG, "Failed to create QR frame queue");
    quirc_destroy(qr_decoder);
    qr_decoder = NULL;
    return false;
  }

  BaseType_t task_result =
      xTaskCreate(qr_decode_task, "qr_decode", QR_DECODE_TASK_STACK_SIZE, NULL,
                  QR_DECODE_TASK_PRIORITY, &qr_decode_task_handle);
  if (task_result != pdPASS) {
    ESP_LOGE(TAG, "Failed to create QR decode task");
    vQueueDelete(qr_frame_queue);
    quirc_destroy(qr_decoder);
    qr_frame_queue = NULL;
    qr_decoder = NULL;
    return false;
  }

  return true;
}

static void qr_decoder_cleanup(void) {
  if (qr_decode_task_handle) {
    vTaskDelete(qr_decode_task_handle);
    qr_decode_task_handle = NULL;
  }

  if (qr_frame_queue) {
    qr_frame_data_t frame_data;
    while (xQueueReceive(qr_frame_queue, &frame_data, 0) == pdTRUE) {
    }
    vQueueDelete(qr_frame_queue);
    qr_frame_queue = NULL;
  }

  if (qr_decoder) {
    quirc_destroy(qr_decoder);
    qr_decoder = NULL;
  }
}

// Camera video processing
static void camera_video_frame_operation(uint8_t *camera_buf,
                                         uint8_t camera_buf_index,
                                         uint32_t camera_buf_hes,
                                         uint32_t camera_buf_ves,
                                         size_t camera_buf_len) {
  if (closing) {
    return;
  }

  EventBits_t current_bits = xEventGroupGetBits(camera_event_group);
  if (!(current_bits & CAMERA_EVENT_TASK_RUN) ||
      (current_bits & CAMERA_EVENT_DELETE)) {
    return;
  }

  uint8_t *back_buffer = (current_display_buffer == display_buffer_a)
                             ? display_buffer_b
                             : display_buffer_a;

  // Crop from 800x640 camera to 720x640 display
  horizontal_crop_cam_to_display(camera_buf, back_buffer, camera_buf_hes,
                                 camera_buf_ves, CAMERA_SCREEN_WIDTH);
  buffer_swap_needed = true;

  if (buffer_swap_needed && !closing && camera_img && bsp_display_lock(0)) {
    current_display_buffer = back_buffer;
    _img_refresh_dsc.data = current_display_buffer;

    lv_img_set_src(camera_img, &_img_refresh_dsc);
    lv_refr_now(NULL);

    buffer_swap_needed = false;
    bsp_display_unlock();
  }

  // QR processing uses the display buffer (what user sees on screen)
  static int64_t last_qr_analysis = 0;
  int64_t current_time = esp_timer_get_time();
  if (qr_frame_queue &&
      current_time - last_qr_analysis > QR_DECODE_INTERVAL_US) {
    last_qr_analysis = current_time;

    qr_frame_data_t dummy;
    while (xQueueReceive(qr_frame_queue, &dummy, 0) == pdTRUE) {
    }

    qr_frame_data_t frame_data = {.frame_data = current_display_buffer,
                                  .width = CAMERA_SCREEN_WIDTH,
                                  .height = CAMERA_SCREEN_HEIGHT,
                                  .data_len = CAMERA_SCREEN_WIDTH *
                                              CAMERA_SCREEN_HEIGHT * 2};

    xQueueSend(qr_frame_queue, &frame_data, 0);
  }
}

static void horizontal_crop_cam_to_display(const uint8_t *camera_buf,
                                           uint8_t *display_buf,
                                           uint32_t camera_width,
                                           uint32_t camera_height,
                                           uint32_t display_width) {
  // Center crop: skip (camera_width - display_width) / 2 pixels from each side
  uint32_t crop_offset = (camera_width - display_width) / 2;
  const uint16_t *src = (const uint16_t *)camera_buf;
  uint16_t *dst = (uint16_t *)display_buf;

  for (uint32_t y = 0; y < camera_height; y++) {
    const uint16_t *src_row = src + (y * camera_width) + crop_offset;
    uint16_t *dst_row = dst + (y * display_width);
    memcpy(dst_row, src_row, display_width * 2); // 2 bytes per pixel
  }
}

// Camera system initialization
static void camera_init(void) {
  if (video_system_initialized) {
    return;
  }

  camera_event_group = xEventGroupCreate();
  if (!camera_event_group) {
    ESP_LOGE(TAG, "Failed to create camera event group");
    return;
  }

  xEventGroupSetBits(camera_event_group, CAMERA_EVENT_TASK_RUN);

  i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
  if (!i2c_handle) {
    ESP_LOGE(TAG, "Failed to get I2C bus handle");
    return;
  }

  esp_err_t err = app_video_main(i2c_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize camera: %s", esp_err_to_name(err));
    return;
  }

  video_system_initialized = true;

  _camera_ctlr_handle =
      app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT_RGB565);
  if (_camera_ctlr_handle < 0) {
    ESP_LOGE(TAG, "Failed to open camera device");
    return;
  }

  ESP_ERROR_CHECK(
      app_video_register_frame_operation_cb(camera_video_frame_operation));

  lv_img_dsc_t img_dsc = {
      .header = {.cf = LV_COLOR_FORMAT_RGB565,
                 .w = CAMERA_SCREEN_WIDTH,
                 .h = CAMERA_SCREEN_HEIGHT},
      .data_size = CAMERA_SCREEN_WIDTH * CAMERA_SCREEN_HEIGHT * 2,
      .data = NULL,
  };
  memcpy(&_img_refresh_dsc, &img_dsc, sizeof(lv_img_dsc_t));

  if (!allocate_display_buffers(CAMERA_SCREEN_WIDTH, CAMERA_SCREEN_HEIGHT)) {
    ESP_LOGE(TAG, "Failed to allocate display buffers");
    return;
  }

  current_display_buffer = display_buffer_a;
  _img_refresh_dsc.data = current_display_buffer;

  ESP_ERROR_CHECK(
      app_video_set_bufs(_camera_ctlr_handle, EXAMPLE_CAM_BUF_NUM, NULL));

  esp_err_t start_err = app_video_stream_task_start(_camera_ctlr_handle, 0);
  if (start_err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start camera stream task: %s",
             esp_err_to_name(start_err));
    return;
  }

  if (!qr_decoder_init(CAMERA_SCREEN_WIDTH,
                       CAMERA_SCREEN_HEIGHT)) { // Initialize QR decoder with
                                                // display dimensions
    ESP_LOGE(TAG, "Failed to initialize QR decoder");
  }
}

static bool camera_run(void) {
  if (_camera_ctlr_handle >= 0 && video_system_initialized) {
    return true;
  }
  camera_init();
  return true;
}

// Public API functions
void qr_scanner_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object for QR scanner page");
    return;
  }

  return_callback = return_cb;

  qr_scanner_screen = lv_obj_create(parent);
  lv_obj_set_size(qr_scanner_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(qr_scanner_screen, lv_color_hex(0x1e1e1e), 0);
  lv_obj_set_style_border_width(qr_scanner_screen, 0, 0);
  lv_obj_set_style_pad_all(qr_scanner_screen, 0, 0);
  lv_obj_set_style_radius(qr_scanner_screen, 0, 0);
  lv_obj_set_style_shadow_width(qr_scanner_screen, 0, 0);
  lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(qr_scanner_screen, touch_event_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *frame_buffer = lv_obj_create(qr_scanner_screen);
  lv_obj_set_size(frame_buffer, CAMERA_SCREEN_WIDTH, CAMERA_SCREEN_HEIGHT);
  lv_obj_center(frame_buffer);
  lv_obj_set_style_bg_opa(frame_buffer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(frame_buffer, 0, 0);
  lv_obj_set_style_radius(frame_buffer, 0, 0);
  lv_obj_clear_flag(frame_buffer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(frame_buffer, touch_event_cb, LV_EVENT_CLICKED, NULL);

  camera_img = lv_img_create(frame_buffer);
  lv_obj_set_size(camera_img, CAMERA_SCREEN_WIDTH, CAMERA_SCREEN_HEIGHT);
  lv_obj_center(camera_img);
  lv_obj_clear_flag(camera_img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(camera_img, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(camera_img, LV_OPA_COVER, 0);

  lv_obj_t *title_label =
      tron_theme_create_label(qr_scanner_screen, "QR Scanner", false);
  tron_theme_apply_label(title_label, true);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

  qr_result_label = tron_theme_create_label(qr_scanner_screen,
                                            "Point camera at QR code", false);
  tron_theme_apply_label(qr_result_label, true);
  lv_obj_align(qr_result_label, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_align(qr_result_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(qr_result_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(qr_result_label, LV_PCT(90));

  if (!camera_run()) {
    ESP_LOGE(TAG, "Failed to initialize camera");
  }
}

void qr_scanner_page_show(void) {
  if (!closing && qr_scanner_screen) {
    lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_scanner_page_hide(void) {
  if (!closing && qr_scanner_screen) {
    lv_obj_add_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_scanner_page_destroy(void) {

  // Step 1: Stop camera operations first
  if (camera_event_group) {
    xEventGroupClearBits(camera_event_group, CAMERA_EVENT_TASK_RUN);
    xEventGroupSetBits(camera_event_group, CAMERA_EVENT_DELETE);
  }

  // Step 2: Stop camera stream and close camera handle
  if (_camera_ctlr_handle >= 0) {
    app_video_stream_task_stop(_camera_ctlr_handle);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for stream to stop

    app_video_close(_camera_ctlr_handle);
    _camera_ctlr_handle = -1;
  }

  // Step 3: Clean up QR decoder (this will stop the task and free resources)
  qr_decoder_cleanup();

  // Step 4: Clean up UI references before deleting objects
  camera_img = NULL;
  qr_result_label = NULL;

  // Step 5: Clean up display buffers
  free_display_buffers();

  // Step 6: Deinitialize video system
  if (video_system_initialized) {
    app_video_deinit();
    video_system_initialized = false;
  }

  // Step 7: Clean up UI objects
  if (qr_scanner_screen) {
    lv_obj_del(qr_scanner_screen);
    qr_scanner_screen = NULL;
  }

  // Step 8: Clean up event group last
  if (camera_event_group) {
    vEventGroupDelete(camera_event_group);
    camera_event_group = NULL;
  }

  // Step 9: Reset state variables
  return_callback = NULL;
  buffer_swap_needed = false;
  closing = false;
}