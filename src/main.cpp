// main.cpp
#include <Arduino.h>
#include "Config.h"
#include "SharedData.h"
#include "NetworkTask.h"
#include "ScannerTask.h"
#include "LineTask.h" // [THÊM]

QueueHandle_t scannerQueue;
char currentTarget[64] = ""; 
volatile RobotCommand currentCommand = CMD_STOP; // Khởi tạo trạng thái dừng

void setup() {
    Serial.begin(115200);
    
    scannerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(ScannerMessage));
    
    // Tạo Task
    xTaskCreatePinnedToCore(TaskNetwork, "NetworkTask", NETWORK_STACK_SIZE, NULL, 1, NULL, 0); // Core 0 (Wifi/BLE)
    xTaskCreatePinnedToCore(TaskScanner, "ScannerTask", SCANNER_STACK_SIZE, NULL, 1, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(TaskLine,    "LineTask",    LINE_STACK_SIZE,    NULL, 2, NULL, 1); // Core 1 (Ưu tiên cao hơn Scanner chút)

    Serial.println("All Tasks Started...");
}

void loop() {
    vTaskDelete(NULL); 
}