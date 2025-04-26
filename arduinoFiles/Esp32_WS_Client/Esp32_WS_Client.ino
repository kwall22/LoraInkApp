//Last Used with Heltec WiFi LoRa 32(V3) board
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include "esp_eap_client.h"
#include <Wire.h> 
#include <Arduino.h> 
#include <SPI.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include <map>

#define UTAH_TECH_WIFI 0 // 0 or 1
#if UTAH_TECH_WIFI
const char* ssid     = "";
const char* identity = "";
#else
//Need these for anything to work
const char* ssid = ""; //WiFi Name
const char* password = ""; //WiFi Pw
#endif
// ca_pem, crt_pem, key_pw, key_pem for Utah Tech WiFi
// const char* ca_pem = R"EOF(
// -----BEGIN CERTIFICATE-----
// -----END CERTIFICATE-----
// -----BEGIN CERTIFICATE-----
// -----END CERTIFICATE-----
// )EOF";

// const char* crt_pem = R"EOF(
// -----BEGIN CERTIFICATE-----
// -----END CERTIFICATE-----
// )EOF";

// const char* key_pw = "secret";
// const char* key_pem = R"EOF(
// -----BEGIN ENCRYPTED PRIVATE KEY-----
// -----END ENCRYPTED PRIVATE KEY-----
// )EOF";

//Cert for Railway App
const char ssl_ca_cert[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ
DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG
AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy
6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw
SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP
Xzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB
hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB
/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU
ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC
hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG
A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN
AQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1y
v4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE38
01S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1
e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtn
UfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoV
aneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+Z
WghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/R
PBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/q
pdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo
6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjV
uYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA
-----END CERTIFICATE-----
)EOF";

// WEBSOCKETS:
using namespace websockets;
const char* websockets_server_host = "wss://loraink.up.railway.app/";

WebsocketsClient client;

// LORA:
#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_SCK 9

#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13

SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);

volatile bool loraMessageReceived = false;

#define PACKET_TYPE_WAKEUP 0xA1
#define PACKET_TYPE_HEADER_FOOTER 0xB2
#define PACKET_TYPE_EVENT 0xC3
#define PACKET_TYPE_STATUS 0xD4
#define PACKET_TYPE_SYNC 0xE5

enum class EventColor : uint8_t {
  RED = 0,
  YELLOW = 1,
  BLUE = 2,
  GREEN = 3,
  ORANGE = 4
};

#pragma pack(1)
struct SyncCountData {
  uint16_t eInkId;
  uint8_t eventCount;
};
#pragma pack()

struct EventData {
  uint16_t eInkId;
  uint8_t dayOfWeek;
  uint16_t startTimeMinutes;
  uint16_t endTimeMinutes;
  uint8_t backgroundColor;
  char description[32];
};

#pragma pack(1)
struct HeaderFooterData {
  uint16_t eInkId;
  uint8_t headerLength;
  char header[100];
  uint8_t footerLength;
  char footer[100];
};
#pragma pack()

struct StatusMessageData {
  uint16_t eInkId;
  char message[150];
};

