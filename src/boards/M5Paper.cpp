#include <Renderer/M5PaperRenderer.h>
#include "Ubuntu_M12pt8b.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <hourglass.h>
#include <SPIFFS.h>
#include "controls/GPIOButtonControls.h"
#include "M5Paper.h"

#define IT8951_5V_ENABLE GPIO_NUM_38
// setup the pins to use for navigation
#define BUTTON_UP_GPIO_NUM GPIO_NUM_47
#define BUTTON_DOWN_GPIO_NUM GPIO_NUM_21
#define BUTTON_SELECT_GPIO_NUM GPIO_NUM_48
// buttons are high when pressed
#define BUTONS_ACTIVE_LEVEL 1

void M5Paper::power_up()
{
  gpio_set_direction(IT8951_5V_ENABLE, GPIO_MODE_OUTPUT);
  gpio_set_level(IT8951_5V_ENABLE, 1);
  vTaskDelay(pdMS_TO_TICKS(280));
}

void M5Paper::prepare_to_sleep()
{
  // keep the power on as we are going into deep sleep not shutting down completely
  gpio_set_level(IT8951_5V_ENABLE, 0);
  //gpio_hold_en(IT8951_5V_ENABLE);
}

Renderer *M5Paper::get_renderer()
{
  return new M5PaperRenderer(
      &Ubuntu_M12pt8b, // regular
      &Ubuntu_M12pt8b,  // bold
      &Ubuntu_M12pt8b,  // italic
      &Ubuntu_M12pt8b, // bold-italic
      hourglass_data,
      hourglass_width,
      hourglass_height);
}

void M5Paper::stop_filesystem()
{
#ifdef USE_SPIFFS
  delete spiffs;
#else
  // Seems to cause issues with the M5 Paper
  // delete sdcard;
#endif
}

ButtonControls *M5Paper::get_button_controls(xQueueHandle ui_queue)
{
  return new GPIOButtonControls(
      BUTTON_UP_GPIO_NUM,
      BUTTON_DOWN_GPIO_NUM,
      BUTTON_SELECT_GPIO_NUM,
      BUTONS_ACTIVE_LEVEL,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });
}
