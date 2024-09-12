#include <LoRa.h>                   // Biblioteca para manejar la comunicación LoRa
#include <SPI.h>                    // Biblioteca para la comunicación SPI con el módulo LoRa
#include <WiFi.h>                   // Biblioteca para manejar la conectividad Wi-Fi
#include <ThingsBoard.h>            // Biblioteca para conectarse a ThingsBoard
#include <Arduino_MQTT_Client.h>    // Biblioteca para manejar la comunicación MQTT

// Pines asignados para el módulo LoRa
#define ss 5    // Pin para CS (Chip Select)
#define rst 2   // Pin para el RESET
#define dio0 4  // Pin para las interrupciones

// Credenciales de la red Wi-Fi
const char* ssid = "";  // Nombre de la red Wi-Fi
const char* pass = "12345689";        // Contraseña de la red Wi-Fi

// Credenciales de conexión a ThingsBoard
#define TB_SERVER "thingsboard.cloud" // URL del servidor de ThingsBoard
#define TOKEN "BMBDsh59a2qMMiuonu5Z"  // Token de autenticación para ThingsBoard

// Tamaño máximo de mensaje para ThingsBoard
constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

// Inicialización del cliente Wi-Fi y ThingsBoard
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

// Función para conectar el ESP32 a la red Wi-Fi
void connectToWiFi() {
  //Serial.println("Conectando al WiFi...");
  int intentos = 0;

  // Intentar conectar a Wi-Fi hasta un máximo de 20 intentos
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.mode(WIFI_STA);  // Modo estación (cliente Wi-Fi)
    WiFi.begin(ssid, pass);  // Conectar a la red con el SSID y contraseña
    delay(500);  // Pausa para dar tiempo a la conexión
    //Serial.print(".");
    intentos++;
  }

  // Verificar si la conexión fue exitosa o falló
  //if (WiFi.status() != WL_CONNECTED) {
  //  Serial.println("Falla al conectar al WiFi.");
  //} else {
  //  Serial.println("Conectado al WiFi");
  //}
}

// Función para conectarse a ThingsBoard
void connectToThingsBoard() {
  if (!tb.connected()) {  // Verificar si ya está conectado
    //Serial.println("Conectando ThingsBoard server");

    // Intentar conectarse al servidor de ThingsBoard
    if (!tb.connect(TB_SERVER, TOKEN)) {
    //  Serial.println("Falla al conectar a ThingsBoard");
    } else {
    //  Serial.println("Conectado to ThingsBoard");
    }
  }
}

// Función para enviar datos a ThingsBoard
void sendDataToThingsBoard(float lat, float lon, float speed) {
  // Crear un mensaje en formato JSON con los datos de latitud, longitud y velocidad
  String jsonData = "{\"latitude\":" + String(lat, 6) + ", \"longitude\":" + String(lon, 6) + ", \"speed\":" + String(speed, 2) + "}";
  
  // Enviar los datos de telemetría a ThingsBoard
  tb.sendTelemetryJson(jsonData.c_str());
  //Serial.println("Datos enviados a ThingsBoard: " + jsonData);
}

// Función setup, que se ejecuta una sola vez al inicio
void setup() {
  //Serial.begin(115200);  // Iniciar la comunicación serial a 115200 baudios

  // Configurar los pines del módulo LoRa
  LoRa.setPins(ss, rst, dio0);
  
  // Iniciar el módulo LoRa en la frecuencia de 433 MHz
  if (!LoRa.begin(433E6)){  
    //Serial.println("Falla al inicilizar LoRa");
    while (1);  // Si falla, se queda en un bucle infinito
  }

  // Conectar a Wi-Fi
  connectToWiFi();

  // Conectar a ThingsBoard
  connectToThingsBoard();
}

// Función loop, que se ejecuta continuamente
void loop() {
  int packetSize = LoRa.parsePacket();  // Verificar si hay un paquete de datos disponible
  
  if (packetSize) {  // Si se recibió un paquete
    //Serial.println("Paquete recibido:");

    String incoming = "";  // Variable para almacenar los datos recibidos
    while (LoRa.available()) {
      incoming += (char)LoRa.read();  // Leer cada carácter del paquete y almacenarlo
    }

    // Procesar los datos recibidos y extraer latitud, longitud y velocidad
    int latIndex = incoming.indexOf("LAT:");       // Buscar la posición de "LAT:"
    int lonIndex = incoming.indexOf(",LONG:");     // Buscar la posición de "LONG:"
    int speedIndex = incoming.indexOf(",SPEED:");  // Buscar la posición de "SPEED:"

    // Convertir las subcadenas de latitud, longitud y velocidad en valores flotantes
    float lat = incoming.substring(latIndex + 4, lonIndex).toFloat();
    float lon = incoming.substring(lonIndex + 6, speedIndex).toFloat();
    float speed = incoming.substring(speedIndex + 7).toFloat();

    // Imprimir los datos recibidos en el monitor serial
    // String latString = String(lat, 6);
    // String lonString = String(lon, 6);
    // String speedString = String(speed, 2) + " kmph";

    // Serial.print("Latitude: ");
    // Serial.println(latString);
    // Serial.print("Longitude: ");
    // Serial.println(lonString);
    // Serial.print("Speed: ");
    // Serial.println(speedString);

    // // Mostrar la intensidad de la señal recibida
    // Serial.print("RSSI: ");
    // Serial.println(LoRa.packetRssi());

    // Verificar si la conexión con ThingsBoard está activa
    if (!tb.connected()) {
      connectToThingsBoard();  // Si no está conectado, reconectar
    }

    // Enviar los datos a ThingsBoard
    sendDataToThingsBoard(lat, lon, speed);
  }

  tb.loop();  // Mantener la conexión con ThingsBoard activa
}
