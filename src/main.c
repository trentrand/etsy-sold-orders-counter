#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "esp/uart.h"

void user_init(void) {
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
}
