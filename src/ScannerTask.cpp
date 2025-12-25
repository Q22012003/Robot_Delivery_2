#include "ScannerTask.h"
#include "Config.h"
#include "SharedData.h"

HardwareSerial SerialGM65(2);

// [THÊM] Biến lưu mã cũ để chống trùng
String lastBarcode = "";
unsigned long lastScanTime = 0;

void setupScanner() {
    SerialGM65.begin(GM65_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
}

void TaskScanner(void *pvParameters) {
    setupScanner();
    
    for (;;) { 
        if (SerialGM65.available()) {
            String rawData = SerialGM65.readStringUntil('\n');
            rawData.trim();

            // Chỉ xử lý nếu có dữ liệu
            if (rawData.length() > 0) {
                
                // --- [LOGIC CHỐNG SPAM / DEBOUNCE] ---
                // 1. Nếu mã MỚI khác mã CŨ: Chấp nhận ngay
                // 2. Nếu mã MỚI giống mã CŨ: Chỉ chấp nhận lại sau 3 giây (tránh trường hợp đứng im gửi liên tục)
                bool isNewCode = !rawData.equals(lastBarcode);
                bool isTimeout = (millis() - lastScanTime > 3000); // 3 giây reset

                if (isNewCode || isTimeout) {
                    
                    Serial.println("[SCANNER] New Code: " + rawData);

                    // Cập nhật trạng thái
                    lastBarcode = rawData;
                    lastScanTime = millis();

                    // Đóng gói gửi đi
                    ScannerMessage msg;
                    strncpy(msg.barcode, rawData.c_str(), sizeof(msg.barcode) - 1);
                    msg.barcode[sizeof(msg.barcode) - 1] = '\0';

                    xQueueSend(scannerQueue, &msg, pdMS_TO_TICKS(10));
                } 
                else {
                    // Debug: In ra để biết nó đang bỏ qua (có thể comment lại cho đỡ rối)
                     Serial.println("[SCANNER] Ignored duplicate: " + rawData);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}