#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "zacerieth";
const char* password = "19451113";

WebServer server(80);
#define RXD2 16
#define TXD2 17

void procesarOrden() {
  if (server.hasArg("cmd")) {
    String cmdStr = server.arg("cmd");
    if (cmdStr.length() > 0) {
      // 1. Limpia mensajes viejos del buffer
      while (Serial2.available()) {
        Serial2.read();
      }

      // 2. Envía el comando ('A', 'P', '+', '-', 'S') a la STM32 Nucleo
      Serial2.write(cmdStr.charAt(0));

      // 3. Espera hasta 120 ms a que la STM32 responda
      unsigned long inicioEspera = millis();
      while (!Serial2.available() && (millis() - inicioEspera < 120)) {
        delay(2);
      }

      String respuesta = "ESPERA,0,1";
      if (Serial2.available()) {
        respuesta = Serial2.readStringUntil('\n');
        respuesta.trim();
      }

      server.send(200, "text/plain", respuesta);
      return;
    }
  }
  server.send(400, "text/plain", "ERROR");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // 9600 baudios hacia STM32 (D2 y D8)
  Serial2.setTimeout(80);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n¡Listo! IP de la ESP32: " + WiFi.localIP().toString());

  server.on("/api", procesarOrden);
  server.begin();
}

void loop() {
  server.handleClient();
}
