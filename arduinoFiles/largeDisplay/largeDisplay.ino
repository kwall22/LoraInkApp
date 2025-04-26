// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Library: https://github.com/ZinggJM/GxEPD2
// Waveshare e-paper displays with SPI: https://forum.arduino.cc/t/waveshare-e-paper-displays-with-spi/467865

//Last used with XIAO_ESP32S3_PLUS board, for Heltec ESP32 WiFi LoRa (V3) see different pin configurations
#include <SPI.h>
#include <RadioLib.h>
#include <string.h>
#include <vector>
#include <esp_sleep.h>

SPIClass hspi(HSPI);

#include "hal/efuse_hal.h"
#include "esp_mac.h"

#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "GxEPD2_display_selection_new_style.h" //lines 24, 222 - 226 are ours
//#define GxEPD2_DRIVER_CLASS GxEPD2_730c_ACeP_730 //line 114 in GxEPD2_selection_check.h
//GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=5*/ EPD_CS, /*DC=*/ 33, /*RST=*/ 35, /*BUSY=*/ 36)); for Heltec in GxEPD2_display_selection_new_style.h 

// color definitions for GxEPD2, values correspond to RGB565 values for TFTs
// values for 3-color or 7-color EPDs
#define GxEPD_RED       0xF800 // 255,   0,   0
#define GxEPD_YELLOW    0xFFE0 // 255, 255,   0 
#define GxEPD_COLORED   GxEPD_RED
// values for 7-color EPDs only
#define GxEPD_BLUE      0x001F //   0,   0, 255
#define GxEPD_GREEN     0x07E0 //   0, 255,   0
#define GxEPD_ORANGE    0xFC00 // 255, 128,   0

// Heltec:
// #define CS 2
// #define MISO 33
// #define MOSI 34
// #define SCK 26
// Seeed:
#define CS 41
#define MISO 6
#define MOSI 5
#define SCK 42

// Heltec:
// #define LORA_PIN_NSS     8
// #define LORA_PIN_DIO1   14
// #define LORA_PIN_RST    12
// #define LORA_PIN_BUSY   13
// Seeed:
#define LORA_PIN_NSS    12
#define LORA_PIN_DIO1   1
#define LORA_PIN_RST    13
#define LORA_PIN_BUSY   2

SX1262 radio = new Module(LORA_PIN_NSS, LORA_PIN_DIO1, LORA_PIN_RST, LORA_PIN_BUSY);

volatile bool receivedFlag = false;
uint8_t NUM_EVENTS = 0;
uint16_t localAddress = 0;
bool wakeUpReceived = false;
uint8_t eventCount = 0;
uint8_t receivedCount = 0;
bool headerFooterReceived = false;
char globalHeader[100] = "";
char globalFooter[100] = ""; 

unsigned long lastEventReceivedTime = 0;
const unsigned long EVENT_RECEIVE_TIMEOUT = 60000;

unsigned long lastPacketReceivedTime = 0;
const unsigned long PACKET_RECEIVE_TIMEOUT = 120000;

struct EventBox{
  int x0;
  int y0;
  int w;
  int h;
};

#define PACKET_TYPE_WAKEUP 0xA1
#define PACKET_TYPE_HEADER_FOOTER 0xB2
#define PACKET_TYPE_EVENT 0xC3
#define PACKET_TYPE_STATUS 0xD4
#define PACKET_TYPE_SYNC 0xE5

struct EventInfo{
  int dayOfWeek;
  int startTimeMinutes;
  int endTimeMinutes;
  uint8_t backgroundColor;
  char description[32];
};
std::vector<EventInfo> eventsList;

#pragma pack(1)
struct HeaderFooterData {
  uint16_t eInkId;
  uint8_t headerLength;
  char header[100];
  uint8_t footerLength;
  char footer[100];
};
#pragma pack()

#pragma pack(1)
struct SyncCountData {
  uint16_t eInkId;
  uint8_t eventCount;
};
#pragma pack()

