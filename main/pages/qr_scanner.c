/*
 * QR Scanner Page
 * Captures camera frames and decodes QR codes in real-time
 */

#include "qr_scanner.h"
#include "../../components/cUR/src/ur_decoder.h"
#include "../ui_components/theme.h"
#include "../utils/memory_utils.h"
#include "../utils/qr_codes.h"
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

// Progress bar dimensions
#define PROGRESS_BAR_HEIGHT 20
#define PROGRESS_FRAME_PADD 2
#define PROGRESS_BLOC_PAD 1
#define MAX_QR_PARTS 100
#define DISPLAY_LOCK_TIMEOUT_MS 100

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
static lv_obj_t *progress_frame = NULL;
static lv_obj_t **progress_rectangles = NULL;
static int progress_rectangles_count = 0;
static lv_obj_t *ur_progress_bar = NULL;
static lv_obj_t *ur_progress_indicator = NULL;
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
static QRPartParser *qr_parser = NULL;
static int previously_parsed = -1;

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

// State management
static volatile bool closing = false;
static volatile bool scan_completed = false;
static volatile bool is_fully_initialized = false;
static volatile bool destruction_in_progress = false;
static volatile int active_frame_operations = 0;
static lv_timer_t *completion_timer = NULL;

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
static void create_progress_indicators(int total_parts);
static void update_progress_indicator(int part_index);
static void cleanup_progress_indicators(void);
static void create_ur_progress_bar(void);
static void update_ur_progress_bar(double percent_complete);
static void cleanup_ur_progress_bar(void);

static void create_progress_indicators(int total_parts) {
  if (total_parts <= 1 || total_parts > MAX_QR_PARTS || !qr_scanner_screen) {
    return;
  }

  if (!bsp_display_lock(DISPLAY_LOCK_TIMEOUT_MS)) {
    return;
  }

  int progress_frame_width = lv_obj_get_width(qr_scanner_screen) * 80 / 100;
  int rect_width = progress_frame_width / total_parts;
  rect_width -= PROGRESS_BLOC_PAD;
  progress_frame_width = total_parts * rect_width + 1;
  progress_frame_width += 2 * PROGRESS_FRAME_PADD + 2;

  progress_frame = lv_obj_create(qr_scanner_screen);
  lv_obj_set_size(progress_frame, progress_frame_width, PROGRESS_BAR_HEIGHT);
  lv_obj_align(progress_frame, LV_ALIGN_BOTTOM_MID, 0, -10);
  theme_apply_frame(progress_frame);
  lv_obj_set_style_pad_all(progress_frame, 2, 0);

  progress_rectangles = malloc(total_parts * sizeof(lv_obj_t *));
  if (!progress_rectangles) {
    ESP_LOGE(TAG, "Failed to allocate progress rectangles array");
    lv_obj_del(progress_frame);
    progress_frame = NULL;
    bsp_display_unlock();
    return;
  }
  progress_rectangles_count = total_parts;

  lv_obj_update_layout(progress_frame);

  for (int i = 0; i < total_parts; i++) {
    progress_rectangles[i] = lv_obj_create(progress_frame);
    lv_obj_set_size(progress_rectangles[i], rect_width - PROGRESS_BLOC_PAD, 12);
    lv_obj_set_pos(progress_rectangles[i], i * rect_width, 0);
    theme_apply_solid_rectangle(progress_rectangles[i]);
  }

  bsp_display_unlock();
}

static void update_progress_indicator(int part_index) {
  if (!progress_rectangles || part_index < 0 ||
      part_index >= progress_rectangles_count) {
    return;
  }

  if (previously_parsed != part_index &&
      bsp_display_lock(DISPLAY_LOCK_TIMEOUT_MS)) {
    lv_obj_set_style_bg_color(progress_rectangles[part_index],
                              highlight_color(), 0);
    if (previously_parsed >= 0) {
      lv_obj_set_style_bg_color(progress_rectangles[previously_parsed],
                                main_color(), 0);
    }
    previously_parsed = part_index;
    bsp_display_unlock();
  }
}

