#ifndef WEBSERVER_H_CUSTOM
#define WEBSERVER_H_CUSTOM

#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "motor_analysis.h"

// External declarations
extern AsyncWebServer server;
extern AsyncWebSocket ws;

// Function declarations
void setupWebServer();
void handleWebSocketMessage(AsyncWebSocketClient *client, const char *message);
void broadcastJson(const String& json);

#endif 