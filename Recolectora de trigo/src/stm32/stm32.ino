#include <SoftwareSerial.h>
SoftwareSerial SerialESP(D2, D8); // Pin D2 = RX, Pin D8 = TX


// --- SALIDAS DE POTENCIA ---
#define RELE_IGNICION   D3
#define RELE_MARCHA     D4
#define H_IN1           D5   // Puente H: Acelerar (+)
#define H_IN2           D6   // Puente H: Desacelerar (-)
#define H_ENA           D7   // PWM Puente H

// --- TIRA LED RGB ANALÓGICA (+12V, R, G, B) ---
#define LED_R           A0
#define LED_G           A1
#define LED_B           A2

// --- BOTONES FÍSICOS (Caja externa) ---
#define BTN_INICIO      D10  // Normalmente Abierto (NA) -> LOW al presionar
#define BTN_PARO        D11  // Normalmente Cerrado (NC) -> ¡DEBE IR A GND!
#define BTN_ACEL_MAS    D12  // Normalmente Abierto (NA) -> LOW al presionar
#define BTN_ACEL_MENOS  D13  // Normalmente Abierto (NA) -> LOW al presionar

enum EstadoSistema { ESPERA, DANDO_MARCHA, ENCENDIDO };
EstadoSistema estadoActual = ESPERA;

unsigned long tiempoInicioMarcha = 0;
const unsigned long DURACION_MARCHA = 2500;

unsigned long tiempoInicioAcel = 0;
bool moviendoAcelerador = false;
const unsigned long PULSO_ACELERADOR = 250;

void colorTiraRGB(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

void enviarEstado() {
  if (estadoActual == ESPERA) SerialESP.println("ESPERA");
  else if (estadoActual == DANDO_MARCHA) SerialESP.println("ARRANCANDO");
  else if (estadoActual == ENCENDIDO) SerialESP.println("ENCENDIDO");
}

void cambiarEstado(EstadoSistema nuevoEstado) {
  estadoActual = nuevoEstado;

  if (estadoActual == ESPERA) {
    digitalWrite(RELE_MARCHA, LOW);
    digitalWrite(RELE_IGNICION, LOW);
    colorTiraRGB(1, 1, 0); // Amarillo: Espera
  } 
  else if (estadoActual == DANDO_MARCHA) {
    digitalWrite(RELE_IGNICION, HIGH);
    digitalWrite(RELE_MARCHA, HIGH);
    colorTiraRGB(0, 0, 1); // Azul: Dando marcha
    tiempoInicioMarcha = millis();
  } 
  else if (estadoActual == ENCENDIDO) {
    digitalWrite(RELE_MARCHA, LOW);
    digitalWrite(RELE_IGNICION, HIGH);
    colorTiraRGB(0, 1, 0); // Verde: Motor encendido
  }
  enviarEstado();
}

void activarAcelerador(bool subir) {
  if (estadoActual != ENCENDIDO) return;
  analogWrite(H_ENA, 190);
  if (subir) {
    digitalWrite(H_IN1, HIGH);
    digitalWrite(H_IN2, LOW);
  } else {
    digitalWrite(H_IN1, LOW);
    digitalWrite(H_IN2, HIGH);
  }
  tiempoInicioAcel = millis();
  moviendoAcelerador = true;
}

void setup() {
SerialESP.begin(9600);
 // Comunicación con ESP32 en D2 y D8

  pinMode(RELE_IGNICION, OUTPUT);
  pinMode(RELE_MARCHA, OUTPUT);
  pinMode(H_IN1, OUTPUT);
  pinMode(H_IN2, OUTPUT);
  pinMode(H_ENA, OUTPUT);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BTN_INICIO, INPUT_PULLUP);
  pinMode(BTN_PARO, INPUT_PULLUP);
  pinMode(BTN_ACEL_MAS, INPUT_PULLUP);
  pinMode(BTN_ACEL_MENOS, INPUT_PULLUP);

  cambiarEstado(ESPERA);
}

void loop() {
  // 1. Prioridad máxima: Botón físico de PARO (NC conectado a GND)
  if (digitalRead(BTN_PARO) == HIGH && estadoActual != ESPERA) {
    cambiarEstado(ESPERA);
  }

  // 2. Botones físicos
  if (digitalRead(BTN_INICIO) == LOW && estadoActual == ESPERA) {
    delay(50);
    if (digitalRead(BTN_INICIO) == LOW) cambiarEstado(DANDO_MARCHA);
  }
  if (digitalRead(BTN_ACEL_MAS) == LOW && !moviendoAcelerador) {
    activarAcelerador(true);
  }
  if (digitalRead(BTN_ACEL_MENOS) == LOW && !moviendoAcelerador) {
    activarAcelerador(false);
  }

  // 3. Comandos desde Python / Celular (vía ESP32)
  if (SerialESP.available()) {
    char cmd = SerialESP.read();
    if (cmd == 'A' && estadoActual == ESPERA) {
      cambiarEstado(DANDO_MARCHA);
    } else if (cmd == 'P') {
      cambiarEstado(ESPERA);
    } else if (cmd == '+') {
      activarAcelerador(true);
    } else if (cmd == '-') {
      activarAcelerador(false);
    } else if (cmd == 'S') {
      enviarEstado();
    }
  }

  // 4. Soltar marcha automáticamente tras 2.5 segundos
  if (estadoActual == DANDO_MARCHA) {
    if (millis() - tiempoInicioMarcha >= DURACION_MARCHA) {
      cambiarEstado(ENCENDIDO);
    }
  }

  // 5. Detener motor del acelerador tras el pulso de ajuste
  if (moviendoAcelerador && (millis() - tiempoInicioAcel >= PULSO_ACELERADOR)) {
    digitalWrite(H_IN1, LOW);
    digitalWrite(H_IN2, LOW);
    analogWrite(H_ENA, 0);
    moviendoAcelerador = false;
  }
}
