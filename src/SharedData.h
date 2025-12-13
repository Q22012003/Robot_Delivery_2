#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Định nghĩa cấu trúc dữ liệu gửi vào hàng đợi (Queue)
struct ScannerMessage {
    char barcode[64]; // Độ dài tối đa của barcode
};

// Khai báo Queue dùng chung (định nghĩa thực tế nằm ở main.cpp)
extern QueueHandle_t scannerQueue;

#endif