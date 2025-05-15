// Inclusión de bibliotecas necesarias 
#include <Arduino.h>        // Biblioteca principal para funciones básicas de Arduino
#include <FastLED.h>        // Biblioteca para controlar tiras LED RGB/RGBW
#include <TaskScheduler.h>  // Biblioteca para programar y ejecutar tareas periódicas

// Configuración de la tira LED
#define LED_PIN     D4      // Pin donde está conectada la tira LED (D4 en ESP8266)
#define NUM_LEDS    60      // Número total de LEDs en la tira
#define LED_TYPE    WS2812B // Tipo específico de LED (WS2812B = NeoPixel)
#define COLOR_ORDER GRB     // Orden de bytes de color (Green-Red-Blue)
#define BRIGHTNESS  150     // Brillo global (0-255, donde 255 es máximo)

// Arreglo que almacena el estado de cada LED
CRGB leds[NUM_LEDS];        // CRGB es una estructura que contiene valores RGB

// Declaración de funciones para efectos visuales RGB
void rainbowTask();         // Efecto arcoíris rotativo
void colorWipeTask();       // Efecto de barrido de color
void meteorTask();          // Efecto de "meteorito" desplazándose
void breatheTask();         // Efecto de respiración (fade in/out)
void twinkleTask();         // Efecto de destellos aleatorios
void fireTask();            // Efecto de fuego simulado

// Función para cambiar entre efectos
void switchToNextEffect();  // Cambiar al siguiente efecto RGB

// Crear el planificador de tareas y definir cada tarea
Scheduler scheduler;        // Objeto principal que maneja todas las tareas

// Definición de tareas con (intervalo en ms, repeticiones, función callback)
Task tRainbow(5000, TASK_FOREVER, &rainbowTask);       // Tarea para efecto arcoíris
Task tColorWipe(5000, TASK_FOREVER, &colorWipeTask);   // Tarea para barrido de color
Task tMeteor(5000, TASK_FOREVER, &meteorTask);         // Tarea para efecto meteorito
Task tBreathe(5000, TASK_FOREVER, &breatheTask);       // Tarea para efecto respiración
Task tTwinkle(5000, TASK_FOREVER, &twinkleTask);       // Tarea para efecto destello
Task tFire(5000, TASK_FOREVER, &fireTask);             // Tarea para efecto fuego

// Variables para controlar las animaciones
uint8_t gHue = 0;            // Tono base para efectos de color (0-255)
uint8_t currentEffect = 0;   // Índice del efecto RGB actual (0-5)
uint8_t meteorPosition = 0;  // Posición actual del meteorito
uint8_t wipePosition = 0;    // Posición actual para el efecto barrido
uint8_t wipeColor = 0;       // Color actual para el efecto barrido
bool wipeDirection = true;   // Dirección del barrido (true=avanzando, false=retrocediendo)
uint8_t breatheBrightness = 0; // Brillo actual para efecto respiración
bool breatheIncreasing = true; // Estado de brillo (incrementando/decrementando)

// Variables para el efecto fuego
uint8_t cooling = 55;        // Valor de enfriamiento para el efecto fuego
uint8_t sparking = 120;      // Probabilidad de chispas para el efecto fuego
uint8_t fireHeight = 30;     // Altura máxima de las llamas
bool firePalette = true;     // Si es true usa paleta personalizada, si es false usa HeatColor()

// Función de inicialización
void setup() {
  Serial.begin(115200);      // Iniciar comunicación serial para debugging
  delay(1000);               // Esperar 1 segundo por estabilidad
  
  // Configurar FastLED con los parámetros definidos
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);  // Establecer brillo global
  FastLED.clear();                     // Limpiar/apagar todos los LEDs
  FastLED.show();                      // Actualizar físicamente los LEDs
  
  // Inicializar el planificador
  scheduler.init();
  
  // Comenzar con la primera animación (arcoíris)
  scheduler.addTask(tRainbow);
  tRainbow.enable();
  
  Serial.println("Sistema de animaciones RGB inicializado");
  Serial.println("Efectos incluidos: Arcoíris, Barrido de color, Meteorito, Respiración, Destello, Fuego");
}

