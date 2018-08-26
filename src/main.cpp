#include "../lib/ws2812.c"

#include "driver/gpio.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <stdio.h>
#include "esp_log.h"

#define WS2812_PIN 22
#define PIXEL_COUNT 22

#define TAG "RingThing"

static rgbVal * pixels;
static TaskHandle_t flushLightsTask = NULL;

extern "C" void testLights(void *params) {
    for(;;) {
        ESP_LOGI(TAG, "Show Lights...");
        ws2812_setColors(PIXEL_COUNT, pixels);

        vTaskSuspend(flushLightsTask);
    }
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "app_main()");

    ws2812_init(WS2812_PIN);

    auto color = makeRGBVal(50, 100, 200);
    auto dark = makeRGBVal(0, 0, 0);

    pixels = (rgbVal *) malloc(sizeof(rgbVal) * PIXEL_COUNT);
    for (auto k = 0; k < PIXEL_COUNT; k++) {
        ESP_LOGI(TAG, "Set lights for pixel %d", k);
        pixels[k] = (k % 2 == 0) ? color : dark;
        ESP_LOGI(TAG, "light %d set up now", k);
    }

    xTaskCreate(testLights, "ws2812 even odd demo", 4096, NULL, 10, &flushLightsTask);

    ESP_LOGI(TAG, "app_main() out");
    return;
}