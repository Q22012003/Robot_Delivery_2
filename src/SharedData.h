// SharedData.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct ScannerMessage {
    char barcode[64];
};

// Định nghĩa các lệnh di chuyển
enum RobotCommand {
    CMD_STOP,
    CMD_FORWARD,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT
};

extern QueueHandle_t scannerQueue;
extern char currentTarget[64];

// [THÊM] Biến toàn cục chia sẻ lệnh di chuyển (Atomic hoặc dùng Queue cũng được, ở đây dùng biến cho đơn giản)
extern volatile RobotCommand currentCommand; 

#endif