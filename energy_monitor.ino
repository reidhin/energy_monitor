// https://github.com/mruettgers/SMLReader
// https://www.bitsandparts.nl/Lichtsensor-Fototransistor-BPW40-p115130
// BPW40
// flat side = collector
// pijl gaat naar emitter

// https://subscription.packtpub.com/book/iot-and-hardware/9781785888564/3/ch03lvl1sec27/reading-and-counting-pulses-with-arduino
// https://roboticsbackend.com/arduino-pulsein-with-interrupts/
// https://forum.arduino.cc/t/counting-pulse-of-energy-meter/550830/8

// follow this first: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

/**
 * ----------------------------------------------------------------------------
 * Energy Monitor
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 */

// include secrets to connect to wifi 
#include "arduino_secrets.h"

// File system
#include <LittleFS.h>

// WiFi and webserver stuff
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

// Port for webserver
#define HTTP_PORT 80

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Nothing yet...

// ----------------------------------------------------------------------------
// Definition of the Led component
// ----------------------------------------------------------------------------

struct Led {
  // state variables
  uint8_t pin;
  bool    on;

  // methods
  void update() {
    digitalWrite(pin, on ? HIGH : LOW);
  }
};

// ----------------------------------------------------------------------------
// Definition of the analog sensor
// ----------------------------------------------------------------------------

struct AnalogSensor {
  // state variables
  uint8_t pin;

  // methods
  int read() {
    return analogRead(pin);
  }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led onboard_led = { LED_BUILTIN, false };  // LED_BUILTIN is a variable arduino

AnalogSensor photo_resistor = { A0 };  // A0 is an arduino variable - ESP8266 Analog Pin ADC0 = A0

AsyncWebServer server(HTTP_PORT);

int sensorValue = 0;

// ----------------------------------------------------------------------------
// LittleFS initialization
// ----------------------------------------------------------------------------

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("Cannot mount LittleFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
  
  //The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
  return String();
}

void onRootRequest(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html", false, processor);
}

void onSensorRequest(AsyncWebServerRequest *request) {
  request->send(200, "text/html", String(photo_resistor.read()));
}

void initWebServer() {
  server.on("/", onRootRequest);
  server.serveStatic("/", LittleFS, "/");
  server.on("/energy", onSensorRequest);
  server.begin();
  Serial.println("HTTP Server Started");
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  pinMode(onboard_led.pin, OUTPUT);

  Serial.begin(115200); delay(500);

  initLittleFS();
  initWiFi();
  initWebServer();
}

void loop() {
  // read the analog in value
  sensorValue = photo_resistor.read();
 
  // print the readings in the Serial Monitor
  Serial.print("sensor = ");
  Serial.println(sensorValue);
  
  delay(1000);
}

