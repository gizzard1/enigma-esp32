/*
   CÓDIGO DEL EMISOR (ESP32 A)
   Este código envía un mensaje cada 2 segundos al otro ESP32.
*/

#include <Arduino.h>
#include <ArduinoJson.h>
// Usaremos D27 como RX y D14 como TX
#define RXD2 27
#define TXD2 14

// =========================================================
//  LÓGICA ENIGMA
// =========================================================
const char* ALFABETO = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char* REFLECTOR_B = "YRUHQSLDPXNGOKMIEBFZCWVJAT";

// Array de rotores
struct RotorInfo {
  const char* wiring;
  char notch;
};

const RotorInfo ROTORES[] = {
  {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q'},  // I
  {"AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E'},  // II
  {"BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V'},  // III
  {"ESOVPZJAYQUIRHXLNFTGKDCMWB", 'J'},  // IV
  {"VZBRGITYUPSDNHLXAWMJQOFEKC", 'Z'}   // V
};

// Función para obtener índice del rotor
int getRotorIndex(const String& name) {
  if (name == "I") return 0;
  if (name == "II") return 1;
  if (name == "III") return 2;
  if (name == "IV") return 3;
  if (name == "V") return 4;
  return 0; // Default to I
}

// Función sanitize optimizada
String sanitize(String msg) {
  String out = "";
  msg.toUpperCase();
  for (size_t i = 0; i < msg.length(); i++) {
    char c = msg.charAt(i);
    if (c >= 'A' && c <= 'Z') out += c;
  }
  return out;
}

// Funciones rotor optimizadas
char rotor_forward(char c, const char* wiring, int offset) {
  int idx = (c - 'A' + offset) % 26;
  return wiring[idx];
}

char rotor_backward(char c, const char* wiring, int offset) {
  // Buscar el carácter en el wiring
  for (int i = 0; i < 26; i++) {
    if (wiring[i] == c) {
      return 'A' + ((i - offset + 26) % 26);
    }
  }
  return c;
}

char reflector(char c) {
  int idx = c - 'A';
  if (idx >= 0 && idx < 26) {
    return REFLECTOR_B[idx];
  }
  return c;
}

void step_positions(int positions[], const String rotors[], int numRotors) {
  bool doblePaso = false;
  
  // Avanzar primer rotor
  positions[0] = (positions[0] + 1) % 26;
  
  // Verificar muescas
  for (int i = 0; i < numRotors - 1; i++) {
    int rotorIdx = getRotorIndex(rotors[i]);
    char notch = ROTORES[rotorIdx].notch;
    
    if (positions[i] == (notch - 'A') || doblePaso) {
      positions[i + 1] = (positions[i + 1] + 1) % 26;
      
      // Verificar doble paso
      if (i + 1 < numRotors) {
        int nextIdx = getRotorIndex(rotors[i + 1]);
        char nextNotch = ROTORES[nextIdx].notch;
        doblePaso = (positions[i + 1] == (nextNotch - 'A'));
      }
    } else {
      break;
    }
  }
}

String enigma_process(String msg, String rotors[], int positions[], int numRotors) {
  String out = "";
  msg = sanitize(msg);
  
  for (size_t i = 0; i < msg.length(); i++) {
    char c = msg.charAt(i);
    
    // Avanzar posiciones
    step_positions(positions, rotors, numRotors);
    
    // Forward a través de rotores
    char signal = c;
    for (int r = 0; r < numRotors; r++) {
      int rotorIdx = getRotorIndex(rotors[r]);
      signal = rotor_forward(signal, ROTORES[rotorIdx].wiring, positions[r]);
    }
    
    // Reflector
    signal = reflector(signal);
    
    // Backward a través de rotores
    for (int r = numRotors - 1; r >= 0; r--) {
      int rotorIdx = getRotorIndex(rotors[r]);
      signal = rotor_backward(signal, ROTORES[rotorIdx].wiring, positions[r]);
    }
    
    out += signal;
  }
  
  return out;
}

void setup() {
  Serial.begin(115200);
  // Nota: Serial2.begin(baudios, modo, pin_RX, pin_TX)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); 
  Serial.println("EMISOR listo en pines 14(TX) y 27(RX)");
}

int leerNumero() {
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  return input.toInt();
}

String leerTexto() {
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toUpperCase();
  return input;
}

void mostrarRotoresDisponibles() {
  Serial.println("\n-----------------------------");
  Serial.println(" ROTORES DISPONIBLES");
  Serial.println("-----------------------------");
  Serial.println("I   - Notch: Q");
  Serial.println("II  - Notch: E"); 
  Serial.println("III - Notch: V");
  Serial.println("IV  - Notch: J");
  Serial.println("V   - Notch: Z");
  Serial.println("-----------------------------");
}

bool rotorValido(const String& nombreRotor) {
  for (int i = 0; i < 5; i++) {
    if (nombreRotor == "I" || nombreRotor == "II" || nombreRotor == "III" || 
        nombreRotor == "IV" || nombreRotor == "V") {
      return true;
    }
  }
  return false;
}

void loop() {

  Serial.println("\n=================================");
  Serial.println("=== INGRESO DE NUEVO MENSAJE ===");
  Serial.println("=================================");
  Serial.println("Escribe el mensaje (solo A-Z, se convertirán a mayúsculas):");
  String msg = leerTexto();

  Serial.println("\n¿Cuántos rotores quieres usar? (3-5):");
  int numRotores = 0;
  while (numRotores < 3 || numRotores > 5) {
    numRotores = leerNumero();
    if (numRotores < 3 || numRotores > 5) {
      Serial.println("Error: Debe ser entre 3 y 5. Intenta de nuevo:");
    }
  }

  mostrarRotoresDisponibles();

  String rotors[5]; 
  int positions[5] = {0, 0, 0, 0, 0}; 
  
  Serial.println("\nSelecciona los rotores en orden (de izquierda a derecha):");
  
  for (int i = 0; i < numRotores; i++) {
    Serial.printf("Rotor %d/%d (I,II,III,IV,V): ", i + 1, numRotores);
    String rotor = leerTexto();
    
    while (!rotorValido(rotor)) {
      Serial.println("Rotor no válido. Usa I,II,III,IV,V:");
      rotor = leerTexto();
    }
    
    rotors[i] = rotor;
  }

  Serial.println("\n-----------------------------");
  Serial.println(" CONFIGURACIÓN FINAL");
  Serial.println("-----------------------------");
  Serial.print("Rotores: ");
  for (int i = 0; i < numRotores; i++) {
    Serial.print(rotors[i]);
    if (i < numRotores - 1) Serial.print(" - ");
  }
  Serial.println();
  Serial.print("Posiciones: ");
  for (int i = 0; i < numRotores; i++) {
    Serial.print("A");
    if (i < numRotores - 1) Serial.print(" - ");
  }
  Serial.println();

  String cifrado = enigma_process(msg, rotors, positions, numRotores);
  if ( cifrado == ""){
    msg = 'C====3';
    cifrado = 'C====3';
  }
  Serial.println("Mensaje original: " + msg);
  Serial.println("Mensaje cifrado: " + cifrado);

  StaticJsonDocument<512> doc;
  doc["mensaje"] = cifrado;
  JsonArray rot = doc.createNestedArray("rotors");
  for (int i = 0; i < numRotores; i++) {
    rot.add(rotors[i]);
  }
  JsonArray pos = doc.createNestedArray("pos");
  for (int i = 0; i < numRotores; i++) {
    pos.add("A");
  }
  doc["numRotores"] = numRotores;

  String json;
  serializeJson(doc, json);

  Serial2.println(json);
}
