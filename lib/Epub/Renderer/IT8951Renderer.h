#pragma once
#include <esp_log.h>
#include <math.h>
#include "Renderer.h"
#include "Ubuntu_M12pt8b.h"

#define GAMMA_VALUE (1.0f / 0.8f)

// Display Width/Height in platformio ini. Adjust to yours if you use a different one

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_IT8951   _panel_instance;
  lgfx::Bus_SPI       _bus_instance;

public:
// Provide method to access VCOM (https://github.com/lovyan03/LovyanGFX/issues/269)
  void setVCOM(uint16_t v) { _panel_instance.setVCOM(v); }
  uint16_t getVCOM(void) { return _panel_instance.getVCOM(); }
  
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。

// SPIバスの設定
      cfg.spi_host = SPI2_HOST;     // 使用するSPIを選択  (VSPI_HOST or HSPI_HOST)
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = false;        // 受信をMOSI IMPORTANT use it on false to read from MISO!
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = 1;          // Set the DMA channel (1 or 2. 0=disable)   使用するDMAチャンネルを設定 (0=DMA不使用)
      cfg.pin_sclk = CONFIG_IT8951_SPI_CLK;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = CONFIG_IT8951_SPI_MOSI;           // SPIのMOSIピン番号を設定
      cfg.pin_miso = CONFIG_IT8951_SPI_MISO;           // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = -1;            // SPIのD/Cピン番号を設定  (-1 = disable)
 
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs           =    CONFIG_IT8951_SPI_CS;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    -1;
      cfg.pin_busy         =    CONFIG_IT8951_BUSY;    // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

      cfg.memory_width     =   EPD_WIDTH;  // ドライバICがサポートしている最大の幅
      cfg.memory_height    =   EPD_HEIGHT;  // ドライバICがサポートしている最大の高さ
      cfg.panel_width      =   EPD_WIDTH;  // 実際に表示可能な幅
      cfg.panel_height     =   EPD_HEIGHT;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           =  true;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // データ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

class IT8951Renderer : public Renderer
{
public:
    LGFX display;
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
     display.setRotation(1);
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
    return display.textWidth(text);
  }

  void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false)
  {
    int ypos = y + get_line_height() + margin_top;
    int xpos = x + margin_left;
    display.setCursor(xpos, ypos);
    display.setFont(get_font(bold, italic));
    display.printf("%s", text);
  }

  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    display.setColor(display.color888(color,color,color));
    display.drawRect(x + margin_left, y + margin_top, width, height);
    if (color!=0) {
      display.setColor(display.color888(0,0,0));
    }
  }

  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    display.fillRect(x + margin_left, y + margin_top, width, height);
  }
  virtual void fill_circle(int x, int y, int r, uint8_t color = 0)
  {
    display.fillCircle(x, y, r);
  }
  
  virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    display.fillTriangle(x0, y0, x1, y1, x2, y2);
  }

  virtual void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    display.drawTriangle(x0, y0, x1, y1, x2, y2);
  }

  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    // Draws nothing because it's being called ineficiently to render images
    display.drawPixel(x + margin_left, y + margin_top);
  }

  virtual void draw_jpeg(const uint8_t *jpg_data, uint32_t jpg_len, int32_t x, int32_t y, int32_t maxWidth, int32_t maxHeight, int32_t offX, int32_t offY) {
    display.drawJpg(jpg_data, jpg_len, x, y, maxWidth, maxHeight, offX, offY);
  }

  virtual void draw_circle(int x, int y, int r, uint8_t color = 0)
  {
    display.drawCircle(x, y, r);
  }

  virtual void flush_display() = 0;
 
  virtual void flush_area(int x, int y, int width, int height) = 0;

  virtual void clear_screen()
  {
    // Leaves marks all over when clearDisplay is not called in quality mode (Full refresh GC16 mode)
    //display.fillScreen(display.color888(255,255,255));
    //printf("Clears in full gray\n");
    display.setEpdMode(epd_mode_t::epd_quality);
    display.clearDisplay();
    vTaskDelay(pdMS_TO_TICKS(50));
    display.setEpdMode(epd_mode_t::epd_fast);
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
    auto space_glyph = display.textWidth((char*)" ");
    return space_glyph*2;
  }

  virtual int get_line_height()
  {
    // Same here check how to recognize this from the font definition
    return 26;
  }

  // dehydate a frame buffer to file. Not used here!
  virtual bool dehydrate()
  {
    return true;
  }

  // hydrate a frame buffer.
  virtual bool hydrate()
  {
    return true;
  }

  virtual void reset() = 0;
};