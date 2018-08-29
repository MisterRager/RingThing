#include "../lib/ws2812.c"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <stdio.h>
#include <dirent.h>
#include "esp_log.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define WS2812_PIN 22
#define PIXEL_COUNT 22

#define TAG "RingThing"

static rgbVal * pixels;
static TaskHandle_t flushLightsTask = NULL;
static TaskHandle_t readFileTask = NULL;
static sdmmc_card_t *card;

bool initCardReaderSdMmc() {
    ESP_LOGI(TAG, "Setup GPIO pull-up for SD/MMC");

    // DAT0
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);
    // DAT1
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);
    // DAT2
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);
    // DAT3
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);
    // CLK
    gpio_set_pull_mode(GPIO_NUM_14, GPIO_PULLUP_ONLY);
    // CMD
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_1;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount the filesystem - might be able to format");
        } else {
            ESP_LOGE(TAG, "FAILED to initialize the card (%d). Make sure SD hardware is configured properly", err);
        }
        return false;
    }

    return true;
}

void cardInfo() {
    sdmmc_card_print_info(stdout, card);
}

void cardPrintFiles() {
    DIR * dr = opendir("/sdcard");

    if (dr == NULL) {
        ESP_LOGE(TAG, "Could not open directory /sdcard");
    } else {
        struct dirent * de;

        while ((de = readdir(dr)) != NULL) {
            ESP_LOGI(TAG, "file %s", de->d_name);
        }

        closedir(dr);
    }
}

extern "C" void testLights(void *params) {
    for(;;) {
        ESP_LOGI(TAG, "Show Lights...");
        ws2812_setColors(PIXEL_COUNT, pixels);

        vTaskSuspend(flushLightsTask);
    }
}

extern "C" void loadSdCard(void * params) {
    ESP_LOGI(TAG, "Load SD card");

    if (initCardReaderSdMmc()) {
        cardInfo();
        cardPrintFiles();
    }
    for(;;) {
        vTaskSuspend(readFileTask);
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

    xTaskCreate(loadSdCard, "mount SDMMC card", 4096, NULL, 10, &readFileTask);
    xTaskCreate(testLights, "ws2812 even odd demo", 4096, NULL, 10, &flushLightsTask);

    ESP_LOGI(TAG, "app_main() out");
    return;
}