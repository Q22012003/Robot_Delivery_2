#include "ScannerTask.h"
#include "Config.h"
#include "SharedData.h"
#include "DebuggerBLE.h"

static String lastSentCode = "";
static unsigned long lastSentMs = 0;
// ===== ADD: anti-bounce tuned for GM65 =====
static unsigned long lastAnyReadTime = 0;
static const unsigned long DUP_COOLDOWN_MS = 1000;   // khóa trùng trong ~0.9s (đủ chống dội, không miss)
static const unsigned long NO_QR_RESET_MS  = 600;   // nếu 200ms không thấy QR -> coi như đã rời QR, cho phép nhận lại

// ===== ADD: log throttle để khỏi spam khi đang DISARM =====

HardwareSerial SerialGM65(2);

static unsigned long lastDisarmLogMs = 0;

void setupScanner() {
    SerialGM65.begin(GM65_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    SerialGM65.setTimeout(20);
}

void TaskScanner(void *pvParameters) {
    setupScanner();

for (;;) {
        unsigned long now0 = millis();
            // ===== HOLD: mute scanner hoàn toàn =====
       if (holdActive) {
           scannerArmed = false;
           scannerUnlockAtMs = 0;
           vTaskDelay(pdMS_TO_TICKS(20));
           continue;
        }

      // ===== ARM lại nếu đã tới thời điểm mở khóa (do NetworkTask set) =====
    if (!scannerArmed && scannerUnlockAtMs != 0 && !holdActive) {
           if ((int32_t)(now0 - scannerUnlockAtMs) >= 0) {
                scannerArmed = true;
                scannerUnlockAtMs = 0;
                debug_println("[SCANNER] ARM (unlock)");
            }
       }

        // Nếu đã một lúc không thấy QR -> reset lastSentCode để lần tới cùng mã vẫn được nhận
       if (lastAnyReadTime != 0 && (now0 - lastAnyReadTime) > NO_QR_RESET_MS) {
            lastSentCode = "";
        }

    if (SerialGM65.available()) {
        String rawData = SerialGM65.readStringUntil('\n');
        rawData.trim();

        if (rawData.length() > 0) {
            unsigned long now = millis();

            // ===== FIX: update lastAnyReadTime mỗi khi có QR data =====
            lastAnyReadTime = now;

            // Nếu scanner đang DISARM -> bỏ qua hết (chỉ flush), tránh spam nhiều lần 1 vị trí
            if (!scannerArmed) {
                if (now - lastDisarmLogMs > 800) {
                    debug_println("[SCANNER] Ignored (DISARM): " + rawData);
                    lastDisarmLogMs = now;
                }
            } else {
            // ===== ANTI-BOUNCE (SAFE VERSION – KHÔNG PHÁ WEB) =====
            // 1. Nếu code khác -> nhận ngay
            // 2. Nếu code giống -> chỉ nhận lại sau 900ms

            bool isNewCode = !rawData.equals(lastSentCode);
            if (isNewCode) {
                debug_println("[SCANNER] New Code: " + rawData);

                lastSentCode = rawData;
                lastSentMs = now;

                ScannerMessage msg;
                strncpy(msg.barcode, rawData.c_str(), sizeof(msg.barcode) - 1);
                msg.barcode[sizeof(msg.barcode) - 1] = '\0';

                BaseType_t ok = xQueueSend(scannerQueue, &msg, pdMS_TO_TICKS(10));
                if (ok == pdTRUE) {
                    scannerArmed = false;
                    scannerUnlockAtMs = 0;
                    debug_println("[SCANNER] DISARM (latched after QR)");
                } else {
                    debug_println("[SCANNER] Queue FULL -> keep ARMED, will retry: " + rawData);
                }

                // ===== Drain buffer nhanh để bỏ QR lặp đã nằm sẵn trong UART =====
                uint32_t t0 = millis();
                while (millis() - t0 < 60) {
                    while (SerialGM65.available()) SerialGM65.read();
                    vTaskDelay(pdMS_TO_TICKS(1));
                }             
            } else {
                // đang đứng trên cùng QR -> ignore, nhưng vẫn GIỮ ARMED để chờ mã mới (vd: 3,2)
                debug_println("[SCANNER] Ignored same code: " + rawData);
            }
          } // end if(scannerArmed)            
        }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}