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

// Hàm xử lý dữ liệu nhận từ AWS (Server gửi xuống)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("[MQTT] Received: ");
    Serial.println(message);

    // Parse JSON để lấy Target và Direction
    StaticJsonDocument<512> doc; // Tăng size buffer lên chút cho an toàn
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        const char* type = doc["type"];
        
        // --- TRƯỜNG HỢP 1: LỆNH DI CHUYỂN (STEP) ---
        if (type && strcmp(type, "STEP") == 0) {
            const char* target = doc["target"];
            const char* dir = doc["direction"]; // "FORWARD", "LEFT", "RIGHT"
            
            // 1. Cập nhật Target (để Scanner check khi tới nơi)
            if (target) {
                strncpy(currentTarget, target, sizeof(currentTarget) - 1);
                currentTarget[sizeof(currentTarget) - 1] = '\0';
            }

            // 2. Cập nhật Lệnh điều khiển cho LineTask chạy
            if (dir) {
                if (strcmp(dir, "FORWARD") == 0) {
                    currentCommand = CMD_FORWARD;
                } 
                else if (strcmp(dir, "LEFT") == 0) {
                    currentCommand = CMD_TURN_LEFT;
                } 
                else if (strcmp(dir, "RIGHT") == 0) {
                    currentCommand = CMD_TURN_RIGHT;
                }
                else {
                    // Mặc định nếu lệnh lạ thì dừng cho an toàn
                    currentCommand = CMD_STOP; 
                }
            }
            
            Serial.printf(">>> UPDATE: Target=%s | Cmd=%s\n", currentTarget, dir);
        }
        // --- TRƯỜNG HỢP 2: LỆNH DỪNG (STOP) ---
        else if (type && strcmp(type, "STOP") == 0) {
             currentCommand = CMD_STOP; // Ngắt motor ngay lập tức
             strcpy(currentTarget, ""); // Xóa target
             Serial.println(">>> STOP COMMAND RECEIVED");
        }
    } else {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
    }
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.print("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
}

void connectAWS() {
    net.setCACert(AWS_ROOT_CA);
    net.setCertificate(DEVICE_CERT);
    net.setPrivateKey(DEVICE_PRIVATE_KEY);

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.setCallback(mqttCallback);

    Serial.print("Connecting AWS...");
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID)) {
            Serial.println("AWS Connected!");
            client.subscribe(MQTT_TOPIC_SUBSCRIBE);
        } else {
            Serial.print(".");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void TaskNetwork(void *pvParameters) {
    connectWiFi();
    connectAWS();

    ScannerMessage incomingMsg;

    for (;;) {
        // Duy trì kết nối
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        if (!client.connected()) connectAWS();
        client.loop();

        // Kiểm tra dữ liệu từ Scanner gửi sang
        if (xQueueReceive(scannerQueue, &incomingMsg, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            String scannedPos = String(incomingMsg.barcode);
            Serial.print("[SCANNER] Read: " + scannedPos);

            // Kiểm tra xem mã quét được có đúng là đích đến hiện tại không
            bool isCorrect = false;
            if (strcmp(currentTarget, "") != 0) { // Nếu đang có target
                if (scannedPos.equals(String(currentTarget))) {
                    Serial.println(" -> [MATCH] REACHED TARGET!");
                    isCorrect = true;
                    
                    // Nếu đã tới đích, có thể tạm dừng xe để chờ lệnh mới từ Server
                    // currentCommand = CMD_STOP; // (Bỏ comment dòng này nếu muốn xe dừng hẳn khi quét trúng)
                } else {
                    Serial.printf(" -> [INFO] Passing node %s (Target: %s)\n", scannedPos.c_str(), currentTarget);
                }
            }

            // Gửi dữ liệu vị trí lên AWS
            String statusStr = isCorrect ? "ARRIVED" : "MOVING";
            
            String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                             "\",\"position\":\"" + scannedPos + 
                             "\",\"status\":\"" + statusStr + "\"}"; 
            
            if(client.publish(MQTT_TOPIC_PUBLISH, payload.c_str())){
                 Serial.println("[NETWORK] Pos Uploaded.");
            } else {
                 Serial.println("[NETWORK] Upload Failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Nhường CPU
    }
}