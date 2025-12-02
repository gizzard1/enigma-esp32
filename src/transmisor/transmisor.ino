/*
   CÓDIGO DEL RECEPTOR (ESP32 B)
   Este código escucha si llegan datos y los muestra en pantalla.
*/

#define RXD2 27
#define TXD2 14

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("RECEPTOR listo en pines 14(RX) y 27(TX)");
}

void loop() {
  // Verificamos si hay datos llegando por el cable
  if (Serial2.available()) {
    
    // Leemos el mensaje completo hasta el salto de línea
    String mensajeRecibido = Serial2.readStringUntil('\n');
    
    // Lo mostramos en el Monitor Serial de la computadora
    Serial.print("Mensaje recibido por cable: ");
    Serial.println(mensajeRecibido);
  }
}
