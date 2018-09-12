
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <stdio.h>
#include "esp_log.h"
#include <functional>

#include "./card.h"
#include "./lights.h"
#include "./wifi.h"
#include "./http_server.h"

static auto TAG = "RingThing";

static TaskHandle_t flushLightsTask = NULL;
static TaskHandle_t readFileTask = NULL;
static TaskHandle_t networkInitTask = NULL;

static void flushLights() {
    if (flushLightsTask) vTaskResume(flushLightsTask);
}

extern "C" void testLights(void *params) {
    lights_init();
    for(;;) {
        lights_flush();
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

extern "C" void startWifiAndHttpServer(void * params) {
    ESP_LOGI(TAG, "Connect to WIFI");
    connect_wifi([]() {
        ESP_LOGI(TAG, "onConnect callback triggered!");
        vTaskResume(networkInitTask);
        flushLights();
    });

    vTaskSuspend(networkInitTask);
    ESP_LOGI(TAG, "Init HTTPD");

    ringthing_http_start_server();

    //const TickType_t xDelay = 10 / portTICK_PERIOD_MS;

    for (;;) {
        ESP_LOGI(TAG, "ENTERING WORKER LOOP FOR HTTPD");
        ringthing_http_server_loop();
        //vTaskDelay(xDelay);
    }

    vTaskSuspend(networkInitTask);
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "app_main()");


    xTaskCreate(loadSdCard, "mount SDMMC card", 4096, NULL, 10, &readFileTask);
    xTaskCreate(testLights, "ws2812 even odd demo", 4096, NULL, 10, &flushLightsTask);
    xTaskCreate(startWifiAndHttpServer, "start wifi/httpd", 4096, NULL, 10, &networkInitTask);

    ESP_LOGI(TAG, "app_main() out");
    return;
}
