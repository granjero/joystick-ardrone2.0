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

// led
//#define LED D8

//botones
#define BOTON D6
#define BOTON_MODO D7
#define RIGHT D1
#define UP_FWD D2
#define DW_BKW D3
#define LEFT D4

Button boton(BOTON, 750);
Button boton_modo(BOTON_MODO, 750);
Button up_fwd(UP_FWD, 750);
Button dw_bkw(DW_BKW, 750);
Button left(LEFT, 750);
Button right(RIGHT, 750);

WiFiUDP udp;
WiFiUDP udpTelemetry;


// constantes y variables
const char* droneIp = "192.168.1.1";
const int localPort = 8888;
const int dronePort = 5556;
const int telemetryPort = 5554;
const char* ssid = "enarrdo";
const char* password = "";

int sequenceNumber = 1;

bool flying = false;

// comandos drone
struct DroneCommands {
  const char* takeoff;
  const char* land;
  const char* move;
  const char* emergency;
  const char* flatTrim;
  const char* setMaxAltitude;
  const char* setMaxTilt;
  const char* setAlarmOk;
  const char* enableNavData;
};

DroneCommands commands = {
  .takeoff = "AT*REF=%d,290718208\r",
  .land = "AT*REF=%d,290717696\r",
  .move = "AT*PCMD=%d,%d,%d,%d,%d,%d\r",
  .emergency = "AT*REF=%d,290717952\r",
  .flatTrim = "AT*FTRIM=%d\r",
  .setMaxAltitude = "AT*CONFIG=%d,\"control:altitude_max\",\"%d\"\r",
  .setMaxTilt = "AT*CONFIG=%d,\"control:euler_angle_max\",\"%f\"\r",
  .setAlarmOk = "AT*CONFIG=%d,\"general:navdata_demo\",\"TRUE\"\r",
  .enableNavData = "AT*CONFIG=%d,\"general:navdata_demo\",\"FALSE\"\r",
};


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

// void receiveTelemetry() {
//   int packetSize = udpTelemetry.parsePacket();
//   if (packetSize) {
//     char buffer[1024];
//     int len = udpTelemetry.read(buffer, sizeof(buffer) - 1);
//     if (len > 0) {
//       buffer[len] = 0;              // Null-terminate the buffer
//       parseTelemetry(buffer, len);  // Parse the telemetry data
//     }
//   }
// }
//
// void parseTelemetry(const char* data, int length) {
//   // Parse the header
//   uint32_t header = *(uint32_t*)(data);
//   uint32_t droneState = *(uint32_t*)(data + 4);
//   uint32_t sequenceNumber = *(uint32_t*)(data + 8);
//   uint32_t visionFlag = *(uint32_t*)(data + 12);
//
//   Serial.print("Header: ");
//   Serial.println(header, HEX);
//   Serial.print("Drone State: ");
//   Serial.println(droneState, HEX);
//   Serial.print("Sequence Number: ");
//   Serial.println(sequenceNumber);
//   Serial.print("Vision Flag: ");
//   Serial.println(visionFlag, HEX);
//
//   // Parse navigation tags
//   int offset = 16;  // Start of navigation tags
//   while (offset < length) {
//     uint16_t tagId = *(uint16_t*)(data + offset);
//     uint16_t tagLength = *(uint16_t*)(data + offset + 2);
//
//     switch (tagId) {
//       case 0:  // Demo tag (contains basic telemetry)
//         parseDemoTag(data + offset + 4, tagLength);
//         break;
//         // Add more cases for other tags as needed
//     }
//
//     offset += 4 + tagLength;  // Move to the next tag
//   }
// }
//
// void parseDemoTag(const char* data, int length) {
//   // Parse demo tag data
//   uint32_t controlState = *(uint32_t*)(data);
//   uint32_t batteryLevel = *(uint32_t*)(data + 36);
//   float altitude = *(int32_t*)(data + 24) / 1000.0;  // Convert to meters
//   float theta = *(int16_t*)(data + 12) / 1000.0;     // Pitch angle in radians
//   float phi = *(int16_t*)(data + 16) / 1000.0;       // Roll angle in radians
//   float psi = *(int16_t*)(data + 20) / 1000.0;       // Yaw angle in radians
//
//   Serial.print("Battery Level: ");
//   Serial.println(batteryLevel);
//   Serial.print("Altitude: ");
//   Serial.println(altitude);
//   Serial.print("Pitch: ");
//   Serial.println(theta);
//   Serial.print("Roll: ");
//   Serial.println(phi);
//   Serial.print("Yaw: ");
//   Serial.println(psi);
// }

void setup() {
  Serial.begin(115200);

  //pinMode(BOTON_MODO, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  boton.begin();
  boton_modo.begin();
  up_fwd.begin();
  dw_bkw.begin();
  left.begin();
  right.begin();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < 5; i++) {
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
      digitalWrite(LED_BUILTIN, HIGH);
    }
    debugln("Conectando con el drone...");
  }
  debugln("Conectado!!!!");

  digitalWrite(LED_BUILTIN, LOW);
  delay(1500);

  udp.begin(localPort);               // Local port to send UDP packets
  // udpTelemetry.begin(telemetryPort);  // Local port for receiving telemetry
}

void loop() {
  bool modo = boton_modo.read();
  bool btn = boton.read ();
  bool up = up_fwd.read();
  bool down = dw_bkw.read();
  bool iz = left.read();
  bool de = right.read();


  debugln(modo);
  debugln(btn);

  // if (btn) {
  //   debug("boton apretado -> ");
  //   if (!flying) {
  //     debugln("despegar.");
  //     sendCommand(commands.takeoff, sequenceNumber++);
  //   } else {
  //     debugln("aterrizarar.");
  //     sendCommand(commands.land, sequenceNumber++);
  //   }
  //   flying = !flying;
  //   debugln(sequenceNumber);
  // }
  //
  // else if (up) {
  //   debug("up_fwd apretado -> ");
  //   if (!flying) {
  //     debugln("setAlarmOK.");
  //     sendCommand(commands.setAlarmOk, sequenceNumber++);
  //     sendCommand(commands.flatTrim, sequenceNumber++);
  //   } else {
  //   }
  // }
  //
  // else if (down) {
  //   debug("dw_bkw apretado -> ");
  //   if (!flying) {
  //     debugln("emergency.");
  //     sendCommand(commands.emergency, sequenceNumber++);
  //   } else {
  //   }
  // }
  //
  // else if (iz) {
  //   debug("left apretado -> ");
  //   if (!flying) {
  //     debugln("telemetry.");
  //     sendCommand(commands.setAlarmOk, sequenceNumber++);
  //     delay(1000);
  //     receiveTelemetry();
  //   } else {
  //   }
  // }
  //
  // else if (de) {
  //   debug("right apretado -> ");
  //   if (!flying) {
  //     debugln("telemetry.");
  //     receiveTelemetry();
  //   } else {
  //   }
  // }
  // receiveTelemetry();

}