static void cleanup_progress_indicators(void) {
  SAFE_FREE_STATIC(progress_rectangles);
  progress_rectangles_count = 0;
  progress_frame = NULL;
  previously_parsed = -1;
}

static void create_ur_progress_bar(void) {
  if (!qr_scanner_screen || ur_progress_bar) {
    return;
  }

  if (!bsp_display_lock(DISPLAY_LOCK_TIMEOUT_MS)) {
    return;
  }

  int bar_width = lv_obj_get_width(qr_scanner_screen) * 80 / 100;
  int bar_height = PROGRESS_BAR_HEIGHT;

  // Create frame container
  ur_progress_bar = lv_obj_create(qr_scanner_screen);
  lv_obj_set_size(ur_progress_bar, bar_width, bar_height);
  lv_obj_align(ur_progress_bar, LV_ALIGN_BOTTOM_MID, 0, -10);
  theme_apply_frame(ur_progress_bar);
  lv_obj_set_style_pad_all(ur_progress_bar, 2, 0);

  // Create progress indicator inside frame
  ur_progress_indicator = lv_obj_create(ur_progress_bar);
  lv_obj_set_size(ur_progress_indicator, 0, 12); // Start with 0 width
  lv_obj_set_pos(ur_progress_indicator, 0, 0);
  theme_apply_solid_rectangle(ur_progress_indicator);
  lv_obj_set_style_bg_color(ur_progress_indicator, highlight_color(), 0);

  bsp_display_unlock();
}

static void update_ur_progress_bar(double percent_complete) {
  if (!ur_progress_bar || !ur_progress_indicator) {
    return;
  }

  if (!bsp_display_lock(DISPLAY_LOCK_TIMEOUT_MS)) {
    return;
  }

  // Calculate indicator width based on percentage (0.0 to 1.0)
  int bar_inner_width =
      lv_obj_get_width(ur_progress_bar) - 4; // Account for padding
  int indicator_width = (int)(bar_inner_width * percent_complete);

  // Clamp to valid range
  if (indicator_width < 0)
    indicator_width = 0;
  if (indicator_width > bar_inner_width)
    indicator_width = bar_inner_width;

  lv_obj_set_width(ur_progress_indicator, indicator_width);

  bsp_display_unlock();
}

static void cleanup_ur_progress_bar(void) {
  ur_progress_bar = NULL;
  ur_progress_indicator = NULL;
}

static void completion_timer_cb(lv_timer_t *timer) {
  // Only trigger callback if not already destroying and scan is complete
  if (scan_completed && return_callback && !closing &&
      !destruction_in_progress) {
    closing = true; // Prevent any new operations
    lv_timer_del(completion_timer);
    completion_timer = NULL;

    // Ensure camera and decoder have stopped before callback
    if (camera_event_group) {
      xEventGroupClearBits(camera_event_group, CAMERA_EVENT_TASK_RUN);
    }

    // Small delay to let any in-flight operations complete
    vTaskDelay(pdMS_TO_TICKS(50));

    return_callback();
  }
}

static void touch_event_cb(lv_event_t *e) {
  if (closing)
    return;
  closing = true;
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
    SAFE_FREE_STATIC(display_buffer_a);
    display_buffer_size = 0;
    return false;
  }

  return true;
}

static void free_display_buffers(void) {
  current_display_buffer = NULL;
  SAFE_FREE_STATIC(display_buffer_a);
  SAFE_FREE_STATIC(display_buffer_b);
  display_buffer_size = 0;
}

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

