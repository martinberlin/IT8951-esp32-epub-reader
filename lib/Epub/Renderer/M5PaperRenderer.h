#pragma once
#include <esp_log.h>

#include <LovyanGFX.hpp>

#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "IT8951Renderer.h"

class M5PaperRenderer : public IT8951Renderer {

public:
  
  M5PaperRenderer(
      const GFXfont *regular_font,
      const GFXfont *bold_font,
      const GFXfont *italic_font,
      const GFXfont *bold_italic_font,
      const uint8_t *busy_icon,
      int busy_icon_width,
      int busy_icon_height)
      : IT8951Renderer(regular_font, bold_font, italic_font, bold_italic_font, busy_icon, busy_icon_width, busy_icon_height)
  {
    display.init();
    display.setVCOM(2000);
    // epd_fast:    LovyanGFX uses a 4×4 16pixel tile pattern to display a pseudo 17 level grayscale.
    // epd_quality: Uses 16 levels of grayscale
    display.setEpdMode(epd_mode_t::epd_fast);
    display.clearDisplay();
  }

  ~M5PaperRenderer()
  {
    // TODO: cleanup and shutdown?
  }

  // Not used with Lovyan since display is flushed automatically
  void flush_display()
  {
  }
  
  void flush_area(int x, int y, int width, int height)
  {
  }

  virtual bool hydrate()
  {
    ESP_LOGI("M5P", "Hydrating EPD");
    return true;
  }

  virtual void reset()
  {
    ESP_LOGI("M5P", "Full clear");
    display.clearDisplay();
  };

};