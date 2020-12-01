#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#include "ssid_config.h"

#include "./https_fetch.c"
#include "./controllers/max7219_display.c"

/* #include "gdbstub.h" */

int orderCount = 0;

void user_init(void) {
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  struct sdk_station_config config = {
    .ssid = WIFI_SSID,
    .password = WIFI_PASS,
  };

  // required to call wifi_set_opmode before station_set_config
  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(&config);

  /* gdbstub_init(); */

  display_init();
  xTaskCreate(&fetch_order_count_task, "Task: Fetch order count", 2048, NULL, 2, NULL);
  xTaskCreate(&display_render_task, "Task: Render order count", 2048, NULL, 3, NULL);
}
