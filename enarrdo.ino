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


void setup() {
  Serial.begin(115200);

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

  udp.begin(localPort);
}

void loop() {
  // como estan pullupeados los botones para que tenga sentido los doy vuelta
  bool btn = boton.pressed();
  bool modo = boton_modo.pressed();
  bool arr = up_fwd.pressed();
  bool aba = dw_bkw.pressed();
  bool izq = left.pressed();
  bool der = right.pressed();
  // volando me interesa el estado y no si fue apretado
  bool btn_state = !boton.read();
  bool modo_state = !boton_modo.read();
  bool arr_state = !up_fwd.read();
  bool aba_state = !dw_bkw.read();
  bool izq_state = !left.read();
  bool der_state = !right.read();


  // *******************************
  // flying
  // *******************************
  if (flying) {
    if (modo_state) {
      // *******************************
      // MODO apretado
      if (btn) {
        // *******************************
        // boton apretado
        debug("boton apretado -> ");
        flying = false;
        debug("aterrizar. seq#: ");
        sendCommand(commands.land, sequenceNumber++);
        debugln(sequenceNumber);
      }

      else if (arr_state) {
        // *******************************
        // boton arriba
        debug("volando - > boton arriba apretado -> ");
        debug("Subir. seq#: ");
        // comando para subir
        debugln(sequenceNumber);
      }

      else if (aba) {
        // *******************************
        // boton abajo
        debug(" volando - > boton abajo apretado -> ");
        debug("Bajar. seq#: ");
        sendCommand(commands.emergency, sequenceNumber++);
        debugln(sequenceNumber);
      }

      else if (der) {
        // *******************************
        // boton derecha
        debugln("boton derecha apretado -> ");
      }

      else if (izq) {
        // *******************************
        // boton izquierda
        debugln("boton izq apretado -> ");
      }
    } else {
      // *******************************
      // MODO suelto
      if (btn) {
        // *******************************
        // boton apretado
        debug("boton apretado -> ");
        flying = false;
        debug("aterrizar. seq#: ");
        sendCommand(commands.land, sequenceNumber++);
        debugln(sequenceNumber);
      }
    }


  } else {
    // *******************************
    // NOT FLYING
    // *******************************
    if (modo_state) {
      // *******************************
      // MODO apretado
      if (btn) {
        // *******************************
        // boton apretado
        debugln("boton apretado -> emergency.");
        sendCommand(commands.emergency, sequenceNumber++);
      }
    } else {
      // *******************************
      // MODO suelto
      if (btn) {
        // *******************************
        // boton apretado
        debug("boton apretado -> ");
        flying = true;
        debug("despegar. seq#: ");
        sendCommand(commands.takeoff, sequenceNumber++);
        debugln(sequenceNumber);
      }

      else if (arr) {
        // *******************************
        // boton arriba
        debug("no volando - > boton arriba apretado -> ");
        debugln("Set Alarm OK.");
        sendCommand(commands.setAlarmOk, sequenceNumber++);
        sendCommand(commands.flatTrim, sequenceNumber++);
      }

      else if (aba) {
        // *******************************
        // boton abajo
        debug("no volando - > boton abajo apretado -> ");
        debug("Emergency. seq#: ");
        sendCommand(commands.emergency, sequenceNumber++);
        debugln(sequenceNumber);
      }

      else if (der) {
        // *******************************
        // boton derecha
        debugln("boton derecha apretado -> ");
      }

      else if (izq) {
        // *******************************
        // boton izquierda
        debugln("boton izq apretado -> ");
      }
    }
  }
}

