#ifndef CERTIFICATES_H
#define CERTIFICATES_H

// WiFi
static const char* WIFI_SSID = "D902_2.4G";
static const char* WIFI_PASSWORD = "NhaD902@@";

// AWS IoT endpoint
static const char* AWS_IOT_ENDPOINT = "a33b04z4mm5umj-ats.iot.us-east-1.amazonaws.com";


#ifndef VEHICLE_ID 
  #define VEHICLE_ID 1
#endif

#if (VEHICLE_ID == 1)
  #define MQTT_CLIENT_ID        "ESP32_GM65_Client_01"
  #define MQTT_TOPIC_SUBSCRIBE  "car/V1/command"
  #define MQTT_TOPIC_PUBLISH    "car/V1/matrix_position"
  #include "Certificates_V1.h"

#elif (VEHICLE_ID == 2)
  #define MQTT_CLIENT_ID        "ESP32_GM65_Client_02"
  #define MQTT_TOPIC_SUBSCRIBE  "car/V2/command"
  #define MQTT_TOPIC_PUBLISH    "car/V2/matrix_position"
  #include "Certificates_V2.h"

#else
  #error "Invalid VEHICLE_ID (must be 1 or 2)"
#endif

#endif // CERTIFICATES_H
