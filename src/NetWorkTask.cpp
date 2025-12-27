// NetworkTask.cpp
#include "NetworkTask.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include "Config.h"
#include "Certificates.h"
#include "SharedData.h"

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
    
    Serial.print("[MQTT] Received: ");
    Serial.println(message);

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
                // A. Gửi xác nhận "CMD_OK" lên AWS ngay lập tức
                // Để Server biết xe đã nhận lệnh và đang chuẩn bị chạy
                String ackPayload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                                    "\",\"status\":\"CMD_OK\"," + 
                                    "\"received_dir\":\"" + String(dir) + "\"}";
                client.publish(MQTT_TOPIC_PUBLISH, ackPayload.c_str());

                // B. Delay 1.5s - 2s (Thời gian chờ ra quyết định/ổn định xe)
                Serial.println("=> Command Received. Waiting 1.5s to execute...");
                // vTaskDelay(pdMS_TO_TICKS(1500)); 

                // C. Thực thi lệnh
                if (strcmp(dir, "LEFT") == 0) {
                    Serial.println("=> ACTION: TURN LEFT");
                    currentCommand = CMD_TURN_LEFT; 
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                } 
                else if (strcmp(dir, "RIGHT") == 0) {
                    Serial.println("=> ACTION: TURN RIGHT");
                    currentCommand = CMD_TURN_RIGHT; 
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                }
                else if (strcmp(dir, "FORWARD") == 0) {
                    Serial.println("=> ACTION: GO STRAIGHT");
                    xQueueReset(scannerQueue);
                    currentCommand = CMD_FORWARD;
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                }
                else {
                    currentCommand = CMD_STOP; 
                    runBlind = true; // [THÊM] Báo hiệu quay mù
                }
            }
        }
        // --- TRƯỜNG HỢP 2: LỆNH DỪNG KHẨN CẤP (STOP) ---
        else if (type && strcmp(type, "STOP") == 0) {
             currentCommand = CMD_STOP;
             Serial.println("=> Command: STOP");
             client.publish(MQTT_TOPIC_PUBLISH, "{\"status\":\"STOPPED_EMERGENCY\"}");
        }
    } else {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
    }
}

void connectWiFi() {
    const TickType_t timeout = pdMS_TO_TICKS(10000); // 10s
    TickType_t startTick = xTaskGetTickCount();

    if (WiFi.status() == WL_CONNECTED) return;
    Serial.print("Connecting WiFi...");
    
    WiFi.mode(WIFI_STA);
    
    //Giảm công suất WiFi xuống 8.5dBm( ) (Mặc định là 20dBm rất tốn điện)
    wifi_power_t txPower = WIFI_POWER_19_5dBm;
    WiFi.setTxPower(txPower); 
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      if ((xTaskGetTickCount() - startTick) > timeout) {
       Serial.println("\nTime out Wifi Connected");
       return;
    }
        vTaskDelay(pdMS_TO_TICKS(500));
        blinkLED(100);
        Serial.print(".");
    }

    if (txPower == WIFI_POWER_19_5dBm ) {
      Serial.println("\nWiFi Connected at WIFI_POWER_19_5dBm");
    }
    else {
      Serial.println("\nWiFi Connected low Power Mode"); 
    }
}

void connectAWS() {
    net.setCACert(AWS_ROOT_CA);
    Serial.println("AWS_ROOT_CA Done");
    net.setCertificate(DEVICE_CERT);
    Serial.println("DEVICE_CERT Done");
    net.setPrivateKey(DEVICE_PRIVATE_KEY);
    Serial.println("DEVICE_PRIVATE_KEY Done");

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    Serial.println("AWS_IOT_ENDPOINT Done");
    client.setCallback(mqttCallback);

    Serial.print("Connecting AWS...");
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID)) {
            Serial.println("AWS Connected!");
            client.subscribe(MQTT_TOPIC_SUBSCRIBE);
            digitalWrite(2, HIGH);
        } else {
            Serial.print(".");
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
            Serial.print("[SCANNER] Read: " + scannedPos);

            bool isCorrect = false;
            if (strcmp(currentTarget, "") != 0) { 
                if (scannedPos.equals(String(currentTarget))) {
                    Serial.println(" -> [MATCH] REACHED TARGET! STOPPING.");
                    currentCommand = CMD_STOP; 
                    strcpy(currentTarget, ""); 
                    isCorrect = true;
                } else {
                    Serial.printf(" -> [INFO] Passing node %s (Target: %s)\n", scannedPos.c_str(), currentTarget);
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
                Serial.println("[NETWORK] Sent status: " + statusStr);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}