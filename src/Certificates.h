#ifndef CERTIFICATES_H
#define CERTIFICATES_H

// WiFi
#ifndef WIFI
 //#define WIFI 1 
 #define WIFI 3
#endif
#if (WIFI == 1)
static const char* WIFI_SSID = "D906_TEC";
static const char* WIFI_PASSWORD = "D906_TEC";

#elif (WIFI == 2)
static const char* WIFI_SSID = "MINH QUI";
static const char* WIFI_PASSWORD = "99999999";

#elif (WIFI == 3)
static const char* WIFI_SSID = "DATN@TDTU";
static const char* WIFI_PASSWORD = "DATN@TDTU";

#endif
// AWS IoT endpoint
static const char* AWS_IOT_ENDPOINT = "a33b04z4mm5umj-ats.iot.us-east-1.amazonaws.com";


#ifndef VEHICLE_ID 
  #define VEHICLE_ID 3
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

#elif (VEHICLE_ID == 3)
  #define MQTT_CLIENT_ID        "ESP32_GM65_Client_03"
  #define MQTT_TOPIC_SUBSCRIBE  "car/V3/command"
  #define MQTT_TOPIC_PUBLISH    "car/V3/matrix_position"
  #include "Certificates_V2.h"

#else
  #error "Invalid VEHICLE_ID (must be 1 or 2)"
#endif

#endif // CERTIFICATES_H
