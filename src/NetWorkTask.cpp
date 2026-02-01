// NetworkTask.cpp
#include "NetworkTask.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "Config.h"
#include "Certificates.h"
#include "SharedData.h"
#include "DebuggerBLE.h"



WiFiClientSecure net;
PubSubClient client(net);


// Hàm hỗ trợ nháy đèn (Block code)
void blinkLED(int delayTime) {
    digitalWrite(2, HIGH);
    vTaskDelay(pdMS_TO_TICKS(delayTime));
    digitalWrite(2, LOW);
    vTaskDelay(pdMS_TO_TICKS(delayTime));
}

// --------------------------------------------------------------------------
// 1. XỬ LÝ LỆNH TỪ AWS GỬI XUỐNG (STEP)
// --------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    // Serial.print("[MQTT] Received: ");
    // Serial.println(message);

    debug_printf("[MQTT] Received: ");
    debug_println(message);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        const char* type = doc["type"];
        
        // --- TRƯỜNG HỢP 1: LỆNH DI CHUYỂN (STEP) ---
        if (type && strcmp(type, "STEP") == 0) {
            const char* dir = doc["direction"]; 
            const char* target = doc["target"];


            // Cập nhật mục tiêu mới (VD: "2,2")
            if (target) {
                strncpy(currentTarget, target, sizeof(currentTarget) - 1);
                currentTarget[sizeof(currentTarget) - 1] = '\0';
            }

            if (dir) {

                // ===== Scanner latch schedule =====
                // DISARM ngay khi nhận STEP, và hẹn ARM lại theo loại lệnh
                // (để tránh GM65 đọc lại QR cũ khi xe vừa bắt đầu chạy)
                uint32_t armDelayMs = 2000; // default cho FORWARD
                if (strcmp(dir, "LEFT") == 0 || strcmp(dir, "RIGHT") == 0) {
                    armDelayMs = 2000 + 80 + 700;   // quay 2s + stop 80ms + 700ms vào forward
                } else if (strcmp(dir, "BACK") == 0) {
                    armDelayMs = 2000 + 120 + 700;  // back spin 2s + stop 120ms + 700ms
                } else {
                    armDelayMs = 2000;               // FORWARD
                }

                scannerArmed = false;
                scannerUnlockAtMs = (uint32_t)(millis() + armDelayMs);
                debug_printf("[SCANNER] STEP received -> DISARM, will ARM in %lu ms\n", (unsigned long)armDelayMs);

                // A. Gửi xác nhận "CMD_OK" lên AWS ngay lập tức
                // Để Server biết xe đã nhận lệnh và đang chuẩn bị chạy
                String ackPayload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                                    "\",\"status\":\"CMD_OK\"," + 
                                    "\"received_dir\":\"" + String(dir) + "\"}";
                client.publish(MQTT_TOPIC_PUBLISH, ackPayload.c_str());

                // B. Delay 1.5s - 2s (Thời gian chờ ra quyết định/ổn định xe)
                //Serial.println("=> Command Received. Waiting 1.5s to execute...");
                debug_printf("=> Command Received. Waiting 1.5s to execute...");
                // vTaskDelay(pdMS_TO_TICKS(1500)); 

                // C. Thực thi lệnh
                if (strcmp(dir, "LEFT") == 0) {
                    //Serial.println("=> ACTION: TURN LEFT");
                    debug_printf("=> ACTION: TURN LEFT");
                    currentCommand = CMD_TURN_LEFT; 
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                    currentHeading = (RobotHeading)((currentHeading + 3) % 4); // LEFT = -90°
                } 
                else if (strcmp(dir, "RIGHT") == 0) {
                    //Serial.println("=> ACTION: TURN RIGHT");
                      debug_printf("=> ACTION: TURN RIGHT");
                    currentCommand = CMD_TURN_RIGHT; 
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                    currentHeading = (RobotHeading)((currentHeading + 1) % 4); // RIGHT = +90°
                }
                else if (strcmp(dir, "FORWARD") == 0) {
                    //Serial.println("=> ACTION: GO STRAIGHT");
                      debug_printf("=> ACTION: GO STRAIGHT");
                    //  xQueueReset(scannerQueue);
                    currentCommand = CMD_FORWARD;
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                }
                else if (strcmp(dir, "BACK") == 0) {
                   debug_printf("=> ACTION: BACK (U-TURN)\n");
                   currentCommand = CMD_BACK;
                   runBlind = true; // quay mù
                   currentHeading = (RobotHeading)((currentHeading + 2) % 4); // <<< quay 180°
                }

                else {
                    currentCommand = CMD_STOP; 
                    runBlind = false; // [THÊM] Báo hiệu quay mù
                }
            }
        }
        // --- TRƯỜNG HỢP 2: LỆNH DỪNG KHẨN CẤP (STOP) ---
        else if (type && strcmp(type, "STOP") == 0) {

     currentCommand = CMD_STOP;
     debug_printf("=> Command: STOP");

     // ===== ADD: nếu server muốn reset hướng =====
     bool resetHeading = doc["reset_heading"] | false;
     if (resetHeading) {
         desiredHeading = HEADING_SOUTH;  // hướng chuẩn để lần sau forward đi vào map
         needHeadingAlign = true;
         debug_printf(" [RESET_HEADING -> SOUTH]\n");
     }

     client.publish(MQTT_TOPIC_PUBLISH, "{\"status\":\"STOPPED_EMERGENCY\"}");
}
    } else {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
    }
}

