#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "dht.h"

#define DHT_PIN GPIO_NUM_4    // Chân kết nối cảm biến DHT
#define WAIT_TIME_MS 2000     // Thời gian chờ giữa các lần đọc DHT

#define UART_NUM UART_NUM_2   // Cổng UART cho GPS
#define TXD_PIN GPIO_NUM_17   // Chân TX của GPS
#define RXD_PIN GPIO_NUM_16   // Chân RX của GPS
#define BUF_SIZE 1024         // Kích thước buffer đọc dữ liệu

static const char *TAG_DHT = "DHT";
static const char *TAG_GPS = "GPS";

// Cấu hình UART cho GPS
void uart_init()
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// Chuyển đổi tọa độ từ định dạng "ddmm.mmmm" sang độ thập phân
float convert_to_decimal(float nmea_coord)
{
    if (nmea_coord == 0.0) return 0.0; // Handle cases where coordinate might be 0.0

    int degrees = (int)(nmea_coord / 100);
    float minutes = nmea_coord - (degrees * 100);
    return degrees + (minutes / 60);
}

// Phân tích câu NMEA GGA để lấy Kinh độ, Vĩ độ, Độ cao
void parse_gga_sentence(char *nmea)
{
    char time[10], lat[15], lat_dir[2], lon[15], lon_dir[2], fix_status[2], sats[3], hdop[5], altitude[10], alt_unit[2];
    float latitude, longitude, altitude_value;

    // Kiểm tra xem có phải câu GGA không
    if (strncmp(nmea, "$GPGGA", 6) == 0) {
        int count = sscanf(nmea, "$GPGGA,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]",
                           time, lat, lat_dir, lon, lon_dir, fix_status, sats, hdop, altitude, alt_unit);

        if (count >= 9 && fix_status[0] != '0') { // Check fix_status as string
            latitude = convert_to_decimal(atof(lat));
            longitude = convert_to_decimal(atof(lon));
            altitude_value = atof(altitude);

            // Chuyển đổi sang giá trị âm nếu hướng là Tây hoặc Nam
            if (lat_dir[0] == 'S') latitude = -latitude;
            if (lon_dir[0] == 'W') longitude = -longitude;

            ESP_LOGI(TAG_GPS, "📍 Vĩ độ (Latitude): %.6f %s, Kinh độ (Longitude): %.6f %s, Độ cao (Altitude): %.2f %s",
                     latitude, lat_dir, longitude, lon_dir, altitude_value, alt_unit);
        } else {
            ESP_LOGW(TAG_GPS, "⚠️ GPS chưa có tín hiệu (No Fix)");
        }
    }
}

// Task đọc dữ liệu từ cảm biến DHT
void read_dht22_task(void *pvParameter)
{
    float temperature = -1, humidity = -1;
    while (1) {
        if (dht_read_data(DHT_TYPE_AM2301, DHT_PIN, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI(TAG_DHT, "🌡️ Nhiệt độ: %.1f °C, 💧 Độ ẩm: %.1f%%", temperature, humidity);
        } else {
            ESP_LOGE(TAG_DHT, "❌ Lỗi đọc cảm biến DHT");
        }
        vTaskDelay(pdMS_TO_TICKS(WAIT_TIME_MS));
    }
}

// Task đọc dữ liệu từ GPS qua UART
void read_gps_task(void *pvParameter)
{
    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            data[len] = '\0';  // Đảm bảo chuỗi kết thúc
            // ESP_LOGI(TAG_GPS, "📡 GPS Raw: %s", data);

            // Tìm và xử lý câu GGA
            char *gga_start = strstr((char *)data, "$GPGGA");
            if (gga_start) {
                parse_gga_sentence(gga_start);
            }
        }
    }
}

// Hàm chính
void app_main(void)
{
    uart_init();  // Khởi tạo UART cho GPS

    ESP_LOGI(TAG_GPS, "Khởi tạo GPS và DHT22...");

    // Tạo task đọc dữ liệu từ DHT22
    xTaskCreate(read_dht22_task, "read_dht22_task", 2048, NULL, 5, NULL);

    // Tạo task đọc dữ liệu từ GPS
    xTaskCreate(read_gps_task, "read_gps_task", 4096, NULL, 5, NULL);
}