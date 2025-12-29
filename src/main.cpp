#include <Arduino.h>
#include "Config.h"
#include "SharedData.h"
#include "NetworkTask.h"
#include "ScannerTask.h"
#include "LineTask.h"
#include "DebuggerBLE.h"

// [THÊM 2 DÒNG NÀY ĐỂ TẮT BROWNOUT]
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

QueueHandle_t scannerQueue;
char currentTarget[64] = ""; 
volatile RobotCommand currentCommand = CMD_STOP; 
// [THÊM] Khởi tạo false
volatile bool runBlind = false;

void setup() {
    // [THÊM DÒNG NÀY NGAY ĐẦU HÀM SETUP]
    // Tắt bộ phát hiện điện áp thấp -> ESP32 sẽ không reset khi đề pa motor
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
    pinMode(2, OUTPUT); // [THÊM] Cấu hình chân LED onboard
    digitalWrite(2, HIGH); // Bật sáng báo hiệu "Đã có điện"
    digitalWrite(2, LOW); // Mặc định tắt
    Serial.begin(115200);
    setupDebug();
    
    scannerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(ScannerMessage));
    
    // Core 0: Network
    xTaskCreatePinnedToCore(TaskNetwork, "NetworkTask", NETWORK_STACK_SIZE, NULL, 1, NULL, 0); 
    // Core 1: Scanner & Line
    xTaskCreatePinnedToCore(TaskScanner, "ScannerTask", SCANNER_STACK_SIZE, NULL, 1, NULL, 1); 
    xTaskCreatePinnedToCore(TaskLine,    "LineTask",    LINE_STACK_SIZE,    NULL, 2, NULL, 1); 

    // Serial.println("All Tasks Started... Brownout Detector DISABLED.");
    debug_printf("All Tasks Started... Brownout Detector DISABLED.\n");
    debug_printf("System Ready. Waiting for commands...\n");
}

void loop() {
    vTaskDelete(NULL); 
}