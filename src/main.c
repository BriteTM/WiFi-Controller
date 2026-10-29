#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "hello";

void app_main(void) {
    int n = 0;
    while (1) {
        ESP_LOGI(TAG, "counter %d", n++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
