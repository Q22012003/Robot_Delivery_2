#include <Arduino.h>
#include "Config.h"
#include "SharedData.h"
#include "NetworkTask.h"
#include "ScannerTask.h"

// Khởi tạo biến toàn cục Queue
QueueHandle_t scannerQueue;

void setup() {
    Serial.begin(115200);
    
    // 1. Tạo hàng đợi (Queue)
    // Chứa tối đa QUEUE_LENGTH phần tử, mỗi phần tử có kích thước sizeof(ScannerMessage)
    scannerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(ScannerMessage));

    if (scannerQueue == NULL) {
        Serial.println("Error creating the queue");
        return;
    }

    // 2. Tạo Task Network (Chạy Core 0 để tách biệt với việc điều khiển)
    // AWS SSL rất tốn Stack, cần cấp phát hào phóng (NETWORK_STACK_SIZE)
    xTaskCreatePinnedToCore(
        TaskNetwork,        // Hàm task
        "NetworkTask",      // Tên task
        NETWORK_STACK_SIZE, // Stack size (bytes)
        NULL,               // Tham số truyền vào
        1,                  // Mức ưu tiên (Thấp hơn các task điều khiển motor)
        NULL,               // Handle
        0                   // Chạy trên Core 0
    );

    // 3. Tạo Task Scanner (Chạy Core 1 - cùng core với Arduino loop mặc định)
    xTaskCreatePinnedToCore(
        TaskScanner,
        "ScannerTask",
        SCANNER_STACK_SIZE,
        NULL,
        1,
        NULL,
        1                   // Chạy trên Core 1
    );

    Serial.println("FreeRTOS Tasks Started...");
}

void loop() {
    // Trong FreeRTOS, loop() không cần làm gì cả, 
    // hoặc dùng để giám sát hệ thống (Watchdog).
    vTaskDelete(NULL); // Xóa task loop mặc định để tiết kiệm RAM
}