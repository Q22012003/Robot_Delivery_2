// DebuggerBLE.h
#ifndef DEBUGGERBLE_H
#define DEBUGGERBLE_H

#include <Arduino.h>

// Khai báo đối tượng Serial phần cứng (UART 1)
extern HardwareSerial SerialHC05;

// Hàm khởi tạo Debug
void setupDebug();

// Hàm in log ra cả 2 nơi (USB + HC05)
void debug_printf(const char *format, ...);

// Hàm in log thường
void debug_println(String msg);

#endif