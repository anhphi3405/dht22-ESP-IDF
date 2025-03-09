#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "dht.h"

#define DHT_PIN GPIO_NUM_4  // Chỉnh sửa chân kết nối
#define WAIT_TIME_MS 2000

void read_dht22_task(void *pvParameter)
{
    float temperature = -1;
    float humidity = -1;

    while (1) {
        if (dht_read_data(DHT_TYPE_AM2301, DHT_PIN, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI("DHT", "Nhiệt độ: %.1f °C, Độ ẩm: %.1f%%", temperature, humidity);
        } else {
            ESP_LOGE("DHT", "Lỗi khi đọc dữ liệu từ cảm biến DHT");
        }
        vTaskDelay(pdMS_TO_TICKS(WAIT_TIME_MS));
    }
}

void app_main(void)
{
    xTaskCreate(read_dht22_task, "read_dht22_task", 2048, NULL, 5, NULL);
}
