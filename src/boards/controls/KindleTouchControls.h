#pragma once

#include "TouchControls.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <Renderer/Renderer.h>
#include "epdiy.h"
#include "driver/gpio.h"
#include <driver/i2c.h>

#include <string.h> // memset

#define TAG "TMA445"
// Front light control
#define FL_PWM  GPIO_NUM_11

#define I2C_PORT I2C_NUM_0
#define TS_INT  GPIO_NUM_3
// NO need for RST, we think

#define TOUCH_ADDR 0x24
// Adjust to your desired level. Too large might record many seconds which is bad for UX
#define TOUCH_QUEUE_LENGTH 15

#define CY_REG_BASE         	0x00
#define GET_NUM_TOUCHES(x)     	((x) & 0x0F)
#define GET_TOUCH1_ID(x)       	(((x) & 0xF0) >> 4)
#define GET_TOUCH2_ID(x)       	((x) & 0x0F)
#define CY_XPOS             		0
#define CY_YPOS             		1
#define CY_MT_TCH1_IDX      		0
#define CY_MT_TCH2_IDX      		1
#define be16_to_cpu(x) (uint16_t)((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

#define CY_OPERATE_MODE    		0x00
#define CY_SOFT_RESET_MODE     0x01
#define CY_LOW_POWER         	0x04 // Non used for now
#define CY_HNDSHK_BIT          0x80

// I2C common protocol defines
#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

static uint8_t sec_key[] = {0x00, 0xFF, 0xA5, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    
#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define GET_BOOTLOADERMODE(reg)		((reg & 0x10) >> 4)
/* TrueTouch Standard Product Gen3 (Txx3xx) interface definition */
struct cyttsp_xydata {
	uint8_t hst_mode;
	uint8_t tt_mode;
	uint8_t tt_stat;
	uint16_t x1 __attribute__ ((packed));
	uint16_t y1 __attribute__ ((packed));
	uint8_t z1;
	uint8_t touch12_id;
	uint16_t x2 __attribute__ ((packed));
	uint16_t y2 __attribute__ ((packed));
	uint8_t z2;
};

struct cyttsp_bootloader_data {
	uint8_t bl_file;
	uint8_t bl_status;
	uint8_t bl_error;
	uint8_t blver_hi;
	uint8_t blver_lo;
	uint8_t bld_blver_hi;
	uint8_t bld_blver_lo;
	uint8_t ttspver_hi;
	uint8_t ttspver_lo;
	uint8_t appid_hi;
	uint8_t appid_lo;
	uint8_t appver_hi;
	uint8_t appver_lo;
	uint8_t cid_0;
	uint8_t cid_1;
	uint8_t cid_2;
};

// I2C methods
esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size);
esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size);
esp_err_t i2c_read_reg( uint8_t reg, uint8_t *pdata, uint8_t count);
esp_err_t i2c_write_reg( uint8_t reg, uint8_t *pdata, uint8_t count);
int _cyttsp_hndshk();
int _cyttsp_hndshk_n_write(uint8_t write_back);

class Renderer;

class KindleTouchControls : public TouchControls
{
private:
  xTaskHandle m_handle;
	void*       m_taskData;
	static void runTask(void* data);
	std::string m_taskName = "touchINT";
  uint16_t    m_stackSize = configMINIMAL_STACK_SIZE * 8;
	uint8_t     m_priority = 5;
	BaseType_t  m_coreId = APP_CPU_NUM;

  ActionCallback_t on_action;
  KindleTouchControls *ts;
  // Touch UI buttons (Top left and activated only when USE_TOUCH is defined)
  uint8_t ui_button_width = 120;
  uint8_t ui_button_height = 34;
  UIAction last_action = NONE;
  Renderer *renderer = nullptr;

  void touchStuff();

public:
  KindleTouchControls(Renderer *renderer, int touch_int, int width, int height, int rotation, ActionCallback_t on_action);
  void render(Renderer *renderer);
  void renderPressedState(Renderer *renderer, UIAction action, bool state = true);
  void handleTouch(int x, int y);
};