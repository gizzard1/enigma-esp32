/*
   CÓDIGO DEL RECEPTOR (ESP32 B)
   Este código escucha si llegan datos y los muestra en pantalla.
*/

#define RXD2 27
#define TXD2 14
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// =========================================================
// !!! CONFIGURACIÓN WI-FI Y TELEGRAM !!!
// =========================================================
// Reemplaza con tu Bot Token real de Telegram
#define BOT_TOKEN "BOT_TOKEN" // <--- REEMPLAZAR
// Reemplaza con tu Chat ID real de Telegram
#define CHAT_ID "CHAT_ID" // <--- REEMPLAZAR
// Reemplaza con el nombre de tu red Wi-Fi
#define WIFI_SSID "SSID_NAME" // <--- REEMPLAZAR
// Reemplaza con la contraseña de tu red Wi-Fi
#define WIFI_PASS "PASS_WIFI" // <--- REEMPLAZAR

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

// =========================================================
//  FUNCIONES WIFI Y TELEGRAM (Estabilidad BT/WiFi)
// =========================================================
bool wifiConnected = false;

void setCustomDNS() {
    IPAddress dns(8, 8, 8, 8);
    WiFi.setDNS(dns); 
}

bool connectWiFi() {
    Serial.print("Conectando WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS); 
    
    for(int i = 0; i < 60; i++) { 
        if(WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            setCustomDNS(); 
            Serial.println(" ✔");
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println(" ❌");
    wifiConnected = false;
    return false;
}

void syncTime() { 
    Serial.print("Sincronizando hora (NTP)...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); 
    time_t now = time(nullptr);
    while (now < 100000) { delay(500); now = time(nullptr); Serial.print("."); }
    Serial.println(" ✔");
}


void enviarTelegram(const String& mensaje) {
    for (int attempt = 0; attempt < 2; attempt++) { 
        Serial.print("\nEnviando a Telegram (Intento " + String(attempt + 1) + ")... ");
        
        if(!wifiConnected || WiFi.status() != WL_CONNECTED) {
            if(!connectWiFi()) { 
                Serial.println("No hay conexión WiFi.");
                if (attempt == 0) continue; 
                return;
            }
        }
        
        HTTPClient http;
        String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage";
        
        String encodedMsg = mensaje;
        encodedMsg.replace(" ", "%20");
        encodedMsg.replace("\n", "%0A");
        
        String payload = "chat_id=" + String(CHAT_ID) + "&text=" + encodedMsg;
        
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.setTimeout(30000); 
        
        int httpCode = http.POST(payload);
        
        Serial.print("HTTP "); Serial.println(httpCode);
        
        if(httpCode == 200) {
            Serial.println("✔ ¡ENVIADO A TELEGRAM!");
            http.end(); 
            return; 
        } else {
            Serial.println("❌ Falló el envío.");
            http.end(); 
            if (attempt == 0) {
                WiFi.disconnect(true); 
                wifiConnected = false;
            }
        }
    }
}
// =========================================================

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("RECEPTOR listo en pines 14(RX) y 27(TX)");

  if(connectWiFi()) { 
      syncTime();
      enviarTelegram("ENIGMA+RECEPTOR+INICIADO+✅");
  } else {
      Serial.println("❌ Test inicial fallido (No se pudo enviar a Telegram).");
  }
}

void loop() {
  // Verificamos si hay datos llegando por el cable
  if (Serial2.available()) {
    
    // Leemos el mensaje completo hasta el salto de línea
    String json = Serial2.readStringUntil('\n');
    
    json.trim();

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
      Serial.println("\n❌ JSON inválido: " + String(err.c_str()));
      return;
    }

    String cifrado = doc["mensaje"].as<String>();
    int numRotores = doc["numRotores"] | 3;
    
    String rotors[5];
    int positions[5] = {0, 0, 0, 0, 0}; 
    
    JsonArray rotorsArray = doc["rotors"];
    for (int i = 0; i < numRotores && i < rotorsArray.size(); i++) {
      rotors[i] = rotorsArray[i].as<String>();
    }

    String descifrado = enigma_process(cifrado, rotors, positions, numRotores);

    Serial.println("\n-----------------------------");
    Serial.println(" MENSAJE RECIBIDO");
    Serial.println("-----------------------------");
    Serial.println("Cifrado: " + cifrado);
    Serial.println("Descifrado: " + descifrado);
    Serial.println("-----------------------------");

    // 1. Responder
    Serial2.println(descifrado);
    
    // 2. Enviar a Telegram
    String msgTelegram = "ENIGMA+MENSAJE%0ACIFRADO:+" + cifrado + "%0ADESCIFRADO:+" + descifrado;
    enviarTelegram(msgTelegram); 
  }
}