enum class EventColor : uint8_t {
  RED = 0,
  YELLOW = 1,
  BLUE = 2,
  GREEN = 3,
  ORANGE = 4
};

void setFlag(void) {
  receivedFlag = true;
}

int timeStringToMinutes(const char* timeStr) {
  int hour, minute;
  sscanf(timeStr, "%d:%d", &hour, &minute);
  return hour * 60 + minute;
}
void minutesToTimeString(int minutes, char* timeString) {
  int hours = minutes / 60;
  int mins = minutes % 60;
  sprintf(timeString, "%d:%02d", (hours > 12) ? hours - 12 : hours, mins);
}

struct EventData {
  uint16_t eInkId;
  uint8_t dayOfWeek;
  uint16_t startTimeMinutes;
  uint16_t endTimeMinutes;
  uint8_t backgroundColor;
  char description[32];
};

void minToTimeString(uint16_t minutes, char* timeString) {
  int hours = minutes / 60;
  int mins = minutes % 60;
  sprintf(timeString, "%02d:%02d", hours, mins);
}

void setup()
{
  delay(200);
  Serial.begin(115200);
  delay(500);
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  localAddress = (mac[3] << 16) | (mac[4] << 8) | mac[5];
  Serial.printf("Local Lora address: %X %d\n", localAddress, localAddress);


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
  //e ink stuff 
  hspi.begin(SCK, MISO, MOSI, CS);
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  display.init(115200, true, 2, false);

  pinMode(LORA_PIN_DIO1, INPUT);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)LORA_PIN_DIO1, 1);
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
  if ((currentTime - lastEventReceivedTime >= EVENT_RECEIVE_TIMEOUT) && (eventsList.size() < eventCount) && (eventsList.size() > 0) && (eventCount > 0)) {
    Serial.println("it has been 60 seconds since we last received an event and we still don't have all of them!");
    Serial.print("Event Count = "); Serial.println(eventCount);
    Serial.print("Events List Size = "); Serial.println(eventsList.size());
    eventsList.clear();
    eventCount = 0;
    headerFooterReceived = false;
  }

  if (currentTime - lastPacketReceivedTime  >= PACKET_RECEIVE_TIMEOUT){
    Serial.println("it has been 2 minutes since we last received a packet!");
    Serial.println("going to sleep!");
    eventsList.clear();
    eventCount = 0;
    headerFooterReceived = false;
    display.hibernate();
    esp_deep_sleep_start();
    Serial.println("this should never be read bc should be sleeping");
  }

}

struct EventBox getEventBox (int dayOfWeek, int startTimeMinutes, int endTimeMinutes) {
  int earliestTimeMinutes = getEarliestTimeMinutes();
  int dayWidth = (750) / 5;
  int daysStartX = 52;
  int daysEndY = 57;
  int timeHeight = 44;
  int x = (daysStartX + (dayOfWeek * dayWidth)) + 2;
  int y = (((startTimeMinutes - earliestTimeMinutes) * timeHeight) / 60) + daysEndY;
  int height = ((endTimeMinutes - startTimeMinutes) * timeHeight) / 60;

  struct EventBox newEventBox;
  newEventBox.x0 = x;
  newEventBox.y0 = y;
  newEventBox.w = (dayWidth - 7);
  newEventBox.h = height;
  return newEventBox;
}