static void qr_decode_task(void *pvParameters) {
  qr_frame_data_t frame_data;
  struct quirc_data *data = malloc(sizeof(struct quirc_data));

  if (!data) {
    ESP_LOGE(TAG, "Failed to allocate memory for quirc_data");
    vTaskDelete(NULL);
    return;
  }

  while (true) {
    // Check closing flag before waiting for queue
    if (closing || destruction_in_progress) {
      break;
    }

    if (xQueueReceive(qr_frame_queue, &frame_data, pdMS_TO_TICKS(100)) !=
        pdTRUE) {
      continue; // Timeout, check flags again
    }

    // Double-check after receiving
    if (closing || destruction_in_progress) {
      break;
    }

    uint8_t *quirc_buf;
    int quirc_width, quirc_height;
    quirc_buf = quirc_begin(qr_decoder, &quirc_width, &quirc_height);

    if (quirc_buf) {
      rgb565_to_grayscale_downsample(frame_data.frame_data, quirc_buf,
                                     frame_data.width, frame_data.height);
      quirc_end(qr_decoder);

      int num_codes = quirc_count(qr_decoder);
      for (int i = 0; i < num_codes; i++) {
        if (closing || destruction_in_progress) {
          break;
        }

        struct quirc_code code;
        quirc_extract(qr_decoder, i, &code);
        quirc_decode_error_t err = quirc_decode(&code, data);

        if (err == QUIRC_SUCCESS && qr_parser) {
          int part_index = qr_parser_parse_with_len(
              qr_parser, (const char *)data->payload, data->payload_len);
          if (part_index >= 0 || qr_parser->total == 1) {
            // Handle pMofN progress indicators
            if (qr_parser->format == FORMAT_PMOFN) {
              if (qr_parser->total > 1 && !progress_frame) {
                create_progress_indicators(qr_parser->total);
              }

              if (part_index >= 0 && qr_parser->total > 1) {
                update_progress_indicator(part_index);
              }
            }
            // Handle UR progress bar
            else if (qr_parser->format == FORMAT_UR && qr_parser->ur_decoder) {
              if (!ur_progress_bar) {
                create_ur_progress_bar();
              }

              double percent_complete = ur_decoder_estimated_percent_complete(
                  (ur_decoder_t *)qr_parser->ur_decoder);
              update_ur_progress_bar(percent_complete);
            }

            if (qr_parser_is_complete(qr_parser)) {
              scan_completed = true;
              // Stop processing more codes once complete
              break;
            }
          }
        }
      }
    }
  }

  free(data);
  vTaskDelete(NULL);
}

static bool qr_decoder_init(uint32_t width, uint32_t height) {
  uint32_t decode_width = width / QR_DECODE_SCALE_FACTOR;
  uint32_t decode_height = height / QR_DECODE_SCALE_FACTOR;

  qr_decoder = quirc_new();
  if (!qr_decoder) {
    ESP_LOGE(TAG, "Failed to create quirc decoder");
    goto error;
  }

  if (quirc_resize(qr_decoder, decode_width, decode_height) < 0) {
    ESP_LOGE(TAG, "Failed to resize quirc decoder");
    goto error;
  }

  qr_frame_queue = xQueueCreate(QR_FRAME_QUEUE_SIZE, sizeof(qr_frame_data_t));
  if (!qr_frame_queue) {
    ESP_LOGE(TAG, "Failed to create QR frame queue");
    goto error;
  }

  BaseType_t task_result =
      xTaskCreate(qr_decode_task, "qr_decode", QR_DECODE_TASK_STACK_SIZE, NULL,
                  QR_DECODE_TASK_PRIORITY, &qr_decode_task_handle);
  if (task_result != pdPASS) {
    ESP_LOGE(TAG, "Failed to create QR decode task");
    goto error;
  }

  qr_parser = qr_parser_create();
  if (!qr_parser) {
    ESP_LOGE(TAG, "Failed to create QR parser");
    goto error;
  }
  return true;

error:
  if (qr_parser) {
    qr_parser_destroy(qr_parser);
    qr_parser = NULL;
  }
  if (qr_decode_task_handle) {
    vTaskDelete(qr_decode_task_handle);
    qr_decode_task_handle = NULL;
  }
  if (qr_frame_queue) {
    vQueueDelete(qr_frame_queue);
    qr_frame_queue = NULL;
  }
  if (qr_decoder) {
    quirc_destroy(qr_decoder);
    qr_decoder = NULL;
  }
  return false;
}

