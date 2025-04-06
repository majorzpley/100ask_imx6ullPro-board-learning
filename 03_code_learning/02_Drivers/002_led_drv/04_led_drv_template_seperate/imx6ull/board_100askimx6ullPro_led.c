#include "led_resource.h"

static struct led_resource board_100askimx6ullPro_led = {
    .pin = GROUP_PIN(5, 3),
};

struct led_resource *get_led_resource(void) {
  return &board_100askimx6ullPro_led;
}