void drawBaseCalendar(int numOfEvents) {
  display.setRotation(2);
  int cols = 5;
  int dayWidth = (750) / cols;

  display.setFullWindow();
  display.firstPage();
  const char* daysOfWeek[5] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
  const char* subHeader = globalHeader;
  const char* footer = globalFooter;
  do {
    display.fillScreen(GxEPD_WHITE);
    //header
    display.drawRect(0, 0, 800, 30, GxEPD_BLACK);
    display.fillRect(0, 0, 800, 30, GxEPD_BLUE);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_WHITE);
    int16_t tbX, tbY; uint16_t tbW, tbH;
    display.getTextBounds(subHeader, 0, 0, &tbX, &tbY, &tbW, &tbH);
    int cursor_x = 0 + (800 - tbW) / 2 - tbX;
    int cursor_y = 0 + (30 - tbH) / 2 - tbY;
    display.setCursor(cursor_x, cursor_y);
    display.print(subHeader);
    //footer
    display.drawRect(0, 453, 800, 27, GxEPD_BLACK);
    display.fillRect(0, 453, 800, 27, GxEPD_BLUE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    int16_t tb_X, tb_Y; uint16_t tb_W, tb_H;
    display.getTextBounds(footer, 0, 0, &tb_X, &tb_Y, &tb_W, &tb_H);
    int cursor_x_ = 0 + (800 - tb_W) / 2 - tb_X;
    int cursor_y_ = 453 + (27 - tb_H) / 2 - tb_Y;
    display.setCursor(cursor_x_, cursor_y_);
    display.print(footer);

    int x0 = 0;
    int y0 = 30;
    int timeColWidth = 50;
    int dayHeight = 27;

    display.drawRect(x0, y0, 800, 423, GxEPD_BLACK);

    for (int i = 0; i < cols; i++) {
      int x = x0 + timeColWidth + (i * dayWidth);
      int y = y0;
      display.drawRect(x, y, dayWidth, dayHeight, GxEPD_BLACK);
      display.fillRect(x, y, dayWidth, dayHeight, GxEPD_RED);
      display.drawLine(x, y, x, (y + 420), GxEPD_BLACK);
      display.setFont(&FreeMonoBold9pt7b);
      display.setTextColor(GxEPD_WHITE);
      int16_t tbx, tby; uint16_t tbw, tbh;
      display.getTextBounds(daysOfWeek[i], 0, 0, &tbx, &tby, &tbw, &tbh);
      int cursorX = x + (dayWidth - tbw) / 2 - tbx;
      int cursorY = y + (dayHeight - tbh) / 2 - tby;
      display.setCursor(cursorX, cursorY);
      display.print(daysOfWeek[i]);
    }

    drawTimeLabels(x0, y0, timeColWidth);
    if (numOfEvents > 0) {
      for (int i = 0; i < numOfEvents; i ++){
        struct EventInfo eventInfo = eventsList[i];
        EventBox eventBox = getEventBox(eventInfo.dayOfWeek, eventInfo.startTimeMinutes, eventInfo.endTimeMinutes);
        addEvent(eventBox, eventInfo.description, eventInfo.startTimeMinutes, eventInfo.endTimeMinutes, eventInfo.backgroundColor);
      }
    }
  } while (display.nextPage());
}

int getEarliestTimeMinutes() {
  if (eventsList.empty()) {
    Serial.println("no events");
    return 360;
  }
  int earliestTime = 1440;

  for (const auto& eventInfo : eventsList) {
    if (eventInfo.startTimeMinutes < earliestTime) {
      earliestTime = eventInfo.startTimeMinutes;
    }
  }
  int startHourInMinutes = (earliestTime / 60) * 60;
  return startHourInMinutes;
}

void drawTimeLabels(int x0, int y0, int timeColWidth) {
  int earliestTimeMinutes = getEarliestTimeMinutes();

  int numSlots = 9;
  char timeLabels[numSlots][6]; 

  for (int i = 0; i < numSlots; i++) {
    int timeMinutes = earliestTimeMinutes + (i * 60);
    minutesToTimeString(timeMinutes, timeLabels[i]);
  }
  int timeSlotHeight = 44;
  for (int i = 0; i < numSlots; i++) {
    int y = y0 + 27 + (i * timeSlotHeight);
    display.drawRect(x0, y, timeColWidth, timeSlotHeight, GxEPD_BLACK);
    display.setFont();
    display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(timeLabels[i], 0, 0, &tbx, &tby, &tbw, &tbh);
    int cursorX = x0 + (timeColWidth - tbw) / 2 - tbx;
    int cursorY = y + (timeSlotHeight - tbh) / 2 - tby;
    display.setCursor(cursorX, cursorY);
    display.print(timeLabels[i]);
  }
}

