
#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include "JPEGHelper.h"
#include "Renderer.h"

static const char *TAG = "JPG";

#define POOL_SIZE 32768

bool JPEGHelper::get_size(const uint8_t *data, size_t data_size, int *width, int *height)
{
  void *pool = malloc(POOL_SIZE);
  if (!pool)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for pool");
    return false;
  }
  m_data = data;
  m_data_pos = 0;
  // decode the jpeg and get its size
  JDEC dec;
  JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
  if (res == JDR_OK)
  {
    ESP_LOGI(TAG, "JPEG Decoded - size %d,%d", dec.width, dec.height);
    *width = dec.width;
    *height = dec.height;
  }
  else
  {
    ESP_LOGE(TAG, "JPEG Decode failed - %d", res);
    return false;
  }
  free(pool);
  m_data = nullptr;
  m_data_pos = 0;
  return true;
}
bool JPEGHelper::render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height)
{
  // Check how to use scaling!
  renderer->draw_jpeg(data, data_size, x_pos, y_pos, width, height, 0, 0);
  return true;
}

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  if (context->m_data == nullptr)
  {
    ESP_LOGE(TAG, "No image data");
    return 0;
  }
  if (buff)
  {
    memcpy(buff, context->m_data + context->m_data_pos, ndata);
  }
  context->m_data_pos += ndata;
  return ndata;
}
