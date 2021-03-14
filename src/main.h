#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>
#ifdef DEBUGWIFI
#include <ESPmDNS.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <Wifi.h>
#endif

#include <cstring>
#include "Lpf2Hub.h"
#include "secret.h"

#ifdef DEBUGWIFI
#define HOST_NAME "duplotrain"
#define USE_MDNS true
#define MAX_SRV_CLIENTS 1

IPAddress local_IP(192, 168, 100, 123);
// Set your Gateway IP address
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);    // optional
IPAddress secondaryDNS(8, 8, 4, 4);  // optional
#endif

// Train control
const uint8_t TRAIN_FORWARD = 1u;    // U   works
const uint8_t TRAIN_BACKWARD = 2u;   // D   works
const uint8_t TRAIN_LIGHT = 3u;      // R1  works (left)
const uint8_t TRAIN_BRAKE = 4u;      // R2  works (no sound)
const uint8_t TRAIN_REFILL = 5u;     // B1  Not really??
const uint8_t TRAIN_HORN = 6u;       // B2  works
const uint8_t TRAIN_DEPARTURE = 7u;  // R3  Left button???? why
const uint8_t TRAIN_STEAM = 8u;      // B3  kinda, but left not right???
const uint8_t TRAIN_STOP = 9u;      // L
const uint8_t TRAIN_NOPE = 0u;

uint8_t TRAIN_CURRENT_STATE = 0u;
int8_t TRAIN_SPEEDF = 50;
int8_t TRAIN_SPEEDB = -50;
// create a hub instance
Lpf2Hub myHub;
uint8_t receivedData;
boolean newData = false;

byte motorPort = (byte)DuploTrainHubPort::MOTOR;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long soundTimer = 3000;  //the value is a number of milliseconds

#ifdef DEBUGWIFI

WiFiMulti wifiMulti;
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];
#endif
char recv_buffer[1] = {};

void recvData();
void handleRecvData();
void playSounds(byte sound);
#ifdef DEBUGWIFI
void waitForConnection();
void waitForDisconnection();
void telnetConnected();
void telnetDisconnected();
void handleTelnetClients();
void telPrint(uint8_t *dataArray, size_t length);
#endif

void speedometerSensorCallback(void *hub, byte portNumber,
                               DeviceType deviceType, uint8_t *pData);
void colorSensorCallback(void *hub, byte portNumber, DeviceType deviceType,
                         uint8_t *pData);


#endif  // MAIN_H