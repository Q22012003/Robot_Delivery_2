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
volatile bool scannerArmed = true;           // mặc định bật scanner
volatile uint32_t scannerUnlockAtMs = 0;     // 0 = chưa có lịch mở lại
volatile bool runBlind = false;              //Khởi tạo false
volatile bool isCalibrated = false;          // [THÊM DÒNG NÀY] Khởi tạo cờ là false
volatile RobotHeading currentHeading = HEADING_SOUTH;
volatile bool needHeadingAlign = false;
volatile RobotHeading desiredHeading = HEADING_SOUTH;
volatile bool holdActive = false;
volatile uint32_t holdUntilMs = 0;

void setup() {

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Tắt bộ phát hiện điện áp thấp -> ESP32 sẽ không reset khi đề pa motor
    pinMode(2, OUTPUT); // [THÊM] Cấu hình chân LED onboard
    digitalWrite(2, HIGH); // Bật sáng báo hiệu "Đã có điện"
    digitalWrite(2, LOW); // Mặc định tắt

     // ====== THÊM: Init đèn giao thông, vừa bật nguồn thì ĐỎ sáng ======
    pinMode(RED_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    digitalWrite(RED_LED, HIGH);     // ĐỎ sáng
    digitalWrite(YELLOW_LED, LOW);   // VÀNG tắt
    digitalWrite(GREEN_LED, LOW);    // XANH tắt

    Serial.begin(115200);
    setupDebug();
    
    scannerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(ScannerMessage));
    
    // Core 0: Network
    xTaskCreatePinnedToCore(TaskNetwork, "NetworkTask", NETWORK_STACK_SIZE, NULL, 1, NULL, 0); 

    // Core 1: Scanner & Line
    xTaskCreatePinnedToCore(TaskScanner, "ScannerTask", SCANNER_STACK_SIZE, NULL, 1, NULL, 1); 
    xTaskCreatePinnedToCore(TaskLine,    "LineTask",    LINE_STACK_SIZE,    NULL, 2, NULL, 1); 
   // xTaskCreatePinnedToCore(TaskDebugBLE,"DebugBLE",    2048,              NULL, 1, NULL, 0);

    debug_printf("All Tasks Started... Brownout Detector DISABLED.\n");
    debug_printf("System Ready. Waiting for commands...\n");
}

void loop() {
    vTaskDelete(NULL); 
}