#pragma once
#include <esp_log.h>
#include <math.h>
#include "Renderer.h"
#include "Ubuntu_M12pt8b.h"

#define GAMMA_VALUE (1.0f / 0.8f)

class IT8951Renderer : public Renderer
{
protected:
  const GFXfont *m_regular_font;
  const GFXfont *m_bold_font;
  const GFXfont *m_italic_font;
  const GFXfont *m_bold_italic_font;
  const uint8_t *m_busy_image;
  int m_busy_image_width;
  int m_busy_image_height;
  bool needs_gray_flush = false;
  
  uint8_t gamma_curve[256] = {0};

  const GFXfont *get_font(bool is_bold, bool is_italic)
  {
    if (is_bold && is_italic)
    {
      return m_bold_italic_font;
    }
    if (is_bold)
    {
      return m_bold_font;
    }
    if (is_italic)
    {
      return m_italic_font;
    }
    return m_regular_font;
  }

public:
  IT8951Renderer(
      const GFXfont *regular_font,
      const GFXfont *bold_font,
      const GFXfont *italic_font,
      const GFXfont *bold_italic_font,
      const uint8_t *busy_icon,
      int busy_icon_width,
      int busy_icon_height)
      : m_regular_font(regular_font), m_bold_font(bold_font), m_italic_font(italic_font), m_bold_italic_font(bold_italic_font),
        m_busy_image(busy_icon), m_busy_image_width(busy_icon_width), m_busy_image_height(busy_icon_height)
  {
    //epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);

    for (int gray_value = 0; gray_value < 256; gray_value++)
    {
      gamma_curve[gray_value] = round(255 * pow(gray_value / 255.0, GAMMA_VALUE));
    }
  }

  void show_busy()
  {
    int x = (EPD_HEIGHT - m_busy_image_width) / 2;
    int y = (EPD_WIDTH - m_busy_image_height) / 2;
    int width = m_busy_image_width;
    int height = m_busy_image_height;
    // Check how to print an image buffer
  }

  void show_img(int x, int y, int width, int height, const uint8_t *img_buffer)
  {
    // Check how to print an image buffer
  }

  void needs_gray(uint8_t color)
  {
    if (color != 0 && color != 255)
    {
      needs_gray_flush = true;
    }
  }

  bool has_gray() {
    return needs_gray_flush;
  }

  int get_text_width(const char *text, bool bold = false, bool italic = false)
  {
    int x = 0, y = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    //epd_get_text_bounds(get_font(bold, italic), text, &x, &y, &x1, &y1, &x2, &y2, &m_font_props);
    return x2 - x1;
  }

  void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false)
  {
    // if using antialised text then set to gray next flush
    // needs_gray_flush = true;
    int ypos = y + get_line_height() + margin_top;
    int xpos = x + margin_left;
    //epd_write_string(get_font(bold, italic), text, &xpos, &ypos, m_frame_buffer, &m_font_props);
  }

  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    needs_gray(color);
    //epd_draw_rect({.x = x + margin_left, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    needs_gray(color);
    //epd_fill_rect({.x = x + margin_left, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void fill_circle(int x, int y, int r, uint8_t color = 0)
  {
    needs_gray(color);
    //epd_fill_circle(x, y, r, color, m_frame_buffer);
  }
  
  virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    needs_gray(color);
    //epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, m_frame_buffer);
  }

  virtual void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    needs_gray(color);
    //epd_draw_triangle(x0, y0, x1, y1, x2, y2, color, m_frame_buffer);
  }

  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    uint8_t corrected_color = gamma_curve[color];
    needs_gray(corrected_color);
    //epd_draw_pixel(x + margin_left, y + margin_top, corrected_color, m_frame_buffer);
  }

  virtual void draw_circle(int x, int y, int r, uint8_t color = 0)
  {
    needs_gray(color);
    //epd_draw_circle(x, y, r, color, m_frame_buffer);
  }

  virtual void flush_display() = 0;
 
  virtual void flush_area(int x, int y, int width, int height) = 0;

  virtual void clear_screen()
  {
    
  }

  virtual int get_page_width()
  {
    // don't forget we are rotated
    return EPD_HEIGHT - (margin_left + margin_right);
  }
  virtual int get_page_height()
  {
    // don't forget we are rotated
    return EPD_WIDTH - (margin_top + margin_bottom);
  }

  virtual int get_space_width()
  {
    //epd_get_glyph(m_regular_font, ' ');
    // Check how to get the advanceX for a font automatically
    auto space_glyph = 12;
    return space_glyph;
  }

  virtual int get_line_height()
  {
    // Same here check how to recognize this from the font definition
    return 20;
  }

  // dehydate a frame buffer to file. Not used here!
  virtual bool dehydrate()
  {
    return true;
  }

  // hydrate a frame buffer
  virtual bool hydrate()
  {
    return true;
  }

  virtual void reset() = 0;
};