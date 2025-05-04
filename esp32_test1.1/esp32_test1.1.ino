#include <SPI.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "ESP_AVRISP.h"
#include "Wire.h"
#include "SC8721.h"

#include "moving_Average.hpp"
#include "Arduino_fader.hpp"

const char* host = "esp-avrisp-test";
const char* ssid = "SSID";//自分の環境に合わせて書き換え
const char* pass = "PASS";//自分の環境に合わせて書き換え
const uint16_t port = 328;

#define reset_pin 4
#define spi_cs1 10
#define button 0
#define led_G 19
#define led_R 18

// SPI clock frequency in Hz
#define AVRISP_SPI_FREQ 400e3

TaskHandle_t th[2];
SC8721 DDC;
ArduinoFader Fader;

ESP_AVRISP avrprog(port, reset_pin, &SPI);

boolean old_button;
volatile boolean FaderAutoControl = true;

void setup() {
  Serial.begin(115200);
  pinMode(button, INPUT_PULLUP);
  pinMode(spi_cs1, OUTPUT);
  pinMode(led_G, OUTPUT);
  pinMode(led_R, OUTPUT);
  digitalWrite(led_R, HIGH);
  digitalWrite(spi_cs1, HIGH);
  Serial.println("");
  Serial.println("Arduino AVR-ISP over TCP");

  Wire.begin(7, 6);
  DDC.status_all_read();
  DDC.cso_setting(1);   //電流制限1A
  DDC.vout_setting(7);  //電圧設定7V
  delay(500);

  SPI.begin(1, 3, 2);  //sck miso mosi
  SPI.setDataMode(SPI_MODE0);
  SPI.setFrequency(AVRISP_SPI_FREQ);
  SPI.setHwCs(false);

  delay(100);
  Fader.initialize(&SPI, spi_cs1);
  avrprog.setReset(false);  // let the AVR run

  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(500);
  WiFi.begin(ssid, pass);
  uint8_t cnt2 = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(10);
    cnt2++;
    if (cnt2 > 250) ESP.restart();
  }

  MDNS.begin(host);
  MDNS.addService("avrisp", "tcp", port);
  avrprog.begin();

  Serial.println(Fader.enabled());
  Serial.println("SYSTEM OK");
  digitalWrite(led_R, LOW);

  xTaskCreatePinnedToCore(sub_process, "sub_process", 4096, NULL, 2, &th[0], 0);
  xTaskCreatePinnedToCore(fader_control, "fader_control", 4096, NULL, 3, &th[1], 0);
}

void sub_process(void* pvParameters) {
  ArduinoOTA.setHostname(host);
  ArduinoOTA.begin();

  while (1) {
    ArduinoOTA.handle();

    delay(1);
  }
}

void fader_control(void* pvParameters) {
  const uint16_t preset[10] = { 0, 10, 100, 512, 200, 1023, 300, 800, 1000, 700 };

  while (1) {
    for (uint8_t i = 0; i < 10; i++) {
      if (FaderAutoControl) Fader.set_target(preset[i]);
      delay(1000);
    }
  }
}

void loop() {
  static AVRISPState_t last_state = AVRISP_STATE_IDLE;
  AVRISPState_t new_state = avrprog.update();
  if (last_state != new_state) {
    switch (new_state) {
      case AVRISP_STATE_IDLE:
        Serial.printf("[AVRISP] now idle\r\n");
        digitalWrite(led_G, HIGH);
        delay(1000);
        Serial.println(Fader.enabled());
        break;
      case AVRISP_STATE_PENDING:
        Serial.printf("[AVRISP] connection pending\r\n");
        digitalWrite(led_R, HIGH);
        break;
      case AVRISP_STATE_ACTIVE:
        Serial.printf("[AVRISP] programming mode\r\n");
        digitalWrite(led_R, HIGH);
        break;
    }
    last_state = new_state;
  }
  // Serve the client
  if (last_state != AVRISP_STATE_IDLE) {
    avrprog.serve();
  }

  if (new_state == AVRISP_STATE_IDLE) {
    if (Fader.get_touchStatus()) {
      Serial.println("fader_val : " + String(Fader.get_faderVal()));
    }

    digitalWrite(led_G, Fader.get_touchStatus());
    digitalWrite(led_R, !Fader.get_lockStatus());

    if (!digitalRead(button)) {
      old_button = true;
      FaderAutoControl = !FaderAutoControl;
      digitalWrite(led_R, HIGH);
      delay(1000);
    } else if (old_button) {
      old_button = false;
      delay(500);
    }

    Fader.refresh();
    //Serial.println("touch_status1 : " + String(Fader.get_touchStatus() ? "Yes" : "No-"));
    //Serial.println("lock_status1 : " + String(Fader.get_lockStatus() ? "Yes" : "No-"));

    delay(10);
  }
}