#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Button.h"

// debug
#define DEBUG true

#if DEBUG
#define debug(msg) Serial.print(msg)
#define debugHEX(msg) Serial.println(msg, HEX)
#define debugln(msg) Serial.println(msg)

#else
#define debug(msg)
#define debugHEX(msg)
#define debugln(msg)
#endif

#define BOTON D1
#define UP_FWD D2
#define DW_BKW D5
#define LEFT D6
#define RIGHT D7

#define LED D8


Button boton(BOTON, 750);
Button up_fwd(UP_FWD, 750);
Button dw_bkw(DW_BKW, 750);
Button left(LEFT, 750);
Button right(RIGHT, 750);

struct DroneCommands {
  const char* takeoff;
  const char* land;
  const char* move;
  const char* emergency;
  const char* flatTrim;
  const char* setMaxAltitude;
  const char* setMaxTilt;
  const char* setAlarmOk;
};

DroneCommands commands = {
  .takeoff = "AT*REF=%d,290718208\r",
  .land = "AT*REF=%d,290717696\r",
  .move = "AT*PCMD=%d,%d,%d,%d,%d,%d\r",
  .emergency = "AT*REF=%d,290717952\r",
  .flatTrim = "AT*FTRIM=%d\r",
  .setMaxAltitude = "AT*CONFIG=%d,\"control:altitude_max\",\"%d\"\r",
  .setMaxTilt = "AT*CONFIG=%d,\"control:euler_angle_max\",\"%f\"\r",
  .setAlarmOk = "AT*CONFIG=%d,\"general:navdata_demo\",\"TRUE\"\r"
};

WiFiUDP udp;
WiFiUDP udpTelemetry;

const char* droneIp = "192.168.1.1";  // Default IP of AR.Drone 2.0
const int localPort = 8888;
const int dronePort = 5556;
const int telemetry = 5554;
const char* ssid = "enarrdo";  // Replace with your drone's SSID
const char* password = "";     // Usually no password

int sequenceNumber = 1;

bool flying = false;

void sendCommand(const char* format, int seq, ...) {
  char command[50];
  va_list args;
  va_start(args, seq);
  vsnprintf(command, sizeof(command), format, args);
  va_end(args);

  udp.beginPacket(droneIp, dronePort);
  udp.write(command);
  udp.endPacket();
}

void receiveTelemetry() {
  int packetSize = udpTelemetry.parsePacket();
  if (packetSize) {
    char buffer[1024];
    int len = udpTelemetry.read(buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = 0;  // Null-terminate the buffer
      debugln("Received telemetry data:");
      for (int i = 0; i < len; i++) {
        Serial.print(buffer[i], HEX);  // Print raw bytes in HEX
        Serial.print(" ");
      }
      Serial.println();
    } else {
      debugln("No se recibió telemetria.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  boton.begin();
  up_fwd.begin();
  dw_bkw.begin();
  left.begin();
  right.begin();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    debugln("Conectando con el drone...");
  }

  debugln("Conectado!!!!");

  udp.begin(localPort);           // Local port to send UDP packets
  udpTelemetry.begin(telemetry);  // Local port for receiving telemetry

  digitalWrite(LED, HIGH);
  delay(1500);

  sendCommand(commands.setAlarmOk, sequenceNumber++);
  sendCommand(commands.flatTrim, sequenceNumber++);
  delay(1500);
}

void loop() {
  if (boton.pressed()) {
    debug("boton apretado -> ");
    if (!flying) {
      debugln("despegar.");
      sendCommand(commands.takeoff, sequenceNumber++);
    } else {
      debugln("aterrizarar.");
      sendCommand(commands.land, sequenceNumber++);
    }
    flying = !flying;
    debugln(sequenceNumber);
  }

  if (up_fwd.pressed()) {
    debug("up_fwd apretado -> ");
    if (!flying) {
      debugln("setAlarmOK.");
      sendCommand(commands.setAlarmOk, sequenceNumber++);
      sendCommand(commands.flatTrim, sequenceNumber++);
    } else {
    }
  }

  if (dw_bkw.pressed()) {
    debug("dw_bkw apretado -> ");
    if (!flying) {
      debugln("emergency.");
      sendCommand(commands.emergency, sequenceNumber++);
    } else {
    }
  }

  if (left.pressed()) {
    debug("left apretado -> ");
    if (!flying) {
      debugln("telemetry.");
      receiveTelemetry();
    } else {
    }
  }
}
