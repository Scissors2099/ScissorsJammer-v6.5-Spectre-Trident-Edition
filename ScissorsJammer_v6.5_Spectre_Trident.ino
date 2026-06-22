#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>
#include <ezButton.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_wifi.h> // 🚀 LIBRERÍA NATIVA: Permite controlar la potencia de transmisión del Wi-Fi
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 📡 ASIGNACIÓN DE PINES DE HARDWARE (MULTI-BUS SPI NATIVO ESP32)
#define RA_CE   4
#define RA_CSN  5
#define RB_CE   27
#define RB_CSN  15
#define BTN_PIN 16

RF24 radioA(RA_CE, RA_CSN);
RF24 radioB(RB_CE, RB_CSN);
ezButton button(BTN_PIN);

SPIClass *vspi = NULL;
SPIClass *hspi = NULL;
BLEAdvertising *pAdvertising = NULL; 

// 🎯 TRIDENTE DE EMISIÓN ASÍNCRONA CRUZADA EXTERNA
const int canalesAmetralladoraA[] = {2, 26};  
const int canalesAmetralladoraB[] = {38, 79}; 

const char* modes[] = {
  "ESCANER RUIDO  ",  
  "RADAR: QUIRURG ",  
  "ATAQUE: TRIPLE C"   // ◄ ¡EVOLUCIÓN TÁCTICA A TRIPLE CAÑÓN!
};

uint8_t opcion_visualizada = 0; 
bool accion_activa = false;      

int numRedes = 0;
int redSeleccionada = 0;
bool radarAtacando = false;
int canalObjetivo = 0;
String nombreObjetivo = "";
String nombreObjetivoCompleto = "";
bool necesitaEscanear = true;
// 🚀 EL FIX DE COMPILACIÓN: Declaramos los temporizadores para el Modo 1 Radar
unsigned long ultimoRastreoTime = 0; 
const unsigned long INTERVALO_RASTREO = 5000;
unsigned long tiempoPresionado = 0;
const unsigned long TIEMPO_LARGO = 800;
bool botonPresionadoAntes = false;
unsigned long ultimoToqueSoltado = 0;
bool escaneando_ahora = false;

int canales_ruido[125]; 

TaskHandle_t TareaCore0;

void setup() {
  Serial.begin(115200);
  button.setDebounceTime(40);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  Wire.begin(21, 22); 
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay(); display.display();
  }
  
  // Nace en WIFI_OFF para blindar la memoria RAM y dejar los buses SPI limpios
  WiFi.mode(WIFI_OFF); delay(100);
  BLEDevice::init("CesarProJammer");

  vspi = new SPIClass(VSPI); hspi = new SPIClass(HSPI);
  vspi->begin(18, 19, 23, 5); hspi->begin(14, 12, 13, 15); 

  if (radioA.begin(vspi)) {
    radioA.setAutoAck(false); radioA.setRetries(0, 0); radioA.setPayloadSize(5);
    radioA.setAddressWidth(3); radioA.setPALevel(RF24_PA_MAX); radioA.setDataRate(RF24_250KBPS);
    radioA.setCRCLength(RF24_CRC_DISABLED); radioA.stopListening();
  }

  if (radioB.begin(hspi)) {
    radioB.setAutoAck(false); radioB.setRetries(0, 0); radioB.setPayloadSize(5);
    radioB.setAddressWidth(3); radioB.setPALevel(RF24_PA_MAX); radioB.setDataRate(RF24_250KBPS);
    radioB.setCRCLength(RF24_CRC_DISABLED); radioB.stopListening();
  }

  xTaskCreatePinnedToCore(loopCore0, "TareaRadioA", 10000, NULL, 1, &TareaCore0, 0);
  Serial.println("[SYSTEM] CesarProJammer v2 TRIPLE CAÑÓN Online.");
  actualizarPantallaOLED();
}
// ========================================================
// 🧠 🔄 NÚCLEO 0 (CORE 0): METRALLETA DE CONTROL INTERNO (VSPI)
// ========================================================
void loopCore0( void * pvParameters ) {
  for(;;) {
    if (accion_activa && opcion_visualizada == 2) {
      for (int i = 0; i < 2; i++) {
        radioA.setChannel(canalesAmetralladoraA[i]);
        radioA.startConstCarrier(RF24_PA_MAX, canalesAmetralladoraA[i]);
        uint32_t ciclos_A = ESP.getCycleCount();
        while ((ESP.getCycleCount() - ciclos_A) < 28000) { uint8_t d = 0; }
      }
    } else {
      radioA.stopListening();
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); 
  }
}

