#define BLYNK_DEVICE_NAME           "ESP-12F"
#define BLYNK_AUTH_TOKEN            "YOUR_TOKEN_HERE"
#define NUM_LEDS 20 //20
#define LED_PIN 14
#define USB_PIN 12

#define NO_EVENT 0
#define EVENT_ON 1
#define EVENT_OFF 2
#define CHANGE_BRIGHTNESS 3
#define CHANGE_COLOR_DOWN 4
#define CHANGE_COLOR_UP 5

#define RGB_STATE V0
#define RGB_COLOR V1
#define RGB_BRIGHTNESS V4
#define RGB_AMBILIGHT_STATE V7

#define USB_STATE V3
#define USB_BRIGHTNESS V2

#define TEMPERATURE V5
#define HUMIDITY V6

#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_AHTX0.h>
#include <TimerMs.h>
#include <ArduinoJson.h>

TimerMs tmrUsb(1);
TimerMs tmrRgb(1);
Adafruit_AHTX0 aht;
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "SSID";
char pass[] = "PASSWORD";
CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);

int usbBright = 0;
byte rgbBright = 0;
bool usbState, rgbState = 0;
byte rgbColor[3] = {0, 0, 0};
byte TmpRgbColor[3] = {0, 0, 0};
sensors_event_t humidity, temp;
int tempValUsb = 0;
byte tempValRgb = 0;
bool tmpVectorUsb, tmpVectorRgb = 0;
byte UsbEvent, RgbEvent = NO_EVENT;
int SwitchPowerValUsb, SwitchPowerValRgb = 0;
bool ambilightState = 0;
DynamicJsonDocument InputData(4096);

BLYNK_CONNECTED() {
  Blynk.syncVirtual(USB_STATE, RGB_STATE);
  Blynk.syncVirtual(USB_BRIGHTNESS, RGB_BRIGHTNESS);
  Blynk.syncVirtual(RGB_COLOR);
}
// запрос температуры
BLYNK_READ(TEMPERATURE)
{
  aht.getEvent(&humidity, &temp);
  Blynk.virtualWrite(TEMPERATURE, temp.temperature);
}
// запрос влажности
BLYNK_READ(HUMIDITY)
{
  aht.getEvent(&humidity, &temp);
  Blynk.virtualWrite(HUMIDITY, humidity.relative_humidity);
}

// Установка яркости ленты
BLYNK_WRITE(RGB_BRIGHTNESS)
{
  //Serial.println("SET BRIGHT RGB");
  if (rgbState){
    tempValRgb = map(param.asInt(), 1, 100, 1, 255);
    RgbEvent = CHANGE_BRIGHTNESS;
    tmpVectorRgb = tempValRgb > rgbBright;
    tmrRgb.start();
  } else rgbBright = map(param.asInt(), 1, 100, 1, 255);
}
// чтение яркости ленты
BLYNK_READ(RGB_BRIGHTNESS)
{
  //Serial.println("READ BRIGHT RGB");
  Blynk.virtualWrite(RGB_BRIGHTNESS, map(rgbBright, 1, 255, 1, 100));
}
// включение/выключение ленты
BLYNK_WRITE(RGB_STATE)
{
  //Serial.println("SWITCH STATE RGB");
  if (param.asInt() == rgbState) return;
  RgbEvent = param.asInt() ? EVENT_ON : EVENT_OFF;
  tempValRgb = param.asInt() ? rgbBright : 0;
  tmpVectorRgb = tempValRgb > rgbBright;
  SwitchPowerValRgb = param.asInt() ? 0 : rgbBright;
  tmrRgb.start();
}
// чтение состояния ленты
BLYNK_READ(RGB_STATE)
{
  //Serial.println("READ STATE RGB");
  Blynk.virtualWrite(RGB_STATE, rgbState);
}
// установка цвета ленты
BLYNK_WRITE(RGB_COLOR)
{
  //Serial.println("SET COLOR RGB");
  if(rgbState){
    RgbEvent = CHANGE_COLOR_DOWN;
    SwitchPowerValRgb = rgbBright;
    TmpRgbColor[0] = param[0].asInt();
    TmpRgbColor[1] = param[1].asInt();
    TmpRgbColor[2] = param[2].asInt();
    tmrRgb.start();
  }else{
    rgbColor[0] = param[0].asInt();
    rgbColor[1] = param[1].asInt();
    rgbColor[2] = param[2].asInt();
  }
}
// чтение цвета
BLYNK_READ(RGB_COLOR)
{
  //Serial.println("READ STATE RGB");
  Blynk.virtualWrite(RGB_COLOR, rgbColor[0], rgbColor[1], rgbColor[2]);
}
// чтение состояния юсби
BLYNK_READ(USB_STATE)
{
  //Serial.println("READ STATE USB");
  Blynk.virtualWrite(USB_STATE, usbState);
}
// включение/выключение юсби
BLYNK_WRITE(USB_STATE)
{
  //Serial.println("SWITCH USB");
  if (param.asInt() == usbState) return;
  UsbEvent = param.asInt() ? EVENT_ON : EVENT_OFF;
  tempValUsb = param.asInt() ? usbBright : 0;
  tmpVectorUsb = tempValUsb > usbBright;
  SwitchPowerValUsb = param.asInt() ? 0 : usbBright;
  tmrUsb.start();
}
// установка яркости юсби
BLYNK_WRITE(USB_BRIGHTNESS)
{
  //Serial.println("SET BRIGHT USB");
  if(usbState){
    tempValUsb = map(param.asInt(), 1, 100, 1, 1023);
    UsbEvent = CHANGE_BRIGHTNESS;
    tmpVectorUsb = tempValUsb > usbBright;
    tmrUsb.start();
  }else usbBright = map(param.asInt(), 1, 100, 1, 1023);
}
// запрос яркости юсби
BLYNK_READ(USB_BRIGHTNESS)
{
  //Serial.println("READ BRIGHT USB");
  Blynk.virtualWrite(USB_BRIGHTNESS, map(usbBright, 1, 1023, 1, 100));
}

