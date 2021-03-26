#include "main.h"

void setup() {
  startMillis = millis();
  Serial.begin(115200);
#ifdef SERIALDBG
  Serial.println("Debug: Booting train");
#endif
#ifdef DEBUGWIFI
  Serial.println("Connecting to WiFi ");
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  wifiMulti.addAP(ssid, password);
  waitForConnection();
  server.begin();
  server.setNoDelay(true);
  String hostNameWifi = HOST_NAME;
  hostNameWifi.concat(".local");
  MDNS.begin(HOST_NAME);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println("23' to connect");
#endif
  myHub.init();
}

// main loop
void loop() {
  // // connect flow
  if (myHub.isConnecting()) {
    myHub.connectHub();
    if (myHub.isConnected()) {
      #ifdef SERIALDBG
      Serial.println("train connected");
      #endif
      delay(200);
      // connect color sensor and activate it for updates
      myHub.activatePortDevice((byte)DuploTrainHubPort::SPEEDOMETER, speedometerSensorCallback);
      delay(200);
      // connect speed sensor and activate it for updates
      myHub.activatePortDevice((byte)DuploTrainHubPort::COLOR, colorSensorCallback);
      delay(200);
      myHub.setLedColor((Color)PURPLE);
    } else {
      #ifdef SERIALDBG
      Serial.println("train connect failed");
      #endif
    }
  }
  recvData();
  if (myHub.isConnected()) {
    handleRecvData();
  }
#ifdef DEBUGWIFI
  handleTelnetClients();
#endif
}

void recvData() {
  if (Serial.available()) {
    recv_buffer[0] = Serial.read();
      #ifdef SERIALDBG
    Serial.println(recv_buffer[0]);
    #endif
    uint8_t sbuf[3];
    sbuf[0] = (uint8_t)recv_buffer[0];
    sbuf[0] = sbuf[0] + 48;  // make ascii number
    sbuf[1] = (uint8_t)'\n';
    sbuf[2] = (uint8_t)'\r';
    receivedData = atoi(recv_buffer);

#ifdef DEBUGWIFI
    telPrint(sbuf, 3);
    //   if (Serial.available() > 0) {
    //     String incomingString = Serial.readStringUntil('\n');
    //     size_t len = incomingString.length();
    //     uint8_t sbuf[len];
    //     incomingString.toCharArray((char *)sbuf, len);
    //     // push UART data to all connected telnet clients
    //     telPrint(sbuf, len);

    //   incomingString.trim();
    //   receivedData = (uint8_t)incomingString.toInt();
    // }
#endif
  } else {
    receivedData = TRAIN_NOPE;
  }
}

void handleRecvData() {
  switch (receivedData) {
    case TRAIN_FORWARD: {
      if(TRAIN_FORWARD != TRAIN_CURRENT_STATE) {
        TRAIN_SPEEDF = 30;
      }
      if(TRAIN_BACKWARD == TRAIN_CURRENT_STATE) {
        myHub.stopBasicMotor(motorPort);
      }
      if(TRAIN_FORWARD == TRAIN_CURRENT_STATE) {
        TRAIN_SPEEDF += 10;
        if(TRAIN_SPEEDF > 100) { TRAIN_SPEEDF = 100;}
      }

      myHub.setBasicMotorSpeed(motorPort, TRAIN_SPEEDF);
      #ifdef SERIALDBG
      Serial.println("Fwd: ");
      #endif
      TRAIN_CURRENT_STATE = TRAIN_FORWARD;
      break;
    }
    case TRAIN_BACKWARD: {
      if(TRAIN_BACKWARD != TRAIN_CURRENT_STATE) {
        TRAIN_SPEEDB = -30;
      }
      if(TRAIN_FORWARD == TRAIN_CURRENT_STATE) {
        myHub.stopBasicMotor(motorPort);
      }
      if(TRAIN_BACKWARD == TRAIN_CURRENT_STATE) {
        TRAIN_SPEEDB -= 10;
        if(TRAIN_SPEEDB < -100) { TRAIN_SPEEDB = -100;}
      }
      myHub.setBasicMotorSpeed(motorPort, TRAIN_SPEEDB);
      #ifdef SERIALDBG
      Serial.println("Back: ");
      #endif
      TRAIN_CURRENT_STATE = TRAIN_BACKWARD;
      break;
    }
    case TRAIN_HORN: {
      playSounds((byte)DuploTrainBaseSound::HORN);
      #ifdef SERIALDBG
      Serial.println("Horn");
      #endif
      break;
    }
    case TRAIN_BRAKE: {
      myHub.stopBasicMotor(motorPort);
      delay(200);
      playSounds((byte)DuploTrainBaseSound::BRAKE);
      #ifdef SERIALDBG
      Serial.println("Brake");
      #endif
      TRAIN_CURRENT_STATE = TRAIN_BRAKE;
      break;
    }
    case TRAIN_REFILL: {
      myHub.stopBasicMotor(motorPort);
      delay(200);
      playSounds((byte)DuploTrainBaseSound::WATER_REFILL);
      #ifdef SERIALDBG
      Serial.println("Refill");
      #endif
      TRAIN_CURRENT_STATE = TRAIN_REFILL;
      break;
    }
    case TRAIN_LIGHT: {
      static uint8_t colorTrain = 0u;
      myHub.setLedColor((Color)colorTrain);
      #ifdef SERIALDBG
      Serial.print("Color: ");
      Serial.println(COLOR_STRING[(Color)colorTrain]);
      #endif
      colorTrain++;
      if (colorTrain > 10) {
        colorTrain = 0u;
      }
      break;
    }
    case TRAIN_STEAM: {
      playSounds((byte)DuploTrainBaseSound::STEAM);
      #ifdef SERIALDBG
      Serial.println("Steam");
      #endif
      break;
    }
    case TRAIN_DEPARTURE: {
      playSounds((byte)DuploTrainBaseSound::STATION_DEPARTURE);
      #ifdef SERIALDBG
      Serial.println("Departure");
      #endif
      break;
    }
    case TRAIN_NOPE: {
      break;
    }
    default:
      break;
  }
}

