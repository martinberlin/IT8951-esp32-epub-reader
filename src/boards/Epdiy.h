#pragma once

#include "Board.h"

class Epdiy : public Board
{
public:
  virtual void power_up();
  virtual void prepare_to_sleep();
  virtual Renderer *get_renderer();
  virtual ButtonControls *get_button_controls(xQueueHandle ui_queue);
  virtual TouchControls *get_touch_controls(Renderer *renderer, xQueueHandle ui_queue);
};