#ifndef CONFIG_H
#define CONFIG_H

// --- Cấu hình GM65 ---
#define RX_PIN 16 
#define TX_PIN 17
#define BAUD_RATE 9600

// --- Cấu hình MQTT Topic ---
#define MQTT_TOPIC_PUBLISH "gm65/data/matrix_position"
#define MQTT_TOPIC_SUBSCRIBE  "gm65/data/command"

#endif