#ifndef DHT_H
#define DHT_H

#include "driver/gpio.h"
#include "esp_err.h"

typedef enum {
    DHT_TYPE_DHT11,
    DHT_TYPE_DHT22,
    DHT_TYPE_AM2301  // AM2301 tương tự DHT22
} dht_sensor_type_t;

/**
 * @brief Đọc dữ liệu từ cảm biến DHT.
 *
 * @param sensor_type Kiểu cảm biến (DHT_TYPE_DHT11, DHT_TYPE_DHT22, hoặc DHT_TYPE_AM2301)
 * @param gpio_num Số chân GPIO kết nối với cảm biến
 * @param[out] humidity Con trỏ chứa giá trị độ ẩm
 * @param[out] temperature Con trỏ chứa giá trị nhiệt độ
 * @return ESP_OK nếu thành công, ngược lại trả về mã lỗi.
 */
esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t gpio_num, float *humidity, float *temperature);

#endif /* DHT_H */
