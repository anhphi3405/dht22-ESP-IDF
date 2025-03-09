#include "dht.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"


#define MAX_TIMINGS 85
static const char *TAG = "DHT";

esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t gpio_num, float *humidity, float *temperature)
{
    uint8_t data[5] = {0,0,0,0,0};
    uint8_t last_state = 1;
    uint8_t counter = 0;
    uint8_t j = 0;

    // Bước 1: Gửi tín hiệu start
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_num, 0);
    vTaskDelay(pdMS_TO_TICKS(18));
    gpio_set_level(gpio_num, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);

    // Bước 2: Đọc xung dữ liệu (40 bit)
    for (int i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (gpio_get_level(gpio_num) == last_state) {
            counter++;
            esp_rom_delay_us(1);
            if (counter == 255)
                break;
        }
        last_state = gpio_get_level(gpio_num);
        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 16) // ngưỡng: >16us coi là bit 1 (có thể hiệu chỉnh lại)
                data[j / 8] |= 1;
            j++;
        }
    }

    // Kiểm tra checksum
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        float h, c;
        if (sensor_type == DHT_TYPE_DHT11) {
            h = data[0];
            c = data[2];
        } else {  // DHT22 hoặc AM2301
            h = ((data[0] << 8) | data[1]) / 10.0f;
            c = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
            if (c > 125) {  // cho trường hợp DHT11 fallback
                c = data[2];
            }
            if (data[2] & 0x80) {
                c = -c;
            }
        }
        *humidity = h;
        *temperature = c;
        ESP_LOGI(TAG, "Humidity = %.1f %% Temperature = %.1f °C", h, c);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Data not good, skip");
        *humidity = -1;
        *temperature = -1;
        return ESP_ERR_TIMEOUT;
    }
}