void setup()
{
  Serial.begin(115200);
  WiFi.hostname("Eugene Ambilight 1.1");
  tmrUsb.setPeriodMode();
  tmrRgb.setPeriodMode();
  tmrUsb.stop();
  tmrRgb.stop();
  aht.begin();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  Blynk.begin(auth, ssid, pass);
  WiFi.setAutoReconnect(true);
  server.begin();
  server.on("/set-rgb", SetRGB);
  server.on("/ambilight-state", ChangeAmbilightState);
  server.on("/ambilight", GoAmbilight);
  server.onNotFound([] () {
    server.send(404, "text/plain", "Not found");
  });
  Serial.println(WiFi.localIP());
}

void loop()
{
  Blynk.run();
  server.handleClient();
  tmrUsb.tick();
  tmrRgb.tick();
  if (tmrUsb.ready()) {
    switch (UsbEvent)
    {
      case EVENT_ON: {
          if (SwitchPowerValUsb != usbBright) {
            SwitchPowerValUsb++;
            analogWrite(USB_PIN, SwitchPowerValUsb);
          } else {
            UsbEvent = NO_EVENT;
            usbState = true;
          }
        } break;
      case EVENT_OFF: {
          if (SwitchPowerValUsb > 0) {
            SwitchPowerValUsb--;
            analogWrite(USB_PIN, SwitchPowerValUsb);
          } else {
            UsbEvent = NO_EVENT;
            usbState = false;
          }
        } break;
      case CHANGE_BRIGHTNESS: {
          if (usbBright != tempValUsb) {
            usbBright += tmpVectorUsb ? 1 : -1;
            analogWrite(USB_PIN, usbBright);
          } else {
            UsbEvent = NO_EVENT;
            usbState = usbBright > 0;
          }
        } break;
      case NO_EVENT: {
          //Serial.println("NO_EVENT USB");
          tmrUsb.stop();
          Blynk.virtualWrite(USB_STATE, usbState);
        } break;
    }
  }
  if (tmrRgb.ready()) {
    switch (RgbEvent) {
      case EVENT_ON: case CHANGE_COLOR_UP: {
          if (SwitchPowerValRgb != rgbBright) {
            SwitchPowerValRgb++;
            LEDS.setBrightness(SwitchPowerValRgb);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            RgbEvent = NO_EVENT;
            rgbState = true;
          }
          //Serial.println(RgbEvent == CHANGE_COLOR_UP ? "CHANGE_COLOR_UP " : "EVENT_ON " + String(SwitchPowerValRgb));
        } break;
      case EVENT_OFF: case CHANGE_COLOR_DOWN: {
          if (SwitchPowerValRgb > 0) {
            SwitchPowerValRgb--;
            LEDS.setBrightness(SwitchPowerValRgb);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            if (RgbEvent == CHANGE_COLOR_DOWN) {
              RgbEvent = CHANGE_COLOR_UP;
              rgbColor[0] = TmpRgbColor[0];
              rgbColor[1] = TmpRgbColor[1];
              rgbColor[2] = TmpRgbColor[2];
            } else {
              RgbEvent = NO_EVENT;
              rgbState = false;
            }
          }
          //Serial.println(RgbEvent == CHANGE_COLOR_DOWN ? "CHANGE_COLOR_DOWN " : "EVENT_OFF " + String(SwitchPowerValRgb));
        } break;
      case CHANGE_BRIGHTNESS: {
          if (rgbBright != tempValRgb) {
            rgbBright += tmpVectorRgb ? 1 : -1;
            LEDS.setBrightness(rgbBright);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            RgbEvent = NO_EVENT;
            rgbState = rgbBright > 0;
          }
          //Serial.println("CHANGE_BRIGHTNESS " + String(rgbBright));
        } break;
      case NO_EVENT: {
          //Serial.println("NO_EVENT RGB");
          tmrRgb.stop();
          Blynk.virtualWrite(RGB_STATE, rgbState);
        } break;
    }
  }
}

