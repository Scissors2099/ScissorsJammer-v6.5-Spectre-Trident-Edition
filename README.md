# ⚔️ ScissorsJammer v6.5-Spectre Trident Edition
> **¡Fuerza bruta de transmisión, precisión de cirujano en recepción y costo de guerrilla maker! 🏆🛰️**

Estación portátil autónoma de contra-inteligencia, auditoría analítica de espectro y defensa de capa física en la banda de 2.4 GHz. Este diseño optimiza el uso de hardware de bajo costo mediante una arquitectura asíncrona de doble núcleo (**FreeRTOS**), aislando las colisiones de hardware entre chips y logrando un despliegue táctico militar que le mete un "tijeretazo" electrónico instantáneo a las frecuencias.

---

## 📺 Características Principales

### 🛰️ 1. ANALIZADOR WI-FI PASIVO (Suite de Inteligencia Lúdica)
Operado de forma 100% pasiva a través de la antena interna del ESP32 sin emitir RF:
* **[Sub 0] Ruido Crudo RF:** Gráfica original tipo Matrix con decaimiento lineal dinámico barriendo 125 canales a través de la Radio B secundaria.

### 🎯 2. RADAR QUIRÚRGICO (Copia Fiel Original)
Buscador de objetivos lógicos Wi-Fi que rastrea, fija y bloquea un SSID específico en el aire. Cuenta con indicador dinámico en vivo de objetivos (`Vistas: X / Y`) y blindaje de hilos en el Core 1 para impedir escapes fantasmas al menú principal.

### 🔥 3. ATAQUE TRIPLE CAÑÓN TRANSMISOR
El soplete electromagnético definitivo. Activa el **Core 0** a 240 MHz constantes batiendo ciclos de CPU nativos en ensamblador y despliega dos radios NRF batiendo freq en espejo mediante buses SPI cruzados (`VSPI` / `HSPI`) combinados con un muro de concreto digital de 20 MHz. 

---

## 🔌 Conexión de Hardware Indestructible

El diseño utiliza una solución de **Sincronismo Electromecánico Invertido** mediante un interruptor industrial de doble cámara independiente:
1. **Lado NO (Lógica):** Conecta el pin `GPIO 16` directo a masa (`GND`) usando la resistencia `INPUT_PULLUP` nativa del ESP32. ¡Cero componentes externos en la protoboard!
2. **Lado NC (Potencia):** Corta físicamente la línea viva de `+3.3V` hacia las radios, disipando la estática, vaciando capacitores y enfriando el silicio en caliente.

### 📌 Tabla de Pines (Pinout Consecutivo Protegido)

| Componente | Pin Radio | Pin ESP32 | Bus de Datos |
| :--- | :--- | :--- | :--- |
| **Radio A (Ataque)** | CE | GPIO 4 | VSPI |
| | CSN | GPIO 5 | VSPI |
| | SCK / MISO / MOSI | GPIO 18 / 19 / 23 | VSPI (Nativo de Fábrica) |
| **Radio B (Espectro)** | CE | GPIO 27 | HSPI |
| | CSN | GPIO 26 | HSPI (**Búnker Libre de Wi-Fi**) |
| | SCK / MISO / MOSI | GPIO 14 / 12 / 13 | HSPI (Conexión Consecutiva Nativa) |
| **Switch Industrial** | Cámara NO | GPIO 16 | Entrada Pull-up Directa a Masa |
| **Pantalla OLED 0.96"**| SCL / SDA | GPIO 22 / 21 | Bus I2C Estándar |

---

## 🛠️ Telemetría y Salud de Silicio
El sistema lee el sensor térmico interno del procesador y estampa la lectura fija en la esquina del display OLED (**`53C`** estables en reposo). Si el chip junta fiebre por el asedio pesado del Triple Cañón, el firmware activa de forma automática una alerta intermitente que grita **`!HOT!`** en pantalla para preservar los transistores.

## 📐 Chasis Portátil
Diseñado para calzar de forma plana, simétrica y con cunas de canto vertical dentro de un gabinete ergonómico tipo gamepad impreso en PLA/PETG por la **Artillery Genius**. Energizado de forma autónoma con una celda de litio 18650 de alta descarga.

---
*Desarrollado con orgullo por la comunidad Maker y de Software Libre.* 🏆🛰️💥 --- stl pronto en thingsverse --- desarrollo


## 📸 Bitácora Visual del Laboratorio

### 🛠️ Estado del Cableado en Protoboard
![Estructura del Cableado](CABLEADO.jpeg)

### 🔋 Placas Calzadas en Postes de Chasis
![Montaje de Placas](https://github.com/saldarriagalau-ux/ScissorsJammer-v6.5-Spectre-Trident-Edition/blob/main/PLACASENBOX.jpeg)

### ⚡ Primer Encendido y Verificación Lógica
![Equipo Funcionando](https://github.com/saldarriagalau-ux/ScissorsJammer-v6.5-Spectre-Trident-Edition/blob/main/FUNCIONANDO.jpeg)

### 📐 Ensamble Mecánico Final
![Gabinete Terminado](https://github.com/saldarriagalau-ux/ScissorsJammer-v6.5-Spectre-Trident-Edition/blob/main/ARMADOFINAL.jpeg)




