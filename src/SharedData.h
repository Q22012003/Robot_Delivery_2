// SharedData.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct ScannerMessage {
    char barcode[64];
};

enum RobotCommand {
    CMD_STOP,
    CMD_FORWARD,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT,
    CMD_BACK   
};

// ===== HOLD from backend =====
extern volatile bool holdActive;
extern volatile uint32_t holdUntilMs;

// ===== LED / Mission state =====
// false: đang đi giao hàng (LED ĐỎ)
// true : đã giao hàng, đang quay về (LED XANH)
extern volatile bool hasDelivered;

extern QueueHandle_t scannerQueue;
extern char currentTarget[64];
extern volatile RobotCommand currentCommand; 
extern volatile bool runBlind;

// - Scanner sẽ DISARM ngay khi đọc được QR hợp lệ
// - Khi nhận lệnh STEP (FORWARD/LEFT/RIGHT/BACK), NetworkTask sẽ set thời điểm mở lại
extern volatile uint32_t scannerUnlockAtMs;
extern volatile bool scannerArmed;

// [THÊM DÒNG NÀY] Biến cờ báo hiệu đã Calib xong chưa
extern volatile bool isCalibrated; 
// ===== ADD: Heading state để reset hướng khi về bến =====
enum RobotHeading {
    HEADING_NORTH = 0,
    HEADING_EAST  = 1,
    HEADING_SOUTH = 2,   // mặc định: hướng "đi vào trong map" (1.1 -> 2.1)
    HEADING_WEST  = 3
};

extern volatile RobotHeading currentHeading;
extern volatile bool needHeadingAlign;
extern volatile RobotHeading desiredHeading;
#endif