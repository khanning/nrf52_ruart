#include <stdint.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"

#include "board/usbuart.h"
#include "board/radio.h"

uint8_t buffer[250];
uint8_t index;

void clock_init(void)
{
  nrf_drv_clock_init();
  nrf_drv_clock_hfclk_request(NULL);
  while(!nrf_drv_clock_hfclk_is_running()) {}
  nrf_drv_clock_lfclk_request(NULL);
  while(!nrf_drv_clock_lfclk_is_running()) {}
}

int main(void)
{
  clock_init();
  app_timer_init();
  usbserial_init();
  radio_configure();

  while(1) {
    upoll();
    if (uavail()) {
      index = 0;
      while (uavail()) {
        buffer[index++] = ugetc();
      }
      radio_send_buffer(buffer, index);
    }
  }
}
