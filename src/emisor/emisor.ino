/*
   CÓDIGO DEL EMISOR (ESP32 A)
   Este código envía un mensaje cada 2 segundos al otro ESP32.
*/

// Usaremos D27 como RX y D14 como TX
#define RXD2 27
#define TXD2 14

void setup() {
  Serial.begin(115200);
  // Nota: Serial2.begin(baudios, modo, pin_RX, pin_TX)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); 
  Serial.println("EMISOR listo en pines 14(TX) y 27(RX)");
}

void loop() {
  // Mensaje de prueba
  String mensaje = "Hola Receptor, probando 1-2-3";
  
  // Enviamos el mensaje por el puerto Serial 2 (hacia el otro ESP32)
  Serial2.println(mensaje);
  
  // También lo mostramos en la pantalla de la PC para saber qué está pasando
  Serial.print("Enviando: ");
  Serial.println(mensaje);

  // Esperamos 2 segundos antes de enviar de nuevo
  delay(2000); 
}