// Función principal de ejecución continua
void loop() {
  scheduler.execute();         // Ejecutar tareas programadas
  FastLED.show();              // Actualizar físicamente los LEDs
  FastLED.delay(1000 / 120);   // Limitar a 120 FPS (8.33ms entre frames)
}

// Función para deshabilitar todas las tareas activas
void disableAllTasks() {
  // Deshabilitar cada tarea individualmente
  tRainbow.disable();
  tColorWipe.disable();
  tMeteor.disable();
  tBreathe.disable();
  tTwinkle.disable();
  tFire.disable();
  
  // Apagar todos los LEDs
  FastLED.clear();
}

// Función para cambiar al siguiente efecto RGB
void switchToNextEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Calcular siguiente efecto (0-5) con módulo para ciclar
  currentEffect = (currentEffect + 1) % 6;
  
  // Activar el nuevo efecto según el índice
  switch (currentEffect) {
    case 0:  // Arcoíris
      scheduler.addTask(tRainbow);
      tRainbow.enable();
      Serial.println("Efecto: Arcoíris");
      break;
    case 1:  // Barrido de color
      scheduler.addTask(tColorWipe);
      wipePosition = 0;                // Reiniciar posición
      wipeColor = random8();           // Color aleatorio
      tColorWipe.enable();
      Serial.println("Efecto: Barrido de color");
      break;
    case 2:  // Meteorito
      scheduler.addTask(tMeteor);
      meteorPosition = 0;              // Reiniciar posición
      tMeteor.enable();
      Serial.println("Efecto: Meteorito");
      break;
    case 3:  // Respiración
      scheduler.addTask(tBreathe);
      breatheBrightness = 0;           // Comenzar desde oscuro
      breatheIncreasing = true;        // Iniciar aumentando brillo
      tBreathe.enable();
      Serial.println("Efecto: Respiración");
      break;
    case 4:  // Destello
      scheduler.addTask(tTwinkle);
      tTwinkle.enable();
      Serial.println("Efecto: Destello");
      break;
    case 5:  // Fuego
      scheduler.addTask(tFire);
      tFire.enable();
      Serial.println("Efecto: Fuego");
      break;
  }
}

// Implementación del efecto arcoíris
void rainbowTask() {
  static int counter = 0;    // Contador para duración del efecto
  
  // Crear efecto arcoíris en toda la tira
  fill_rainbow(leds, NUM_LEDS, gHue, 7);  // 7 es el desplazamiento de color entre LEDs
  
  // Actualizar color base periódicamente
  EVERY_N_MILLISECONDS(20) {  // Cada 20ms (50Hz)
    gHue++;  // Incrementar tono base (0-255, vuelve a 0 automáticamente)
  }
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos (250 * 8.33ms = ~2080ms)
    counter = 0;
    switchToNextEffect();  // Cambiar al siguiente efecto
  }
}

