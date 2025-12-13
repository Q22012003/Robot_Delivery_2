#include "NetworkTask.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Config.h"
#include "Certificates.h"
#include "SharedData.h"

WiFiClientSecure net;
PubSubClient client(net);

// --- Hàm phụ trợ (Private) ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Received on ");
    Serial.print(topic);
    Serial.print(": ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    // Xử lý lệnh điều khiển robot tại đây hoặc gửi vào Queue điều khiển khác
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    
    Serial.print("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500)); // Dùng vTaskDelay thay cho delay
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
}

void connectAWS() {
    // Cấu hình chứng chỉ
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

// --- Main Task Function ---
void TaskNetwork(void *pvParameters) {
    connectWiFi();
    connectAWS();

    ScannerMessage incomingMsg;

    for (;;) {
        // 1. Duy trì kết nối
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        if (!client.connected()) connectAWS();
        client.loop();

        // 2. Kiểm tra xem có dữ liệu từ Scanner gửi sang không
        // pdMS_TO_TICKS(10) có nghĩa là chờ tối đa 10ms để check hàng đợi
        if (xQueueReceive(scannerQueue, &incomingMsg, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            Serial.print("[NETWORK] Publishing: ");
            Serial.println(incomingMsg.barcode);

            String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                             "\",\"position\":\"" + String(incomingMsg.barcode) + "\"}";
            
            if(client.publish(MQTT_TOPIC_PUBLISH, payload.c_str())){
                 Serial.println("-> Sent OK");
            } else {
                 Serial.println("-> Send Failed");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Nhường CPU
    }
}