const char* formatEventTime(int startTimeMinutes, int endTimeMinutes) {
  static char formattedTime[20];
  char startTimeStr[6];
  char endTimeStr[6];

  minutesToTimeString(startTimeMinutes, startTimeStr);
  minutesToTimeString(endTimeMinutes, endTimeStr);

  sprintf(formattedTime, "%s-%s", startTimeStr, endTimeStr);
  return formattedTime;
}

uint16_t getTextColor(uint8_t backgroundColor) {
  switch (backgroundColor) {
    case static_cast<uint8_t>(EventColor::GREEN):
      return GxEPD_WHITE;
    case static_cast<uint8_t>(EventColor::BLUE):
      return GxEPD_WHITE;
    case static_cast<uint8_t>(EventColor::RED):
      return GxEPD_WHITE;
    case static_cast<uint8_t>(EventColor::YELLOW):
      return GxEPD_BLACK;
    case static_cast<uint8_t>(EventColor::ORANGE):
      return GxEPD_BLACK;
    default:
      return GxEPD_WHITE;
  }
}

uint16_t getColorFromEnum(EventColor color) {
  switch (color) {
    case EventColor::RED:    return GxEPD_RED;
    case EventColor::YELLOW: return GxEPD_YELLOW;
    case EventColor::BLUE:   return GxEPD_BLUE;
    case EventColor::GREEN:  return GxEPD_GREEN;
    case EventColor::ORANGE: return GxEPD_ORANGE;
    default:                 return GxEPD_GREEN;
  }
}

void addEvent(EventBox eventBoxToAdd, char* eventName, int startMinutes, int endMinutes, uint8_t backgroundColor) {
  const char* timeString = formatEventTime(startMinutes, endMinutes);
  uint16_t textColor = getTextColor(backgroundColor);

  int rectX0 = eventBoxToAdd.x0;
  int rectY0 = eventBoxToAdd.y0;
  int rectW = eventBoxToAdd.w;
  int rectH = eventBoxToAdd.h;

  int16_t tbx, tby; uint16_t tbw, tbh;

  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(textColor);
  display.getTextBounds(eventName, 0, 0, &tbx, &tby, &tbw, &tbh);

  display.drawRect(rectX0, rectY0, rectW, rectH, GxEPD_BLACK);
  display.fillRect(rectX0, rectY0, rectW, rectH, getColorFromEnum(static_cast<EventColor>(backgroundColor)));

  int cursorX = rectX0 + (rectW - tbw) / 2 - tbx;
  int cursorY = rectY0 + (rectH - tbh) / 2 - tby;
  display.setCursor(cursorX, cursorY - 10);
  display.print(eventName);
  int16_t Timetbx, Timetby; uint16_t Timetbw, Timetbh;
  display.getTextBounds(timeString, 0, 0, &Timetbx, &Timetby, &Timetbw, &Timetbh);
  int timeCursorX = rectX0 + (rectW - Timetbw) / 2 - Timetbx;
  display.setCursor(timeCursorX, cursorY + 8);
  display.print(timeString);
}

void drawCalendar(int numOfEvents) {
  drawBaseCalendar(numOfEvents);
}