static void qr_decoder_cleanup(void) {
  // Signal decode task to stop
  closing = true;

  // Wait for decode task to finish (with timeout)
  if (qr_decode_task_handle) {
    int wait_count = 0;
    const int max_wait_ms = 500;
    const int check_interval_ms = 10;

    while (eTaskGetState(qr_decode_task_handle) != eDeleted &&
           wait_count < (max_wait_ms / check_interval_ms)) {
      vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
      wait_count++;
    }

    if (eTaskGetState(qr_decode_task_handle) != eDeleted) {
      ESP_LOGW(TAG, "Force deleting QR decode task");
      vTaskDelete(qr_decode_task_handle);
    }
    qr_decode_task_handle = NULL;
  }

  // Now safe to delete queue and decoder
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

  if (qr_parser) {
    qr_parser_destroy(qr_parser);
    qr_parser = NULL;
  }
}

static void camera_video_frame_operation(uint8_t *camera_buf,
                                         uint8_t camera_buf_index,
                                         uint32_t camera_buf_hes,
                                         uint32_t camera_buf_ves,
                                         size_t camera_buf_len) {
  if (closing || !is_fully_initialized || !camera_event_group) {
    return;
  }

  EventBits_t current_bits = xEventGroupGetBits(camera_event_group);
  if (!(current_bits & CAMERA_EVENT_TASK_RUN) ||
      (current_bits & CAMERA_EVENT_DELETE)) {
    return;
  }

  __atomic_add_fetch(&active_frame_operations, 1, __ATOMIC_SEQ_CST);

  if (!display_buffer_a || !display_buffer_b || !current_display_buffer) {
    __atomic_sub_fetch(&active_frame_operations, 1, __ATOMIC_SEQ_CST);
    return;
  }

  uint8_t *back_buffer = (current_display_buffer == display_buffer_a)
                             ? display_buffer_b
                             : display_buffer_a;

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

  __atomic_sub_fetch(&active_frame_operations, 1, __ATOMIC_SEQ_CST);
}

static void horizontal_crop_cam_to_display(const uint8_t *camera_buf,
                                           uint8_t *display_buf,
                                           uint32_t camera_width,
                                           uint32_t camera_height,
                                           uint32_t display_width) {
  uint32_t crop_offset = (camera_width - display_width) / 2;
  const uint16_t *src = (const uint16_t *)camera_buf;
  uint16_t *dst = (uint16_t *)display_buf;

  for (uint32_t y = 0; y < camera_height; y++) {
    const uint16_t *src_row = src + (y * camera_width) + crop_offset;
    uint16_t *dst_row = dst + (y * display_width);
    memcpy(dst_row, src_row, display_width * 2);
  }
}

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

  _camera_ctlr_handle = app_video_open(CAM_DEV_PATH, APP_VIDEO_FMT_RGB565);
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

  ESP_ERROR_CHECK(app_video_set_bufs(_camera_ctlr_handle, CAM_BUF_NUM, NULL));

  esp_err_t start_err = app_video_stream_task_start(_camera_ctlr_handle, 0);
  if (start_err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start camera stream task: %s",
             esp_err_to_name(start_err));
    return;
  }

  if (!qr_decoder_init(CAMERA_SCREEN_WIDTH, CAMERA_SCREEN_HEIGHT)) {
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

void qr_scanner_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  (void)parent;

  return_callback = return_cb;
  closing = false;
  scan_completed = false;
  is_fully_initialized = false;
  active_frame_operations = 0;

  qr_scanner_screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(qr_scanner_screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(qr_scanner_screen, lv_color_hex(0x1e1e1e), 0);
  lv_obj_set_style_bg_opa(qr_scanner_screen, LV_OPA_COVER, 0);
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
  lv_obj_set_style_pad_all(frame_buffer, 0, 0);
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
      theme_create_label(qr_scanner_screen, "QR Scanner", false);
  theme_apply_label(title_label, true);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 8);

  if (!camera_run()) {
    ESP_LOGE(TAG, "Failed to initialize camera");
    return;
  }

  completion_timer = lv_timer_create(completion_timer_cb, 100, NULL);
  is_fully_initialized = true;
}