// Implementación del efecto barrido de color
void colorWipeTask() {
  static int counter = 0;    // Contador para duración y ralentización
  
  // Realizar el barrido de color (más lento)
  if (counter % 5 == 0) {    // Cada 5 ciclos para ralentizar
    leds[wipePosition] = CHSV(wipeColor, 255, 255);  // Asignar color HSV al LED actual
    
    wipePosition++;          // Avanzar al siguiente LED
    if (wipePosition >= NUM_LEDS) {  // Al llegar al final
      wipePosition = 0;      // Volver al inicio
      wipeColor += 30;       // Cambiar color para el siguiente barrido
    }
  }
  
  // Desvanecer gradualmente el resto de LEDs
  fadeToBlackBy(leds, NUM_LEDS, 10);  // Reducir brillo un 10% por ciclo
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Implementación del efecto meteorito
void meteorTask() {
  static int counter = 0;    // Contador para duración y ralentización
  
  // Desvanecer todos los LEDs para crear estela
  fadeToBlackBy(leds, NUM_LEDS, 64);  // Reducir brillo un 25% por ciclo
  
  // Dibujar meteorito con cola degradada
  int meteorSize = 3;        // Tamaño del meteorito (3 LEDs)
  for (int i = 0; i < meteorSize; i++) {
    // Verificar que la posición esté dentro del rango válido
    if ((meteorPosition - i < NUM_LEDS) && (meteorPosition - i >= 0)) {
      // Color más brillante al frente, más tenue en la cola
      leds[meteorPosition - i] = CHSV(gHue, 255, 255 - (i * 50));
    }
  }
  
  // Actualizar posición (más lento)
  if (counter % 3 == 0) {    // Cada 3 ciclos para ralentizar
    meteorPosition++;        // Avanzar meteorito
    if (meteorPosition > NUM_LEDS + meteorSize) {  // Cuando sale completamente
      meteorPosition = 0;    // Reiniciar desde el inicio
      gHue += 32;            // Cambiar color del siguiente meteorito
    }
  }
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Implementación del efecto respiración
void breatheTask() {
  static int counter = 0;    // Contador para duración y ralentización
  
  // Efecto de respiración (fade in/out)
  if (counter % 2 == 0) {    // Cada 2 ciclos para ralentizar
    if (breatheIncreasing) {
      breatheBrightness += 1;  // Aumentar brillo
      if (breatheBrightness >= 250) {  // Al llegar al máximo
        breatheIncreasing = false;     // Comenzar a disminuir
      }
    } else {
      breatheBrightness -= 1;  // Disminuir brillo
      if (breatheBrightness <= 10) {   // Al llegar al mínimo
        breatheIncreasing = true;      // Comenzar a aumentar
        gHue += 15;                    // Cambiar color cuando está oscuro
      }
    }
  }
  
  // Aplicar mismo color y brillo a toda la tira
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, breatheBrightness));
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Implementación del efecto destello
void twinkleTask() {
  static int counter = 0;    // Contador para duración
  
  // Desvanecer todos los LEDs gradualmente
  fadeToBlackBy(leds, NUM_LEDS, 10);  // Reducir brillo un 10% por ciclo
  
  // Añadir destellos aleatorios
  if (random8() < 50) {  // ~20% de probabilidad por ciclo
    int pos = random16(NUM_LEDS);          // Posición aleatoria
    leds[pos] += CHSV(gHue + random8(64), 200, 255);  // Añadir color brillante
  }
  
  // Cambiar gradualmente el tono base
  EVERY_N_MILLISECONDS(30) {  // Cada 30ms
    gHue++;  // Rotar los colores base
  }
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Implementación del efecto fuego
void fireTask() {
  static int counter = 0;    // Contador para duración
  static byte heat[NUM_LEDS]; // Array para almacenar valores de calor por LED
  
  // Reducir temperatura en cada LED (enfriamiento)
  for (int i = 0; i < NUM_LEDS; i++) {
    // Más cerca del inicio, mayor enfriamiento para que el fuego se eleve
    int cooldown = random8(0, ((cooling * 10) / NUM_LEDS) + 2);
    
    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }
  
  // Propagación del calor hacia arriba
  for (int i = NUM_LEDS - 1; i >= 2; i--) {
    heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
  }
  
  // Generar chispas aleatorias en la base
  if (random8() < sparking) {
    int y = random8(fireHeight);
    heat[y] = min(255, heat[y] + random8(160, 255));
  }
  
  // Convertir valores de calor a colores RGB y mostrar
  for (int i = 0; i < NUM_LEDS; i++) {
    // Escalamos el calor a la posición vertical
    byte scaledHeat = scale8(heat[i], 240);
    
    // Método directo para calcular el color basado en calor
    CRGB color;
    if (firePalette) {
      // Negro para temperatura muy baja
      if (scaledHeat < 40) {
        color = CRGB::Black;
      } 
      // Rojo oscuro para temperatura baja
      else if (scaledHeat < 128) {
        color = CRGB(scaledHeat * 2, 0, 0);
      } 
      // Rojo brillante a naranja para temperatura media
      else if (scaledHeat < 190) {
        color = CRGB(255, (scaledHeat - 128) * 2, 0);
      } 
      // Naranja a amarillo para temperatura alta
      else {
        color = CRGB(255, 255, (scaledHeat - 190) * 4);
      }
    } else {
      // Alternativa: usar paleta de fuego predefinida
      color = HeatColor(scaledHeat);
    }
    
    leds[NUM_LEDS - 1 - i] = color; // Invertimos para que el fuego suba
  }
  
  // Alternar entre métodos cada cierto tiempo para variedad
  if (counter % 60 == 0) {
    firePalette = !firePalette;
  }
  
  // Contar para cambiar a la siguiente animación después de tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}