int timeStringToMinutes(const char* timeStr) {
  int hour, minute;
  sscanf(timeStr, "%d:%d", &hour, &minute);
  return hour * 60 + minute;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    radio.setFrequency(910.125);
    radio.setSpreadingFactor(9);
    radio.setBandwidth(250.0);
    radio.setCodingRate(5);
    radio.setSyncWord(0x12);
    radio.setCRC(2);
    Serial.println(F("lora success!"));
  } else {
    Serial.print(F("lora failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  WiFi.mode(WIFI_STA);
#if UTAH_TECH_WIFI
  // esp_wifi_sta_enterprise_enable();
  // esp_eap_client_set_identity((const uint8_t*) identity, strlen(identity));
  // esp_eap_client_set_ca_cert((const uint8_t*) ca_pem, strlen(ca_pem)+1);
  // esp_eap_client_set_certificate_and_key(
  //   (const uint8_t*) crt_pem, strlen(crt_pem)+1,
  //   (const uint8_t*) key_pem, strlen(key_pem)+1,
  //   (const uint8_t*) key_pw, strlen(key_pw));
  // WiFi.begin(ssid);
  delay(10);
#else
  WiFi.begin(ssid, password);
#endif

  int attempts = 0;
  Serial.print("WiFi connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 60) {
      Serial.println();
      Serial.println("Failed to connect!");
      return;
    }
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  Serial.println("Connecting to WS server.");

  client.setCACert(ssl_ca_cert);
  bool connected = client.connect("wss://loraink.up.railway.app/");
  if(connected) {
    Serial.println("Connected! to wss");
    DynamicJsonDocument doc(1024);  
    doc["type"] = "identify"; 
    doc["clientId"] = "esp32";
    char buffer[1024];
    size_t len = serializeJson(doc, buffer);
    client.send(buffer, len);
  } else {
    Serial.println("Not Connected to server!");
  }
  
  client.onMessage([&](WebsocketsMessage message){
    Serial.print("Got Message from WSS: ");
    Serial.println(message.data());
    Serial.println(F("sending to lora"));
    String fullMessage = message.data();
    processJson(fullMessage);
  });
}

void processJson(String jsonString) {
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* eInkId = doc["eInkId"];
  const char* messageType = doc["messageType"];

  if (strcmp(messageType, "schedule") == 0) {
    Serial.println("message type is schedule");
    const char* header = doc["header"];
    const char* footer = doc["footer"];
    JsonArray events = doc["events"];
    sendScheduleOverLoRa(eInkId, header, footer, events);
  }

  if (strcmp(messageType, "status") == 0) {
    Serial.println("message type is status");
    const char* message = doc["message"];
    sendStatusMessageOverLoRa(eInkId, message);
  }
}

void sendTypeWakeupPacket(){
  uint8_t packet = PACKET_TYPE_WAKEUP;
  int state = radio.transmit(&packet, sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("wakeup send fail"); Serial.println(state);
  } else {
    Serial.print("wakeup sent!");
  }
}

void sendTypeSyncCountPacket(SyncCountData data) {
  uint8_t packet[sizeof(SyncCountData) + 1];
  packet[0] = PACKET_TYPE_SYNC;
  memcpy(packet + 1, &data, sizeof(SyncCountData));
  int state = radio.transmit(packet, sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("sync data send fail "); Serial.println(state);
  } else {
    Serial.print("sync sent!");
  }
}

void sendTypeHeaderFooterPacket(HeaderFooterData data) {
  uint8_t packet[sizeof(HeaderFooterData) + 1];
  packet[0] = PACKET_TYPE_HEADER_FOOTER;
  memcpy(packet + 1, &data, sizeof(HeaderFooterData));
  int state = radio.transmit(packet, sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Header footer data send fail"); Serial.println(state);
  } else {
    Serial.print("header footer sent!");
  }
}

void sendTypeEventDataPacket(EventData data) {
  uint8_t packet[sizeof(EventData) + 1];
  packet[0] = PACKET_TYPE_EVENT;
  memcpy(packet + 1, &data, sizeof(EventData));
  int state = radio.transmit(packet, sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("event data send fail"); Serial.println(state);
  } else {
    Serial.print("event sent!");
  }
}

void sendTypeStatusMessageData (StatusMessageData data){
  uint8_t packet[sizeof(StatusMessageData) + 1];
  packet[0] = PACKET_TYPE_STATUS;
  memcpy(packet + 1, &data, sizeof(StatusMessageData));
  int state = radio.transmit(packet, sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("status message data send fail"); Serial.println(state);
  } else {
    Serial.print("status sent!");
  }
}

void sendScheduleOverLoRa(const char* eInkId, const char* header, const char* footer, JsonArray events) {

  sendTypeWakeupPacket();
  delay(1000);

  SyncCountData syncDataToSend;
  uint8_t eventNum = events.size();
  if (events.size() > 255) {
    Serial.println("error: too many events, max 255");
    eventNum = 255;
  }
  Serial.println(strlen(eInkId));
  syncDataToSend.eventCount = eventNum;
  Serial.println(syncDataToSend.eventCount);
  sscanf(eInkId, "%4hx", &syncDataToSend.eInkId);
  Serial.println(syncDataToSend.eventCount);

  sendTypeSyncCountPacket(syncDataToSend);
  delay(2000);

  HeaderFooterData headerFooterToSend;
  sscanf(eInkId, "%4hx", &headerFooterToSend.eInkId);
  Serial.println(headerFooterToSend.eInkId, HEX);
  headerFooterToSend.headerLength = strlen(header);
  if (headerFooterToSend.headerLength >= sizeof(headerFooterToSend.header)) {
    headerFooterToSend.headerLength = sizeof(headerFooterToSend.header) - 1;
  }
  strncpy(headerFooterToSend.header, header, headerFooterToSend.headerLength);
  headerFooterToSend.header[headerFooterToSend.headerLength] = '\0';
  headerFooterToSend.footerLength = strlen(footer);
  if (headerFooterToSend.footerLength >= sizeof(headerFooterToSend.footer)) {
    headerFooterToSend.footerLength = sizeof(headerFooterToSend.footer) - 1;
  }
  strncpy(headerFooterToSend.footer, footer, headerFooterToSend.footerLength);
  headerFooterToSend.footer[headerFooterToSend.footerLength] = '\0';

  sendTypeHeaderFooterPacket(headerFooterToSend);
  delay(2000);

  for (JsonObject event : events) {
    EventData eventData;
    sscanf(eInkId, "%4hx", &eventData.eInkId);
    Serial.print("Sending eInkId uint16_t: ");
    Serial.println(eventData.eInkId, HEX);
    eventData.dayOfWeek = event["dayOfWeek"];
    eventData.startTimeMinutes = timeStringToMinutes(event["startTime"]);
    eventData.endTimeMinutes = timeStringToMinutes(event["endTime"]);
    strncpy(eventData.description, event["description"], sizeof(eventData.description) - 1);
    eventData.description[sizeof(eventData.description) - 1] = '\0';

    const char* colorStr = event["color"];
    if (strcmp(colorStr, "RED") == 0) {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::RED);
    } else if (strcmp(colorStr, "YELLOW") == 0) {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::YELLOW);
    } else if (strcmp(colorStr, "BLUE") == 0) {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::BLUE);
    } else if (strcmp(colorStr, "GREEN") == 0) {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::GREEN);
    } else if (strcmp(colorStr, "ORANGE") == 0) {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::ORANGE);
    } else {
      eventData.backgroundColor = static_cast<uint8_t>(EventColor::GREEN);
      Serial.println("Warning: Invalid color in JSON, defaulting to GREEN");
    }
    
    Serial.print("background color about to be sent: ");
    Serial.println(eventData.backgroundColor);
    
    sendTypeEventDataPacket(eventData);
    delay(500);
  }
}

void sendStatusMessageOverLoRa(const char* eInkId, const char* message) {
  sendTypeWakeupPacket();
  delay(1000);
  StatusMessageData statusDataToSend;
  sscanf(eInkId, "%4hx", &statusDataToSend.eInkId);
  Serial.print("Sending eInkId uint16_t: ");
  Serial.println(statusDataToSend.eInkId, HEX);
  
  strncpy(statusDataToSend.message, message, sizeof(statusDataToSend.message) - 1);
  statusDataToSend.message[sizeof(statusDataToSend.message) - 1] = '\0';

  sendTypeStatusMessageData(statusDataToSend);
  delay(500);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
  }
  if(client.available()) {
    client.poll();
  } else {
    Serial.println("Disconnected from server. Reconnecting...");
    bool connected = client.connect("wss://loraink.up.railway.app/");
    if (connected) {
      Serial.println("Reconnected to WebSocket server!");
      DynamicJsonDocument doc(1024);  
      doc["type"] = "identify";
      doc["clientId"] = "esp32";
      char buffer[1024];
      size_t len = serializeJson(doc, buffer);
      client.send(buffer, len);
    } else {
      Serial.println("Reconnection failed!");
    }
  }
  delay(500);
}
