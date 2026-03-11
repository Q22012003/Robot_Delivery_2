// Config.h
#ifndef CONFIG_H
#define CONFIG_H
#include "Certificates.h"
// --- 1. Cấu hình GM65 (QR Code) ---
// LƯU Ý: Phải đổi sang chân khác vì 16, 17 đã dùng cho Sensor
#define RX_PIN 22 
#define TX_PIN 23 
#define GM65_BAUD_RATE 115200

// --- 2. Cấu hình 8 Mắt Dò Line (INPUT ANALOG) ---
// Thứ tự từ Trái sang Phải: S0 -> S7
#define SENSOR_0 35
#define SENSOR_1 32
#define SENSOR_2 33
#define SENSOR_3 34
#define SENSOR_4 36
#define SENSOR_5 39
//#define SENSOR_6 27
//#define SENSOR_7 14 
#define NUM_SENSORS 6

// --- 3. Cấu hình Động Cơ (OUTPUT) ---
// Dựa trên code mới (L298N Mode):
// Left Motor: LDIR(5), IN4(18), LPWM(19)
#define PIN_LDIR     5
#define PIN_IN4_LEFT 18
#define PIN_LPWM     19

// Right Motor: RDIR(17), IN2(16), RPWM(4)
#define PIN_RDIR      17
#define PIN_IN2_RIGHT 16
#define PIN_RPWM      4

// --- 4. PID & SPEED ---
// --- 4. PID & SPEED ---
#if (VEHICLE_ID == 1 || VEHICLE_ID == 2)
  #define BASE_SPEED       58    // Tốc độ chạy thẳng cơ bản (speed_run_forward)
  #define BASE_SPEED_BLIND 65
  #define MAX_SPEED        120   // Giới hạn PWM
  #define TURN_SPEED       85    // Tốc độ khi quay mù (Left/Right)
  #define FORWARD 75
  #define BACK 90
#else
  // V1 và V2 dùng chung
  #define BASE_SPEED       50    // Tốc độ chạy thẳng cơ bản (speed_run_forward)
  #define BASE_SPEED_BLIND 45
  #define MAX_SPEED        120   // Giới hạn PWM
  #define TURN_SPEED       80    // Tốc độ khi quay mù (Left/Right)
  #define FORWARD 60
  #define BACK 70
#endif

// PID Constants từ code mới
#define PID_KP 0.8
#define PID_KD 2.4
// Code mới không thấy dùng KI, nhưng giữ define nếu cần mở rộng sau này
#define PID_KI 0.0

// // --- 5. Hệ thống ---
// // Chọn 1 trong 2: set VEHICLE_ID = 1 hoặc 2
// #define VEHICLE_ID 1
// #if (VEHICLE_ID == 1)
//   #define MQTT_CLIENT_ID        "ESP32_GM65_Client_01"
//   #define MQTT_TOPIC_SUBSCRIBE  "car/V1/command"
//   #define MQTT_TOPIC_PUBLISH    "car/V1/matrix_position"
// #elif (VEHICLE_ID == 2)
//   #define MQTT_CLIENT_ID        "ESP32_GM65_Client_02"
//   #define MQTT_TOPIC_SUBSCRIBE  "car/V2/command"
//   #define MQTT_TOPIC_PUBLISH    "car/V2/matrix_position"
// #else
//   #error "Invalid VEHICLE_ID"
// #endif


#define MQTT_TOPIC_DEBUG      "gm65/debug"

// --- 6. Bluetooth Debug ---
#define DEBUG_RX_PIN 13 // Nối vào TX của HC-05
#define DEBUG_TX_PIN 14 // Nối vào RX của HC-05
#define DEBUG_BAUD   9600 // HC-05 thường mặc định là 9600

// --- 7. Led State ----
#define RED_LED 25
#define YELLOW_LED 26
#define GREEN_LED 27

// --- 8. ULTRASONIC ----
#define ULTRASONIC_TRIG 21
#define ULTRASONIC_ECHO 15

// --- 9. OBSTACLE / ULTRASONIC LOGIC ----
#define OBSTACLE_DISTANCE_CM           4.0f
#define OBSTACLE_SAMPLES               3
#define OBSTACLE_CONFIRM_HITS          2
#define OBSTACLE_POLL_MS               80
#define OBSTACLE_COOLDOWN_MS           2500
#define OBSTACLE_BACK_ARM_DELAY_MS     3400

// Stack size 
#define QUEUE_LENGTH 10
#define NETWORK_STACK_SIZE (1024 * 6)
#define SCANNER_STACK_SIZE (1024 * 4)
#define LINE_STACK_SIZE    (1024 * 4)

#endif