// ========================================================
// 🧠 HILO PRINCIPAL: NÚCLEO 1 (CORE 1 - void loop unificado v5.0)
// ========================================================
void loop() {
  button.loop();
  int estadoBoton = digitalRead(BTN_PIN);
  
  if (estadoBoton == LOW) {
    if (!botonPresionadoAntes) { tiempoPresionado = millis(); botonPresionadoAntes = true; }
    
    // ⏳ GATILLO PULSACIÓN LARGA SIMÉTRICA (ENTRAR / SALIR DE CUALQUIER MODO)
    if ((millis() - tiempoPresionado > TIEMPO_LARGO) && botonPresionadoAntes) {
      botonPresionadoAntes = false;
      
      // 🚀 LOGICA DE ALTERNANCIA CENTRAL DE ACCIÓN
      if (opcion_visualizada == 0) {
        if (!accion_activa) {
          accion_activa = true;
          Serial.println("[SYSTEM] Core 1: Inicializando Osciloscopio en Caliente...");
          // Inicialización express limpia por bus HSPI
          radioB.begin(hspi); radioB.setAutoAck(false); radioB.setRetries(0, 0);
          radioB.setPayloadSize(5); radioB.setAddressWidth(3); radioB.setPALevel(RF24_PA_MAX);
          radioB.setDataRate(RF24_250KBPS); radioB.setCRCLength(RF24_CRC_DISABLED); radioB.stopListening();
          for(int i = 0; i < 125; i++) canales_ruido[i] = 0;
        } else {
          // SALIDA SIMÉTRICA POR CLIC LARGO
          accion_activa = false;
          radioB.stopListening();
          Serial.println("[SYSTEM] Core 1: Osciloscopio en Standby.");
        }
        actualizarPantallaOLED();
      }
      else if (opcion_visualizada == 2) {
        if (!accion_activa) {
          accion_activa = true;
          Serial.println("[SYSTEM] Core 1: Inicializando Cañón Wi-Fi AP Canal 7...");
          WiFi.mode(WIFI_AP); delay(50);
          WiFi.softAP("WALL_AFH", "12345678", 7, 0, 4); 
          esp_wifi_set_max_tx_power(78); delay(50);
        } else {
          // SALIDA SIMÉTRICA POR CLIC LARGO (Apagado absoluto del Muro)
          accion_activa = false;
          radioB.stopListening();
          WiFi.mode(WIFI_OFF); 
          Serial.println("[SYSTEM] Core 1: Cañón Canal 7 Desactivado de forma segura.");
        }
        actualizarPantallaOLED();
      }
      else if (opcion_visualizada == 1) {
        if (!accion_activa) { accion_activa = true; necesitaEscanear = true; radarAtacando = false; }
        else {
          if (!radarAtacando && numRedes > 0) {
            radarAtacando = true; canalObjetivo = WiFi.channel(redSeleccionada);
            nombreObjetivoCompleto = WiFi.SSID(redSeleccionada); nombreObjetivo = nombreObjetivoCompleto;
            if (nombreObjetivo.length() > 14) nombreObjetivo = nombreObjetivo.substring(0, 12) + "..";
            ultimoRastreoTime = millis();
          } else {
            // SALIDA SIMÉTRICA EN MODO RADAR
            accion_activa = false; radarAtacando = false; radioB.stopListening();
            Serial.println("[SYSTEM] Core 1: Radar en Standby.");
          }
        }
        actualizarPantallaOLED();
      }
      delay(400); // Candado de tiempo para evitar rebotes al soltar el botón largo
    }
  } else {
    if (botonPresionadoAntes) {
      unsigned long duracion = millis() - tiempoPresionado;
      botonPresionadoAntes = false;
      
      if (duracion < TIEMPO_LARGO) {
        // ➔ GATILLO PULSACIÓN CORTA ORDINARIA (NAVEGACIÓN PURA CON EMISIÓN APAGADA)
        if (opcion_visualizada == 1 && accion_activa && !radarAtacando) {
          if (numRedes > 0) { redSeleccionada = (redSeleccionada + 1) % numRedes; actualizarPantallaOLED(); }
        } else if (!accion_activa) {
          opcion_visualizada = (opcion_visualizada + 1) % 3; 
          radioB.stopListening();
          actualizarPantallaOLED();
        }
      }
    }
  }

  // --- 🛰️ TRACKING DINÁMICO COORDINADO EN RADIO B (CORE 1) ---
  bool escaneando_ahora = false;
  if (opcion_visualizada == 1 && accion_activa && radarAtacando && (millis() - ultimoRastreoTime > INTERVALO_RASTREO)) {
    ultimoRastreoTime = millis(); escaneando_ahora = true; radioB.stopListening(); delay(5);
    int redesEncontradas = WiFi.scanNetworks(false, true);
    for (int i = 0; i < redesEncontradas; i++) {
      if (WiFi.SSID(i) == nombreObjetivoCompleto) {
        int nuevoCanal = WiFi.channel(i);
        if (nuevoCanal != canalObjetivo) { canalObjetivo = nuevoCanal; actualizarPantallaOLED(); }
        break;
      }
    }
    escaneando_ahora = false;
  }

  // --- 📡 COMPORTAMIENTO HARDWARE SEGÚN ACCIÓN ACTIVA ---
  if (accion_activa && !escaneando_ahora) {
    if (opcion_visualizada == 2) {
      for (int i = 0; i < 2; i++) {
        radioB.setChannel(canalesAmetralladoraB[i]);
        radioB.startConstCarrier(RF24_PA_MAX, canalesAmetralladoraB[i]);
        uint32_t ciclos_B = ESP.getCycleCount();
        while ((ESP.getCycleCount() - ciclos_B) < 28000) { yield(); }
      }
    }
    else if (opcion_visualizada == 1 && radarAtacando) {
      int canalRadioNRF = (2412 + (canalObjetivo - 1) * 5) - 2400;
      radioB.setChannel(canalRadioNRF); radioB.startConstCarrier(RF24_PA_MAX, canalRadioNRF);
    }
    else if (opcion_visualizada == 0) {
      // 🚀 BARRIDO OSCILOSCOPIO SÍNCRONO: Corre continuo de forma fluida
      radioB.powerUp();
      for (int i = 0; i < 125; i++) {
        radioB.stopListening(); radioB.setChannel(i); radioB.startListening(); 
        delayMicroseconds(130); 
        if (radioB.testRPD()) {
          canales_ruido[i] = 20; 
        } else {
          if (canales_ruido[i] > 0) canales_ruido[i] -= 2; 
        }
      }
      radioB.stopListening();
      actualizarPantallaOLED();
      delay(5);
    }
  } 
  else {
    // Si no está la acción activa, refresco estático para ver el menú en reposo
    static unsigned long ultimoRefrescoEstatico = 0;
    if (millis() - ultimoRefrescoEstatico > 100) {
      ultimoRefrescoEstatico = millis();
      actualizarPantallaOLED();
    }
  }
} // Fin del void loop() del Core 1
// ========================================================
// 📺 MOTOR GRÁFICO UNIFICADO OLED
// ========================================================
void actualizarPantallaOLED() {
  if (opcion_visualizada == 1 && accion_activa && necesitaEscanear && !radarAtacando) {
    display.clearDisplay(); display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextSize(1); display.setCursor(15, 25); display.print("ESCANEANDO AIRE..."); display.display();
    WiFi.mode(WIFI_STA); delay(50);
    numRedes = WiFi.scanNetworks(); redSeleccionada = 0; necesitaEscanear = false;
  }

  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  // 🌡️ LEEMOS EL SENSOR DE SILICIO NATIVO DEL ESP32
float temp_celsius = temperatureRead(); 

// Configuramos el texto en la esquina superior derecha
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(90, 0); 

// Si el chip junta mucha fiebre por el ataque pesado, parpadea una alerta visual
if (temp_celsius > 65.0 && (millis() / 300) % 2 == 0) {
  display.print("¡HOT!"); 
} else {
  display.print(String((int)temp_celsius) + "C");
}

  display.drawFastHLine(0, 16, 128, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 5); display.print(modes[opcion_visualizada]);
  
  if (opcion_visualizada == 0 && accion_activa) {
    // 📊 MATRIX EN VIVO ACTIVA (Osciloscopio infinito)
    for (int i = 0; i < 125; i++) {
      if (canales_ruido[i] > 0) {
        int altura_pixel = map(canales_ruido[i], 0, 20, 0, 46);
        if (altura_pixel > 46) altura_pixel = 46;
        if (altura_pixel < 0)  altura_pixel = 0;
        display.drawFastVLine(i + 2, 62 - altura_pixel, altura_pixel, SSD1306_WHITE);
      }
    }
    display.drawFastHLine(2, 62, 124, SSD1306_WHITE);
  }
  else if (!accion_activa) {
    // Menú en reposo absoluto, esperando tu clic sostenido táctico para inicializar
    display.setCursor(10, 26); display.print("Opcion de Menu");
    display.setCursor(10, 44); display.print("[Manten presionado]");
  } else {
    display.setCursor(10, 26);
    switch (opcion_visualizada) {
      case 1:
        if (radarAtacando) {
          display.print("TRACKING ACTIVO!"); display.setCursor(10, 38);
          display.print("SSID: " + nombreObjetivo); display.setCursor(10, 48);
          display.print("Canal Actual: " + String(canalObjetivo));
        } else {
          if (numRedes == 0) { display.print("Sin redes halladas"); } 
          else {
            String ssidActual = WiFi.SSID(redSeleccionada);
            if (ssidActual.length() > 14) ssidActual = ssidActual.substring(0, 12) + "..";
            display.print("SSID: " + ssidActual); display.setCursor(10, 38);
            display.print("Canal Router: " + String(WiFi.channel(redSeleccionada))); display.setCursor(10, 48);
            display.print("Red: " + String(redSeleccionada + 1) + " / " + String(numRedes));
          }
        }
        break;
      case 2: // 🚀 NUEVO MARCO DE PANEL: TRIPLE CAÑÓN CON MURO ELECTROMAGNÉTICO WIFi
        display.print("TRIPLE CANON ON");
        display.setCursor(10, 36); display.print("MURO CH 7 (2442MHz)");
        display.setCursor(10, 48); display.print("M: 2,26,38,79 + BLE");
        break;
    }
  }
  display.display();
}