void SetRGB(){
  Serial.print(server.arg("r"));Serial.print(server.arg("g"));Serial.println(server.arg("b"));
  if(server.arg("r") && server.arg("g") && server.arg("b")){
    if(rgbState){
      RgbEvent = CHANGE_COLOR_DOWN;
      SwitchPowerValRgb = rgbBright;
      TmpRgbColor[0] = server.arg("r").toInt();
      TmpRgbColor[1] = server.arg("g").toInt();
      TmpRgbColor[2] = server.arg("b").toInt();
      tmrRgb.start();
    }else{
      rgbColor[0] = server.arg("r").toInt();
      rgbColor[1] = server.arg("g").toInt();
      rgbColor[2] = server.arg("b").toInt();
    }
    server.send(200, "text/plain", "ok");
  }else server.send(400, "text/plain", "Bad request");
  /*if (server.method() == HTTP_POST) {
    Serial.
  }else server.send(405, "text/plain", "Method Not Allowed");*/
}

void GoAmbilight(){
  Serial.println("GoAmbilight");
  if (server.method() == HTTP_POST) {
    if(server.arg("plain")){
      deserializeJson(InputData, server.arg("plain"));
      if(InputData.containsKey("Items") && InputData.containsKey("ItemsCount")){
        for (int i = 0; i < sizeof(InputData["Items"]) / 2 - 1; i++){
          Serial.print(InputData["Items"][i]["r"].as<int>()); Serial.print(" ");
          Serial.print(InputData["Items"][i]["g"].as<int>()); Serial.print(" ");
          Serial.print(InputData["Items"][i]["b"].as<int>()); Serial.print(" ");
          Serial.println(InputData["Items"][i]["i"].as<int>());
          leds[InputData["Items"][i]["i"].as<int>()] = CRGB(InputData["Items"][i]["r"].as<int>(), InputData["Items"][i]["g"].as<int>(), InputData["Items"][i]["b"].as<int>());
          FastLED.show();
        }
        
        server.send(200, "text/plain", "ok");
      }else server.send(400, "text/plain", "Bad request");
    }else server.send(400, "text/plain", "Bad request");
  }else server.send(405, "text/plain", "Method Not Allowed");
}

void ChangeAmbilightState(){
  if (server.method() == HTTP_POST) {
    if(server.arg("state")){
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, server.arg("plain"));
      if(doc.containsKey("state")){
        ambilightState = doc["state"] == true;
        server.send(200, "text/plain", "OK");
      }else server.send(400, "text/plain", "Bad request");
    }else server.send(400, "text/plain", "Bad request");
  }else server.send(405, "text/plain", "Method Not Allowed");
}