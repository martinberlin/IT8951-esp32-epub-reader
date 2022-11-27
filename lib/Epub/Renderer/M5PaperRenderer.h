#pragma once
#include <esp_log.h>

#include <LovyanGFX.hpp>

#include "Ubuntu_M12pt8b.h"
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "IT8951Renderer.h"

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

class M5PaperRenderer : public IT8951Renderer {

public:
  LGFX display;
  
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
    // epd_fast:    LovyanGFX uses a 4×4 16pixel tile pattern to display a pseudo 17 level grayscale.
    // epd_quality: Uses 16 levels of grayscale
    display.setEpdMode(epd_mode_t::epd_fast);
    display.clearDisplay();
    //driver.SetColorReverse(true);
  }

  ~M5PaperRenderer()
  {
    // TODO: cleanup and shutdown?
  }

  void flush_display()
  {
    printf("flush_display()");
  }
  void flush_area(int x, int y, int width, int height)
  {
    printf("flush_display(%d,%d, W:%d, H:%d)", x, y,  width, height);
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