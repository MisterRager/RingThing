#include "../lib/ws2812.c"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define WS2812_PIN 22
#define PIXEL_COUNT 22

#define TAG "RingThing"

static rgbVal * pixels;
static TaskHandle_t flushLightsTask = NULL;

void initCardReader() {
    ESP_LOGI(TAG, "Setup GPIO pull-up for SD/MMC");

    // DAT0 (SD) or DO (SPI)
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);
    // CD (SD) or CS (SPI)
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);
    // CLK
    gpio_set_pull_mode(GPIO_NUM_14, GPIO_PULLUP_ONLY);
    // CMD (SD) or DI (SPI)
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.flags |= SDMMC_HOST_FLAG_SPI;

    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = GPIO_NUM_4;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount the filesystem - might be able to format");
        } else {
            ESP_LOGE(TAG, "FAILED to initialize the card (%d). Make sure SD hardware is configured properly", err);
        }
    }
}

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

    auto color = makeRGBVal(10, 5, 0);
    auto dark = makeRGBVal(0, 0, 0);

    pixels = (rgbVal *) malloc(sizeof(rgbVal) * PIXEL_COUNT);
    for (auto k = 0; k < PIXEL_COUNT; k++) {
        ESP_LOGI(TAG, "Set lights for pixel %d", k);
        pixels[k] = (k % 2 == 0) ? color : dark;
        ESP_LOGI(TAG, "light %d set up now", k);
    }

    xTaskCreate(testLights, "ws2812 even odd demo", 4096, NULL, 10, &flushLightsTask);

    initCardReader();

    ESP_LOGI(TAG, "app_main() out");
    return;
}