void connectWiFi() {
    const TickType_t timeout = pdMS_TO_TICKS(10000); 
    TickType_t startTick = xTaskGetTickCount();

    if (WiFi.status() == WL_CONNECTED) return;
    
    // Thêm \n để xuống dòng đàng hoàng
    debug_printf("Connecting WiFi...\n"); 
    
    WiFi.mode(WIFI_STA);
    wifi_power_t txPower = WIFI_POWER_19_5dBm;
    WiFi.setTxPower(txPower); 
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        if ((xTaskGetTickCount() - startTick) > timeout) {
            debug_printf("Time out Wifi Connected\n"); // Xóa \n ở đầu, để ở cuối
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        blinkLED(100);
        // Bỏ in dấu chấm hoặc in dấu chấm thì phải chấp nhận nó dính
        // debug_printf("."); <--- Tạm thời comment dòng này cho log sạch
    }

    if (txPower == WIFI_POWER_19_5dBm ) {
      debug_printf("WiFi Connected (High Power)\n");
    }
    else {
      debug_printf("WiFi Connected (Low Power)\n");
    }
}

void connectAWS() {
    net.setCACert(AWS_CERT_CA);
    //Serial.println("AWS_ROOT_CA Done");
    debug_printf("AWS_ROOT_CA Done\n");
    net.setCertificate(AWS_CERT_CRT);
    //Serial.println("DEVICE_CERT Done");
    debug_printf("DEVICE_CERT Dones\n");
    net.setPrivateKey(AWS_CERT_PRIVATE);
    //Serial.println("DEVICE_PRIVATE_KEY Done");
    debug_printf("DEVICE_PRIVATE_KEY Done\n");

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    //Serial.println("AWS_IOT_ENDPOINT Done");
    debug_printf("AWS_IOT_ENDPOINT Done\n");
    client.setCallback(mqttCallback);

    //Serial.print("Connecting AWS...");
     debug_printf("Connecting AWS...\n");
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID)) {
            //Serial.println("AWS Connected!");
            debug_printf("AWS Connected!\n");
            client.subscribe(MQTT_TOPIC_SUBSCRIBE);
            digitalWrite(2, HIGH);
        } else {
            //Serial.print(".");
            debug_printf(".\n");
            // [THÊM] Nháy đèn nhanh để báo đang cố kết nối
            blinkLED(500); 
        }
    }
}

// --------------------------------------------------------------------------
// 2. VÒNG LẶP CHÍNH CỦA NETWORK TASK
// --------------------------------------------------------------------------
void TaskNetwork(void *pvParameters) {
    digitalWrite(2, LOW);
    // [THÊM ĐOẠN NÀY] Chờ cho đến khi LineTask Calib xong (biến cờ true)
    debug_println("[NetworkTask] Waiting for Calibration...");
    while (!isCalibrated) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Kiểm tra lại mỗi 100ms
    }
    debug_println("[NetworkTask] Calibration finished. Starting WiFi...");
    connectWiFi();
    connectAWS();

    ScannerMessage incomingMsg;

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        if (!client.connected()) connectAWS();
        // Nếu đã kết nối OK, Đèn phải SÁNG LIÊN TỤC
        if (WiFi.status() == WL_CONNECTED && client.connected()) {
             digitalWrite(2, HIGH);
        }
        client.loop();

        if (xQueueReceive(scannerQueue, &incomingMsg, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            String scannedPos = String(incomingMsg.barcode);
            //Serial.print("[SCANNER] Read: " + scannedPos);
              debug_println("[SCANNER] Read: " + scannedPos);
            bool isCorrect = false;
            if (strcmp(currentTarget, "") != 0) { 
                if (scannedPos.equals(String(currentTarget))) {
                    Serial.println(" -> [MATCH] REACHED TARGET! STOPPING.");
                    debug_printf(" -> [MATCH] REACHED TARGET! STOPPING.");
                    currentCommand = CMD_STOP; 
                    strcpy(currentTarget, ""); 
                    isCorrect = true;
                } else {
                    Serial.printf(" -> [INFO] Passing node %s (Target: %s)\n", scannedPos.c_str(), currentTarget);
                      debug_printf("-> [INFO] Passing node %s (Target: %s)\n", scannedPos.c_str(), currentTarget);
                }
            }

            // --- SỬA Ở ĐÂY ---
            // Cũ: String statusStr = isCorrect ? "ARRIVED" : "MOVING";
            // Mới: Đổi thành "OK" để khớp với Backend Node.js
            String statusStr = isCorrect ? "OK" : "MOVING"; 
            
            String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                             "\",\"position\":\"" + scannedPos + 
                             "\",\"status\":\"" + statusStr + "\"}"; 
            
            if (client.publish(MQTT_TOPIC_PUBLISH, payload.c_str())) {
               // Serial.println("[NETWORK] Sent status: " + statusStr);
               debug_println("[NETWORK] Sent status: " + statusStr);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}