#pragma once

#include "KindleTouchControls.h"

QueueHandle_t gpio_evt_queue = NULL;

static KindleTouchControls *instance = nullptr;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size) {
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be read
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ), ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    // Send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, (i2c_ack_type_t)ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, (i2c_ack_type_t)NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    // write the data
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* Read contents of a register
---------------------------------------------------------------------------*/
esp_err_t i2c_read_reg( uint8_t reg, uint8_t *pdata, uint8_t count ) {
	return( i2c_master_read_slave_reg( I2C_NUM_0, TOUCH_ADDR,  reg, pdata, count ) );
}

/* Write value to specified register
---------------------------------------------------------------------------*/
esp_err_t i2c_write_reg( uint8_t reg, uint8_t *pdata, uint8_t count ) {
	return( i2c_master_write_slave_reg( I2C_NUM_0, TOUCH_ADDR,  reg, pdata, count ) );
}

int _cyttsp_hndshk_n_write(uint8_t write_back)
{
	int retval = -1;
	uint8_t hst_mode[1];
	uint8_t cmd[1];
	uint8_t tries = 0;
	while (retval < 0 && tries++ < 20){
		DELAY(5);
        retval = i2c_read_reg(0x00, (uint8_t*)&hst_mode, sizeof(hst_mode));
        if(retval < 0) {	
			printf("%s: bus read fail on handshake ret=%d, retries=%d\n",
				__func__, retval, tries);
			continue;
		}
        cmd[0] = hst_mode[0] & CY_HNDSHK_BIT ?
		write_back & ~CY_HNDSHK_BIT :
		write_back | CY_HNDSHK_BIT;
        retval = i2c_write_reg(CY_REG_BASE, (uint8_t*)&cmd, sizeof(cmd));
        if(retval < 0)
			printf("%s: bus write fail on handshake ret=%d, retries=%d\n",
				__func__, retval, tries);
    }
    return retval;
}

// 317 cyttsp.c
int _cyttsp_hndshk()
{
	int retval = -1;
	uint8_t hst_mode[1];
	uint8_t cmd[1];
	uint8_t tries = 0;
	while (retval < 0 && tries++ < 20){
		DELAY(5);
        retval = i2c_read_reg(0x00, (uint8_t*)&hst_mode, sizeof(hst_mode));
        if(retval < 0) {	
			printf("%s: bus read fail on handshake ret=%d, retries=%d\n",
				__func__, retval, tries);
			continue;
		}
        cmd[0] = hst_mode[0] & CY_HNDSHK_BIT ?
		hst_mode[0] & ~CY_HNDSHK_BIT :
		hst_mode[0] | CY_HNDSHK_BIT;
        retval = i2c_write_reg(CY_REG_BASE, (uint8_t*)&cmd, sizeof(cmd));
        if(retval < 0)
			printf("%s: bus write fail on handshake ret=%d, retries=%d\n",
				__func__, retval, tries);
    }
    return retval;
}

/**
 * Static class member that actually runs the target task.
 * touch Receive INT from Queue
 * 
 */
static void touch_INT(void *data) {
    uint32_t io_num;
    struct cyttsp_xydata xy_data;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            //printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            /* get event data from CYTTSP device */
            int retval;
            retval = i2c_read_reg(CY_REG_BASE, (uint8_t*)&xy_data, sizeof(xy_data));
            if (retval < 0) {
		        printf("%s: Error, fail to read device on host interface bus\n", __func__);
            }
            /* provide flow control handshake */
            _cyttsp_hndshk();
            int touch = GET_NUM_TOUCHES(xy_data.tt_stat);
            if (touch) {
                xy_data.x1 = be16_to_cpu(xy_data.x1);
                xy_data.y1 = be16_to_cpu(xy_data.y1);
                instance->handleTouch(xy_data.x1, xy_data.y1);
                printf("x:%d y:%d\n",xy_data.x1,xy_data.y1);
            }
        }

    }
}


void KindleTouchControls::touchStuff() {
    // soft reset
    esp_err_t err;
    uint8_t softres[] = {0x01};
    err = i2c_write_reg(CY_REG_BASE, (uint8_t *) &softres, 1);
    if (err != ESP_OK) {
        printf("i2c_write_reg CY_SOFT_RESET_MODE FAILED \n");
    }
    DELAY(50);
    // write sec_key
    err = i2c_write_reg(CY_REG_BASE, (uint8_t *) &sec_key, sizeof(sec_key));
    if (err != ESP_OK) {
        printf("i2c_write_reg sec_key FAILED \n");
    }
    printf("i2c_write_reg sec_key OK \n");
    DELAY(88);
    
    // Before this there is a part that reads sysinfo and sets some registers
    // Maybe that is the key to start the engines faster?
    uint8_t tries = 0;
    struct cyttsp_bootloader_data  bl_data = {};
    do {
		DELAY(20);
        i2c_read_reg(CY_REG_BASE, (uint8_t *) &bl_data, sizeof(bl_data));
        uint8_t *bl_data_p = (uint8_t *)&(bl_data);
        /* for (int i = 0; i < sizeof(struct cyttsp_bootloader_data); i++) {
			printf("bl_data[%d]=0x%x\n", i, bl_data_p[i]);
        }
        printf("bl_data status=0x%x\n", bl_data.bl_status); */
	} while (GET_BOOTLOADERMODE(bl_data.bl_status) && tries++ < 10);

    printf("bl_data status=0x%x\n", GET_BOOTLOADERMODE(bl_data.bl_status));

    // Set OP mode
    int retval;
    uint8_t cmd = CY_OPERATE_MODE;
    struct cyttsp_xydata xy_data;
    printf("%s set operational mode\n",__func__);
	  memset(&(xy_data), 0, sizeof(xy_data));
    /* wait for TTSP Device to complete switch to Operational mode */
	  DELAY(20);
    retval = i2c_read_reg(CY_REG_BASE, (uint8_t *) &xy_data, sizeof(xy_data));
    printf("%s: hstmode:0x%x tt_mode:0x%x tt_stat:0x%x ", __func__, xy_data.hst_mode, xy_data.tt_mode, xy_data.tt_stat);
}

