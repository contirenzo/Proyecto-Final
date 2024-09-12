#include <TinyGPS++.h>          // Biblioteca para manejar el GPS y procesar los datos NMEA
#include <SoftwareSerial.h>     // Biblioteca para manejar la comunicación serial por software (utilizada para el GPS)
#include <LoRa.h>               // Biblioteca para manejar la comunicación LoRa
#include <avr/sleep.h>          // Biblioteca para manejar el modo de sueño del Arduino (bajo consumo)
#include <avr/power.h>          // Biblioteca para apagar periféricos y ahorrar energía
#include <avr/wdt.h>            // Biblioteca para configurar el temporizador Watchdog

// Pines para la comunicación con el GPS
#define rxGPS 4                 // Pin de recepción de datos del GPS
#define txGPS -1                // Pin de transmisión de datos del GPS (no utilizado)
#define gpsControlPin 5         // Pin de control del encendido y apagado del GPS

// Pines para el módulo LoRa
#define ss 10                   // Pin de CS (Chip Select) para el LoRa
#define rst 8                   // Pin de reinicio del LoRa
#define dio0 9                  // Pin para la interrupción del LoRa

// Objeto GPS
TinyGPSPlus gps;                
SoftwareSerial gpsSerial(rxGPS, txGPS);  // Comunicación serial por software con el GPS

volatile int f_wdt = 1;  // Flag para el temporizador Watchdog

// Interrupción del temporizador Watchdog
ISR(WDT_vect) {
  if (f_wdt == 0) {
    f_wdt = 1;  // Cambiar el flag cuando el temporizador expire
  } else {
    WDTCSR |= _BV(WDIE);  // Habilitar interrupción Watchdog
  }
}

// Configuración inicial del sistema
void setup() {
  
  //Serial.begin(9600);        // Iniciar comunicación serial para depuración a 9600 baudios
  gpsSerial.begin(9600);     // Iniciar comunicación serial con el GPS a 9600 baudios
 
  pinMode(gpsControlPin, OUTPUT);  // Configurar el pin de control del GPS como salida
  digitalWrite(gpsControlPin, LOW);  // Mantener el GPS apagado inicialmente
  
  // Inicializar el módulo LoRa con los pines definidos
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {  // Configurar LoRa en la frecuencia de 433 MHz
  //  Serial.println("Falla al inicializar Lora");
    while (1);  // Si falla, quedarse en un bucle infinito
  }
  //Serial.println("LoRa inicializado");
 
  // Configurar el modo sleep para ahorrar energía
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

// Bucle principal del sistema
void loop() {
  //Serial.println("Enciendo GPS");
  digitalWrite(gpsControlPin, HIGH);  // Encender el GPS

  bool gpsDataAcquired = false;       // Flag para verificar si se obtuvieron datos GPS
  unsigned long startTime = millis(); // Tiempo inicial de espera
  unsigned long timeout = 60000;     // Tiempo máximo de espera (1 minuto) para adquirir datos GPS

  // Esperar hasta que se obtengan datos GPS o se exceda el tiempo de espera
  while ((millis() - startTime < timeout) && !gpsDataAcquired) {
    if (gpsSerial.available()) {  // Si hay datos disponibles del GPS
      if (gps.encode(gpsSerial.read())) {  // Procesar los datos GPS
        if (gps.location.isValid()) {  // Verificar si la ubicación es válida
          gpsDataAcquired = true;  // Se obtuvieron datos GPS válidos
          Serial.println("Acquired");
        }
      }
    }
  }

  // Si se obtuvieron datos GPS válidos
  if (gpsDataAcquired) {
    // Guardar los datos GPS en variables
    String lat = String(gps.location.lat(), 6);
    String lon = String(gps.location.lng(), 6);
    String speed = String(gps.speed.mps());

    // Imprimir los datos GPS en el monitor serial
    // Serial.println("GPS data acquired...");
    // Serial.println("LAT: " + lat);
    // Serial.println("LONG: " + lon);
    // Serial.println("SPEED: " + speed);

    // Preparar los datos GPS en una cadena para enviar
    String dataToSend = "LAT:" + lat + ",LONG:" + lon + ",SPEED:" + speed;

    // Iniciar la transmisión de datos por LoRa
    LoRa.beginPacket();
    LoRa.print(dataToSend);  // Enviar la cadena de datos GPS
    int result = LoRa.endPacket();  // Finalizar la transmisión

    // Verificar si la transmisión LoRa fue exitosa
    // if (result) {
    //   Serial.println("Transmisión exitosa");
    // } else {
    //   Serial.println("Falla de transimisión");
    // }
  } else {
    // Si no se pudieron adquirir datos GPS
    //Serial.println("Failed to acquire GPS data");
  }
  
  // Apagar el GPS para ahorrar energía
  digitalWrite(gpsControlPin, LOW);
  //Serial.println("Apago GPS");
  //Serial.println("Entrando en modo sleep");
  
  delay(2000);  // Pausa antes de entrar en modo de sueño

  // Entrar en modo de sueño por 5 minutos, utilizando el temporizador Watchdog
  for (int i = 0; i < 225; i++) {  // 225 ciclos de ~8 segundos = ~30 minutos
    f_wdt = 0;
    setup_watchdog(9);  // Configurar el temporizador Watchdog para aproximadamente 8 segundos
    while (f_wdt == 0) {
      system_sleep();  // Poner el sistema en modo de sueño
    }
  }

  // Deshabilitar el temporizador Watchdog después del ciclo de sueño
  wdt_disable();
  delay(2000);  // Pausa después del ciclo de sueño
}

// Función para poner el sistema en modo de sueño
void system_sleep() {
  ADCSRA &= ~(1 << ADEN);  // Apagar el convertidor analógico-digital (ADC) para ahorrar energía
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Configurar el modo de sueño profundo
  sleep_enable();  // Habilitar el modo de sueño
  sleep_mode();  // Entrar en el modo de sueño
  sleep_disable();  // Deshabilitar el modo de sueño al despertar
  ADCSRA |= (1 << ADEN);  // Rehabilitar el ADC al despertar
}

// Configurar el temporizador Watchdog para controlar el tiempo de sueño
void setup_watchdog(int i) {
  byte bb;
  int ww;
  if (i > 9) i = 9;  // Asegurar que el intervalo no exceda el máximo
  bb = i & 7;
  if (i > 7) bb |= (1 << 5);  // Configurar el bit correcto para intervalos largos
  bb |= (1 << WDCE);  // Habilitar cambios en el temporizador
  ww = bb;

  MCUSR &= ~(1 << WDRF);  // Resetear la bandera de reinicio
  WDTCSR |= (1 << WDCE) | (1 << WDE);  // Iniciar la secuencia de tiempo
  WDTCSR = bb;  // Establecer el nuevo valor de tiempo del Watchdog
  WDTCSR |= _BV(WDIE);  // Habilitar la interrupción del Watchdog
}
