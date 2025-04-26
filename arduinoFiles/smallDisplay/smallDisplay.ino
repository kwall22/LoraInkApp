// Pick your panel  -  https://github.com/todd-herbert/heltec-eink-modules
//Last used with Heltec Wireless Paper board, v1_1 display
#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeMonoBoldOblique12pt7b.h"
#include "Fonts/FreeMonoOblique18pt7b.h"
#include "Fonts/FreeMonoOblique24pt7b.h"
#include <RadioLib.h>
#include <esp_sleep.h>
#include <vector>

#include "hal/efuse_hal.h"
#include "esp_mac.h"

EInkDisplay_WirelessPaperV1_1 display;
//uses esp32 S3 chip 
#define LORA_IRQ_PIN 3
#define EINK_UPDATE_PIN 4
#define LORA_PIN_MOSI 10
#define LORA_PIN_MISO 11
#define LORA_PIN_SCK 9

#define LORA_PIN_NSS     8 //esp32 v3 
#define LORA_PIN_DIO1   14 //esp32 v3
#define LORA_PIN_RST    12 //esp32 v3
#define LORA_PIN_BUSY   13 //esp32 v3 

SX1262 radio = new Module(LORA_PIN_NSS, LORA_PIN_DIO1, LORA_PIN_RST, LORA_PIN_BUSY);
volatile bool receivedFlag = false;
uint16_t localAddress = 0;
unsigned long lastPacketReceivedTime = 0;
const unsigned long PACKET_RECEIVE_TIMEOUT = 60000;

#define PACKET_TYPE_WAKEUP 0xA1
#define PACKET_TYPE_HEADER_FOOTER 0xB2
#define PACKET_TYPE_EVENT 0xC3
#define PACKET_TYPE_STATUS 0xD4
#define PACKET_TYPE_SYNC 0xE5

struct StatusMessageData {
  uint16_t eInkId;
  char message[150];
};

void setFlag(void) {
  receivedFlag = true;
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_GPIO:     Serial.println("Wakeup caused by GPIO"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup() {
  Serial.begin(9600);
  delay(200);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  localAddress = (mac[3] << 16) | (mac[4] << 8) | mac[5];
  Serial.printf("Local Lora address: %X %d\n", localAddress, localAddress);

  SPI.begin(LORA_PIN_SCK, LORA_PIN_MISO, LORA_PIN_MOSI, LORA_PIN_NSS);
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    radio.setFrequency(910.125);
    radio.setSpreadingFactor(9);
    radio.setBandwidth(250.0);
    radio.setCodingRate(5);
    radio.setSyncWord(0x12);
    radio.setCRC(2);
    Serial.println(F("LoRa initialized!"));
  } else {
    Serial.println(F("LoRa init failed!"));
    Serial.println(state);
    while (true);
  }

  radio.setPacketReceivedAction(setFlag);

  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true) { delay(10); }
  }

  pinMode(LORA_PIN_DIO1, INPUT);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)LORA_PIN_DIO1, 1);
}

std::vector<String> wordWrapToArray(String text, int maxWidth) {
  std::vector<String> lines;
  String currentLine = "";
  int currentWordStart = 0;

  for (int i = 0; i <= text.length(); i++) {
    char currentChar = (i < text.length()) ? text.charAt(i) : ' ';

    if (currentChar == ' ') {
      String currentWord = text.substring(currentWordStart, i);

      if (currentLine.length() == 0) {
        currentLine = currentWord;
      } else if (currentLine.length() + 1 + currentWord.length() <= maxWidth) {
        currentLine += " " + currentWord;
      } else {
        lines.push_back(currentLine);
        currentLine = currentWord;
      }
      currentWordStart = i + 1;
    }
  }
  if (currentLine.length() > 0) {
    lines.push_back(currentLine);
  }
  return lines;
}

void updateEInkDisplay(String message) {
  display.clearMemory();
  display.landscape();
  display.setFont( &FreeMonoBoldOblique12pt7b );
  std::vector<String> wrappedLines = wordWrapToArray(message, 16);

  int numberOfLines = wrappedLines.size();
  Serial.print("number of lines: "); Serial.println(numberOfLines);
  int yOffset;
  if (numberOfLines == 2) {
    yOffset = -18;
  } else if (numberOfLines == 3) {
    yOffset = -24;
  } else if (numberOfLines >= 4) {
    yOffset = -48;
  } else {
    yOffset = -48; // 70 characters max
  }

  int16_t x = 0;
  int16_t tbx, tby; uint16_t tbw, tbh;
  int lineHeight = 24;

  if (message.length() > 16){
    for (const String& line : wrappedLines) {
      display.getTextBounds(line, x, yOffset, &tbx, &tby, &tbw, &tbh);
      Serial.print("line: "); Serial.println(line);
      Serial.print("x: "); Serial.println(x);
      Serial.print("yOffset: "); Serial.println(yOffset);
      Serial.print("tbx: "); Serial.println(tbx);
      Serial.print("tby: "); Serial.println(tby);
      Serial.print("tbw: "); Serial.println(tbw);
      Serial.print("tbh: "); Serial.println(tbh);
      display.printCenter(line, 0, yOffset);

      //yOffset += tbh + (lineHeight - tbh);
      yOffset += lineHeight;
    }
  }
  else {
    display.printCenter(message);
  }
  display.update();
  delay(2000);
}

void processPacket(byte* packetData, int packetLength) {
  if (packetLength < 1) {
    Serial.println("Packet too short");
    return;
  }
  uint8_t packetType = packetData[0];
  switch (packetType) {
    case PACKET_TYPE_STATUS:
      if (packetLength >= sizeof(StatusMessageData) + 1) {
        StatusMessageData data;
        memcpy(&data, packetData + 1, sizeof(StatusMessageData));
        Serial.print("status message: e ink id ="); 
        Serial.println(data.eInkId, HEX);
        if (data.eInkId == localAddress) {
          Serial.print("message="); 
          Serial.println(data.message);
          Serial.print("Packet length:"); 
          Serial.println(packetLength);
          updateEInkDisplay(String(data.message));
        } else {
          Serial.println("message not for me, going to sleep in 60 seconds");
        }
      } else Serial.println("status message size mismatch");
      break;
    default:
      Serial.print("Unknown type: "); Serial.println(packetType);
  }
}

void loop() {
  if (receivedFlag) {
    lastPacketReceivedTime = millis();
    Serial.println("RADIO RECEIVED PACKET (setFlag)");
    int packetLength = radio.getPacketLength();
    Serial.printf("length: %d\n", packetLength);

    byte packetData[packetLength];
    radio.readData(packetData, packetLength);
    processPacket(packetData, packetLength);
    receivedFlag = false;
    radio.startReceive();
  } 
  unsigned long currentTime = millis();
  if (currentTime - lastPacketReceivedTime >= PACKET_RECEIVE_TIMEOUT) {
    Serial.println("it has been 60 seconds since we last received a packet, going to sleep!");
    esp_deep_sleep_start();
    Serial.println("this should never be read bc should be sleeping");
  }
}