/**
 * Only play a sound if the counter elapsed, to avoid multi sounds
 **/
void playSounds(byte sound) {
  currentMillis = millis();
  if (currentMillis - startMillis >= soundTimer) {
    // TODO(ph) play a sound
    myHub.playSound(sound);
    startMillis = millis();
  }
}

void colorSensorCallback(void *hub, byte portNumber, DeviceType deviceType,
                         uint8_t *pData) {
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_COLOR_SENSOR) {
    int color = myHub->parseColor(pData);
    #ifdef SERIALDBG
    Serial.print("Color: ");
    Serial.println(COLOR_STRING[color]);
    #endif
    myHub->setLedColor((Color)color);

    if (color == (byte)RED) {
      myHub->playSound((byte)DuploTrainBaseSound::BRAKE);
    } else if (color == (byte)BLUE) {
      myHub->playSound((byte)DuploTrainBaseSound::WATER_REFILL);
    } else if (color == (byte)YELLOW) {
      myHub->playSound((byte)DuploTrainBaseSound::HORN);
    }
  }
}

void speedometerSensorCallback(void *hub, byte portNumber,
                               DeviceType deviceType, uint8_t *pData) {
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_SPEEDOMETER) {
    int speed = myHub->parseSpeedometer(pData);
    #ifdef SERIALDBG
    Serial.print("Fwd: ");
    Serial.println(TRAIN_SPEEDF);
    #endif
    if (speed > 10) {
      #ifdef SERIALDBG
      Serial.println("Forward");
      #endif
      myHub->setBasicMotorSpeed(motorPort, TRAIN_SPEEDF);
    } else if (speed < -10) {
      #ifdef SERIALDBG
      Serial.println("Back: ");
      Serial.println(TRAIN_SPEEDB);
      #endif
      myHub->setBasicMotorSpeed(motorPort, TRAIN_SPEEDB);
    } else {
      #ifdef SERIALDBG
      Serial.println("Stop");
      #endif
      myHub->stopBasicMotor(motorPort);
    }
  }
}

#ifdef DEBUGWIFI
void waitForConnection() {
  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    } else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }
}

void waitForDisconnection() {
  while (WiFi.status() == WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Disconnected!");
}

void telnetConnected() { Serial.println("Telnet connection established."); }

void telnetDisconnected() { Serial.println("Telnet connection closed."); }

void handleTelnetClients() {
  uint8_t i = 0;
  if (wifiMulti.run() == WL_CONNECTED) {
    // check if there are any new clients
    if (server.hasClient()) {
      for (i = 0; i < MAX_SRV_CLIENTS; i++) {
        // find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected()) {
          if (serverClients[i]) serverClients[i].stop();
          serverClients[i] = server.available();
          if (!serverClients[i]) Serial.println("available broken");
          Serial.print("New client: ");
          Serial.print(i);
          Serial.print(' ');
          Serial.println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        // no free/disconnected spot so reject
        server.available().stop();
      }
    }
    // check clients for data
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        if (serverClients[i].available()) {
          // get data from the telnet client and push it to the UART
          while (serverClients[i].available())
            Serial.write(serverClients[i].read());
        }
      } else {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
      }
    }
  } else {
    Serial.println("WiFi not connected!");
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) serverClients[i].stop();
    }
    delay(1000);
  }
}

// function to print to Telnet
void telPrint(uint8_t *dataArray, size_t length) {
  if (wifiMulti.run() == WL_CONNECTED) {
    for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        serverClients[i].write(dataArray, length);
        delay(1);
      }
    }
  }
}
#endif
