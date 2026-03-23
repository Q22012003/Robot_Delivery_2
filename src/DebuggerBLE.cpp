// DebuggerBLE.cpp
#include "DebuggerBLE.h"
#include "Config.h"
#include <stdarg.h>

HardwareSerial SerialHC05(1); 

void setupDebug() {
    // Serial.begin(115200); // Đã gọi ở Main, không cần gọi lại
    
    // Khởi động HC-05
    SerialHC05.begin(DEBUG_BAUD, SERIAL_8N1, DEBUG_RX_PIN, DEBUG_TX_PIN);
    
    Serial.println("------------------------------------");
    Serial.println(">>> USB Debug Ready");
    Serial.printf(">>> HC-05 Started on RX:%d TX:%d\n", DEBUG_RX_PIN, DEBUG_TX_PIN);
    Serial.println("------------------------------------");

    SerialHC05.println("ESP32 CONNECTED VIA HC-05 OK!");
}

// --- HÀM NÀY ĐÃ ĐƯỢC NÂNG CẤP ---
void debug_printf(const char *format, ...) {
    char loc_buf[256]; 
    char * temp = loc_buf;
    
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    va_end(arg);
    
    // Duyệt qua từng ký tự để in
    for(int i = 0; i < len; i++) {
        // Nếu gặp ký tự xuống dòng (\n)
        if(temp[i] == '\n') {
            // Thì in thêm ký tự về đầu dòng (\r) trước
            Serial.write('\r');
            SerialHC05.write('\r');
        }
        // Sau đó in ký tự bình thường
        Serial.write(temp[i]);
        SerialHC05.write(temp[i]);
    }
}

void debug_println(String msg) {
    // println của Arduino tự động có \r\n nên không cần sửa
    Serial.println(msg);
    SerialHC05.println(msg);
}

void TaskDebugBLE(void* pv) {
    uint32_t lastBeat = 0;
    for(;;) {
        uint32_t now = millis();
        if (now - lastBeat >= 1000) {
            // heartbeat để biết chắc UART->HC05 đang chạy
            SerialHC05.printf("[BT] alive ms=%lu\r\n", (unsigned long)now);
            lastBeat = now;
        }

        // echo test: gõ vào MoBaXterm sẽ thấy phản hồi lại
        while (SerialHC05.available()) {
            char c = (char)SerialHC05.read();
            Serial.write(c);          // xem trên USB nếu cần
            SerialHC05.write(c);      // echo lại BT
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}