KindleTouchControls::KindleTouchControls(Renderer *renderer, int touch_int, int width, int height, int rotation, ActionCallback_t on_action)
    : on_action(on_action), renderer(renderer)
{
  instance = this;

  gpio_set_pull_mode(FL_PWM, (gpio_pull_mode_t)GPIO_PULLDOWN_ENABLE);
  /** Instantiate touch. Important inject here the display width and height size in pixels
        setRotation(3)     Portrait mode */
//zero-initialize the config structure.
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = TS_INT;
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t)1;
    gpio_config(&io_conf);
    //create a queue to handle gpio event from isr
    struct cyttsp_xydata xy_data;
    gpio_evt_queue = xQueueCreate(TOUCH_QUEUE_LENGTH, sizeof(uint8_t));

    //i2c_master_init(); // I2C is already started by LVGL

    // Setup interrupt for this IO that goes low on the interrupt
    gpio_set_intr_type(TS_INT, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(TS_INT, gpio_isr_handler, (void *)TS_INT);
    
    
    // INT detection task
    xTaskCreatePinnedToCore(touch_INT, TAG, configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
    
    touchStuff();
}

void KindleTouchControls::render(Renderer *renderer)
{
  renderer->set_margin_top(0);
  uint16_t x_offset = 10;
  uint16_t x_triangle = x_offset + 70;
  // DOWN
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->draw_triangle(x_triangle, 20, x_triangle - 5, 6, x_triangle + 5, 6, 0);
  // UP
  x_offset = ui_button_width + 30;
  x_triangle = x_offset + 70;
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->draw_triangle(x_triangle, 6, x_triangle - 5, 20, x_triangle + 5, 20, 0);
  // SELECT
  x_offset = ui_button_width * 2 + 60;
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->draw_circle(x_offset + (ui_button_width / 2) + 9, 15, 5, 0);
  renderer->set_margin_top(35);
}

void KindleTouchControls::renderPressedState(Renderer *renderer, UIAction action, bool state)
{
  renderer->set_margin_top(0);
  switch (action)
  {
  case DOWN:
  {
    if (state)
    {
      renderer->fill_triangle(80, 20, 75, 6, 85, 6, 0);
    }
    else
    {
      renderer->fill_triangle(81, 19, 76, 7, 86, 7, 255);
    }
    renderer->flush_area(76, 6, 10, 15);
    break;
  }
  case UP:
  {
    if (state)
    {
      renderer->fill_triangle(220, 6, 220 - 5, 20, 220 + 5, 20, 0);
    }
    else
    {
      renderer->fill_triangle(221, 7, 221 - 5, 19, 221 + 5, 19, 255);
    }
    renderer->flush_area(195, 225, 10, 15);
  }
  break;
  case SELECT:
  {
    uint16_t x_circle = (ui_button_width * 2 + 60) + (ui_button_width / 2) + 9;
    renderer->fill_circle(x_circle, 15, 5, 0);
    renderer->flush_area(x_circle - 3, 12, 6, 6);
    // TODO - this causes a stack overflow when select is picked
    // renderPressedState(renderer, last_action, false);
  }
  break;
  case LAST_INTERACTION:
  case NONE:
    break;
  }
  renderer->set_margin_top(35);
}

void KindleTouchControls::handleTouch(int x, int y)
{
  UIAction action = NONE;
  ESP_LOGI("TOUCH", "Received touch event %d,%d", x, y);
  if (x >= 10 && x <= 10 + ui_button_width && y < 200)
  {
    action = DOWN;
    renderPressedState(renderer, UP, false);
  }
  else if (x >= 150 && x <= 150 + ui_button_width && y < 200)
  {
    action = UP;
    renderPressedState(renderer, DOWN, false);
  }
  else if (x >= 300 && x <= 300 + ui_button_width && y < 200)
  {
    action = SELECT;
  }
  else
  {
    // Touched anywhere but not the buttons
    action = LAST_INTERACTION;
  }
  last_action = action;
  if (action != NONE)
  {
    on_action(action);
  }
}