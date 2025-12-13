#include "ScannerTask.h"
#include "Config.h"
#include "SharedData.h"

HardwareSerial SerialGM65(2);

void setupScanner() {
    SerialGM65.begin(GM65_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
}

void TaskScanner(void *pvParameters) {
    // Setup chạy 1 lần ở đây nếu muốn
    setupScanner();
    
    for (;;) { // Vòng lặp vô tận thay cho loop()
        if (SerialGM65.available()) {
            String rawData = SerialGM65.readStringUntil('\n');
            rawData.trim();

            if (rawData.length() > 0) {
                Serial.println("[SCANNER] Read: " + rawData);

                // Đóng gói dữ liệu
                ScannerMessage msg;
                // Copy chuỗi an toàn, tránh tràn bộ nhớ
                strncpy(msg.barcode, rawData.c_str(), sizeof(msg.barcode) - 1);
                msg.barcode[sizeof(msg.barcode) - 1] = '\0'; // Đảm bảo null-terminated

                // Gửi vào Queue (chờ 10ms nếu Queue đầy)
                xQueueSend(scannerQueue, &msg, pdMS_TO_TICKS(10));
            }
        }
        
        // Delay nhẹ để nhường CPU cho task khác nếu không có dữ liệu
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}