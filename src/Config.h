// Config.h
#ifndef CONFIG_H
#define CONFIG_H

// --- 1. Cấu hình GM65 (QR Code) ---
// LƯU Ý: Phải đổi sang chân khác vì 16, 17 đã dùng cho Sensor
#define RX_PIN 16 
#define TX_PIN 17 
#define GM65_BAUD_RATE 115200

// --- 2. Cấu hình 5 Mắt Dò Line (INPUT) ---
// Thứ tự: {16, 17, 5, 18, 19}
#define SENSOR_1 19 // Trái cùng
#define SENSOR_2 18
#define SENSOR_3 5  // Giữa
#define SENSOR_4 22
#define SENSOR_5 23 // Phải cùng
#define NUM_SENSORS 5

// --- 3. Cấu hình Động Cơ (OUTPUT) ---
// Right Motor (EnA) - InA (35), InB (32)
// LƯU Ý: GPIO 35 trên ESP32 thường là INPUT ONLY. Kiểm tra kỹ nếu bánh phải không chạy.
#define MOTOR_R_PWM 26 
#define MOTOR_R_IN1 4 
#define MOTOR_R_IN2 32 

// Left Motor (EnB) - InC (33), InD (25)
#define MOTOR_L_PWM 27 
#define MOTOR_L_IN1 33 
#define MOTOR_L_IN2 25 

// --- 4. PID & SPEED ---
#define START_SPEED 85      //Define value < 60 (error motor) 
#define MAX_SPEED   85      
#define TURN_SPEED_LOW  150  
#define TURN_SPEED_HIGH 150  

#define PID_KP 11.5
#define PID_KI 0.0 
#define PID_KD 3.8

// --- 5. Hệ thống ---
#define MQTT_TOPIC_PUBLISH "gm65/data/matrix_position"
#define MQTT_TOPIC_SUBSCRIBE  "gm65/data/command"
#define MQTT_TOPIC_DEBUG      "gm65/debug"

// --- 6. Bluetooth Debug ---
#define DEBUG_RX_PIN 13 // Nối vào TX của HC-05
#define DEBUG_TX_PIN 14 // Nối vào RX của HC-05
#define DEBUG_BAUD   9600 // HC-05 thường mặc định là 9600

#define QUEUE_LENGTH 10
#define NETWORK_STACK_SIZE (1024 * 6)
#define SCANNER_STACK_SIZE (1024 * 4)
#define LINE_STACK_SIZE    (1024 * 4)

#endif