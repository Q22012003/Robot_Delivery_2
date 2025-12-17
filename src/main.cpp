// main.cpp
#include <Arduino.h>
#include "Config.h"
#include "SharedData.h"
#include "NetworkTask.h"
#include "ScannerTask.h"

QueueHandle_t scannerQueue;

// [THÊM] Khởi tạo biến target rỗng
char currentTarget[64] = ""; 

void setup() {
    Serial.begin(115200);
    
    scannerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(ScannerMessage));

    if (scannerQueue == NULL) {
        Serial.println("Error creating the queue");
        return;
    }

    xTaskCreatePinnedToCore(TaskNetwork, "NetworkTask", NETWORK_STACK_SIZE, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskScanner, "ScannerTask", SCANNER_STACK_SIZE, NULL, 1, NULL, 1);

    Serial.println("FreeRTOS Tasks Started...");
}

void loop() {
    vTaskDelete(NULL); 
}