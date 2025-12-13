#ifndef CONFIG_H
#define CONFIG_H

// --- Cấu hình GM65 ---
#define RX_PIN 16 
#define TX_PIN 17
#define GM65_BAUD_RATE 9600

// --- Cấu hình MQTT Topic ---
#define MQTT_TOPIC_PUBLISH "gm65/data/matrix_position"
#define MQTT_TOPIC_SUBSCRIBE  "gm65/data/command"

// --- Cấu hình FreeRTOS ---
#define QUEUE_LENGTH 10       // Hàng đợi chứa tối đa 10 mã barcode chưa kịp gửi
#define NETWORK_STACK_SIZE (1024 * 10) // SSL cần nhiều RAM (10KB)
#define SCANNER_STACK_SIZE (1024 * 4)  // Task đọc serial nhẹ hơn (4KB)

#endif