void qr_scanner_page_show(void) {
  if (is_fully_initialized && !closing && qr_scanner_screen) {
    lv_obj_clear_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_scanner_page_hide(void) {
  if (is_fully_initialized && !closing && qr_scanner_screen) {
    lv_obj_add_flag(qr_scanner_screen, LV_OBJ_FLAG_HIDDEN);
  }
}

void qr_scanner_page_destroy(void) {
  // Prevent any callbacks or operations from starting
  destruction_in_progress = true;
  closing = true;
  is_fully_initialized = false;

  // Stop completion timer first
  if (completion_timer) {
    lv_timer_del(completion_timer);
    completion_timer = NULL;
  }
  scan_completed = false;

  // Stop camera stream immediately
  if (camera_event_group) {
    xEventGroupClearBits(camera_event_group, CAMERA_EVENT_TASK_RUN);
    xEventGroupSetBits(camera_event_group, CAMERA_EVENT_DELETE);
  }

  // Wait for active frame operations to complete
  int wait_count = 0;
  const int max_wait_ms = 300;
  const int check_interval_ms = 10;
  while (__atomic_load_n(&active_frame_operations, __ATOMIC_SEQ_CST) > 0 &&
         wait_count < (max_wait_ms / check_interval_ms)) {
    vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
    wait_count++;
  }

  int remaining_ops =
      __atomic_load_n(&active_frame_operations, __ATOMIC_SEQ_CST);
  if (remaining_ops > 0) {
    ESP_LOGW(TAG, "Timeout waiting for frame operations (remaining: %d)",
             remaining_ops);
  }

  // Close camera handle (stops stream task)
  if (_camera_ctlr_handle >= 0) {
    app_video_stream_task_stop(_camera_ctlr_handle);
    vTaskDelay(pdMS_TO_TICKS(50)); // Let stream task finish
    app_video_close(_camera_ctlr_handle);
    _camera_ctlr_handle = -1;
  }

  // Clean up QR decoder (this waits for decode task)
  qr_decoder_cleanup();

  // Now safe to clean up UI
  if (bsp_display_lock(1000)) {
    camera_img = NULL;
    cleanup_progress_indicators();
    cleanup_ur_progress_bar();
    if (qr_scanner_screen) {
      lv_obj_del(qr_scanner_screen);
      qr_scanner_screen = NULL;
    }
    bsp_display_unlock();
  } else {
    ESP_LOGW(TAG, "Failed to lock display for UI cleanup");
    camera_img = NULL;
    cleanup_progress_indicators();
    cleanup_ur_progress_bar();
    if (qr_scanner_screen) {
      lv_obj_del(qr_scanner_screen);
      qr_scanner_screen = NULL;
    }
  }

  // Free buffers
  free_display_buffers();

  // Deinit video system
  if (video_system_initialized) {
    app_video_deinit();
    video_system_initialized = false;
  }

  // Clean up event group last
  if (camera_event_group) {
    vEventGroupDelete(camera_event_group);
    camera_event_group = NULL;
  }

  // Reset state
  return_callback = NULL;
  buffer_swap_needed = false;
  destruction_in_progress = false;
  closing = false;
  active_frame_operations = 0;
}

char *qr_scanner_get_completed_content(void) {
  return qr_scanner_get_completed_content_with_len(NULL);
}

char *qr_scanner_get_completed_content_with_len(size_t *content_len) {
  if (qr_parser && qr_parser_is_complete(qr_parser)) {
    size_t result_len;
    char *complete_result = qr_parser_result(qr_parser, &result_len);
    if (content_len) {
      *content_len = result_len;
    }
    return complete_result; // Caller must free this
  }
  if (content_len) {
    *content_len = 0;
  }
  return NULL;
}

bool qr_scanner_is_ready(void) { return is_fully_initialized && !closing; }

int qr_scanner_get_format(void) {
  if (qr_parser) {
    return qr_parser_get_format(qr_parser);
  }
  return -1;
}

bool qr_scanner_get_ur_result(const char **ur_type_out,
                              const uint8_t **cbor_data_out,
                              size_t *cbor_len_out) {
  if (qr_parser) {
    return qr_parser_get_ur_result(qr_parser, ur_type_out, cbor_data_out,
                                   cbor_len_out);
  }
  return false;
}
