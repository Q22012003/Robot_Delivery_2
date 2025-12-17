// Config.h
#ifndef CONFIG_H
#define CONFIG_H

// --- Cấu hình GM65 ---
#define RX_PIN 16 
#define TX_PIN 17
#define GM65_BAUD_RATE 9600

// --- Cấu hình 5 Mắt Dò Line (INPUT) ---
// Giả sử 0 là gặp Line (Đen), 1 là nền (Trắng) hoặc ngược lại tùy cảm biến
#define SENSOR_1 32 // Trái cùng
#define SENSOR_2 33
#define SENSOR_3 25 // Giữa
#define SENSOR_4 26
#define SENSOR_5 27 // Phải cùng

// --- Cấu hình Động Cơ (L298N hoặc tương tự) ---
// Motor A (Trái)
#define MOTOR_L_IN1 18
#define MOTOR_L_IN2 19
#define MOTOR_L_PWM 5 

// Motor B (Phải)
#define MOTOR_R_IN3 21
#define MOTOR_R_IN4 22
#define MOTOR_R_PWM 23 

// --- Tham số điều khiển ---
#define BASE_SPEED 150       // Tốc độ cơ bản (0-255)
#define TURN_SPEED 160       // Tốc độ khi quay cua
#define TURN_90_TIME 600     // Thời gian quay 90 độ (ms) - CẦN CÂN CHỈNH THỰC TẾ
#define KP 20                // Hệ số PID (P)
#define KD 50                // Hệ số PID (D)

// --- Cấu hình MQTT ---
#define MQTT_TOPIC_PUBLISH "gm65/data/matrix_position"
#define MQTT_TOPIC_SUBSCRIBE  "gm65/data/command"

// --- FreeRTOS ---
#define QUEUE_LENGTH 10
#define NETWORK_STACK_SIZE (1024 * 6)
#define SCANNER_STACK_SIZE (1024 * 4)
#define LINE_STACK_SIZE    (1024 * 4) // [THÊM] Stack cho dò line

#endif