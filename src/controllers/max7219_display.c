#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <max7219/max7219.h>
#include "./digit_font.h"

#include <max7219/max7219.h>

#include "gdbstub.h"

#define MAX7219_CS_PIN 15
#define MAX7219_TASK_DELAY 1000

static max7219_display_t display = {
  .cs_pin       = MAX7219_CS_PIN,
  .digits       = 8,
  .cascade_size = 4,
  .mirrored     = false
};

void display_init() {
  max7219_init(&display);
}

void display_render_digit(int digit, int cs) {
  max7219_draw_image_8x8(&display, cs, DIGITS[digit]);
}

void display_render_task(void *pvParameters) {
  uint8_t counter = 0;

  while (true) {
    max7219_clear(&display);

    display_render_digit((counter+0) % 10, 0);
    display_render_digit((counter+1) % 10, 1);
    display_render_digit((counter+2) % 10, 2);
    display_render_digit((counter+3) % 10, 3);

    vTaskDelay(MAX7219_TASK_DELAY / portTICK_PERIOD_MS);

    counter++;
  }
}