void processPacket(byte* packetData, int packetLength) {
  if (packetLength < 1) {
    Serial.println("Packet too short");
    return;
  }
  uint8_t packetType = packetData[0];
  Serial.print("received packet of type: ");
  Serial.println(packetType);

  switch (packetType) { 
    case PACKET_TYPE_SYNC:
      if (packetLength >= sizeof(SyncCountData) + 1) {
        eventsList.clear();
        eventCount = 0;
        headerFooterReceived = false;

        SyncCountData data;
        memcpy(&data, packetData + 1, sizeof(SyncCountData));
        Serial.print("Type Sync: E-Ink ID ="); 
        Serial.println(data.eInkId, HEX); 
        if (data.eInkId != localAddress) {
          Serial.println("Sync message not for me, should go to sleep");
          return;
        }
        Serial.print("Event Count ="); Serial.println(data.eventCount);
        if(data.eventCount > 0){
          eventCount = data.eventCount;
        }
        else {
          eventCount = 0;
        }
      } else Serial.println("Sync size mismatch");
      break;
    case PACKET_TYPE_HEADER_FOOTER:
      if (packetLength >= sizeof(HeaderFooterData) + 1) {
        HeaderFooterData data;
        memcpy(&data, packetData + 1, sizeof(HeaderFooterData));
        Serial.print("HeaderFooter: eInkId= "); 
        Serial.println(data.eInkId, HEX); 
        if (data.eInkId != localAddress) {
          Serial.println("header and footer not for me, should go to sleep");
          return;
        }
        Serial.print("Header Length = "); Serial.println(data.headerLength); 
        Serial.print("Header ="); Serial.println(data.header);
        Serial.print("Footer Length = "); Serial.println(data.footerLength); 
        Serial.print("Footer = "); Serial.println(data.footer);

        data.header[data.headerLength] = '\0';
        data.footer[data.footerLength] = '\0';

        strncpy(globalHeader, data.header, sizeof(globalHeader) - 1);
        globalHeader[sizeof(globalHeader) - 1] = '\0';
        strncpy(globalFooter, data.footer, sizeof(globalFooter) - 1);
        globalFooter[sizeof(globalFooter) - 1] = '\0';
        headerFooterReceived = true;

        if(eventsList.size() == eventCount && eventCount == 0 && headerFooterReceived){
          drawCalendar(0);
          eventsList.clear();
          eventCount = 0;
          headerFooterReceived = false;
        }
      } else Serial.println("Header Footer size mismatch");
      break;
    case PACKET_TYPE_EVENT:
      if (packetLength >= sizeof(EventData) + 1) {
        EventData data;
        memcpy(&data, packetData + 1, sizeof(EventData));
        Serial.print("Event: eInkId="); 
        Serial.println(data.eInkId, HEX);
        if (data.eInkId != localAddress) {
          Serial.println("message not for me, should go to sleep");
          return;
        }

        lastEventReceivedTime = millis();

        Serial.print("Day of Week ="); Serial.println(data.dayOfWeek); 
        Serial.print("Start Time Minutes ="); Serial.println(data.startTimeMinutes);
        Serial.print("End Time Minutes ="); Serial.println(data.endTimeMinutes); 
        Serial.print("Background Color ="); Serial.println(data.backgroundColor);
        Serial.print("Description ="); Serial.println(data.description);
        EventInfo eventInfo;
        eventInfo.dayOfWeek = (data.dayOfWeek - 1);
        eventInfo.startTimeMinutes = data.startTimeMinutes;
        eventInfo.endTimeMinutes = data.endTimeMinutes;
        strncpy(eventInfo.description, data.description, sizeof(eventInfo.description) - 1);
        eventInfo.description[sizeof(eventInfo.description) - 1] = '\0';
        eventInfo.backgroundColor = data.backgroundColor;

        eventsList.push_back(eventInfo);

        Serial.print("Received event: ");
        Serial.println(eventInfo.description);
        Serial.print("List size: ");
        Serial.println(eventsList.size());
        if(eventsList.size() == eventCount && eventCount > 0 && headerFooterReceived) {
          Serial.println("in if statement for draw calendar");
          drawCalendar(eventsList.size());
          eventsList.clear();
          eventCount = 0;
          headerFooterReceived = false;
        }
      } else Serial.println("Event size mismatch");
      break;
    default:
      Serial.print("Unknown type: "); Serial.println(packetType);
  }
}