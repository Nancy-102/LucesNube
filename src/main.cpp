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

// Declaración de funciones para efectos visuales
void rainbowTask();         // Efecto arcoíris rotativo
void colorWipeTask();       // Efecto de barrido de color
void meteorTask();          // Efecto de "meteorito" desplazándose
void breatheTask();         // Efecto de respiración (fade in/out)
void twinkleTask();         // Efecto de destellos aleatorios
void blinkTask();           // Efecto intermitente ámbar (I)
void reverseTask();         // Efecto de reversa - blanco (B)
void directionLeftTask();   // Efecto direccional izquierda (L)
void directionRightTask();  // Efecto direccional derecha (R)
void stopTask();            // Efecto de alto - rojo fijo (S)

// Funciones para cambiar entre efectos
void switchToNextEffect();            // Cambiar al siguiente efecto RGB
void switchToBlinkEffect();           // Activar efecto intermitente
void switchToReverseEffect();         // Activar efecto reversa
void switchToDirectionLeftEffect();   // Activar efecto izquierda
void switchToDirectionRightEffect();  // Activar efecto derecha
void switchToStopEffect();            // Activar efecto alto

// Crear el planificador de tareas y definir cada tarea
Scheduler scheduler;        // Objeto principal que maneja todas las tareas

// Definición de tareas con (intervalo en ms, repeticiones, función callback)
Task tRainbow(5000, TASK_FOREVER, &rainbowTask);       // Tarea para efecto arcoíris
Task tColorWipe(5000, TASK_FOREVER, &colorWipeTask);   // Tarea para barrido de color
Task tMeteor(5000, TASK_FOREVER, &meteorTask);         // Tarea para efecto meteorito
Task tBreathe(5000, TASK_FOREVER, &breatheTask);       // Tarea para efecto respiración
Task tTwinkle(5000, TASK_FOREVER, &twinkleTask);       // Tarea para efecto destello
Task tBlink(500, TASK_FOREVER, &blinkTask);            // Tarea para intermitente (2Hz)
Task tReverse(500, TASK_FOREVER, &reverseTask);        // Tarea para reversa (2Hz)
Task tDirectionLeft(100, TASK_FOREVER, &directionLeftTask);    // Tarea para dirección izquierda (10Hz)
Task tDirectionRight(100, TASK_FOREVER, &directionRightTask);  // Tarea para dirección derecha (10Hz)
Task tStop(1000, TASK_FOREVER, &stopTask);             // Tarea para efecto alto (1Hz)

// Variables para controlar las animaciones
uint8_t gHue = 0;            // Tono base para efectos de color (0-255)
uint8_t currentEffect = 0;   // Índice del efecto RGB actual (0-4)
uint8_t currentPalette = 0;  // Índice de paleta de colores (no usado activamente)
uint8_t meteorPosition = 0;  // Posición actual del meteorito
uint8_t wipePosition = 0;    // Posición actual para el efecto barrido
uint8_t wipeColor = 0;       // Color actual para el efecto barrido
bool wipeDirection = true;   // Dirección del barrido (true=avanzando, false=retrocediendo)
uint8_t breatheBrightness = 0; // Brillo actual para efecto respiración
bool breatheIncreasing = true; // Estado de brillo (incrementando/decrementando)

// Variables de estado para modos especiales de señalización
bool inBlinkMode = false;    // Modo intermitente ámbar (I) activo
bool inReverseMode = false;  // Modo reversa blanco (B) activo
bool inLeftMode = false;     // Modo direccional izquierda (L) activo
bool inRightMode = false;    // Modo direccional derecha (R) activo
bool inStopMode = false;     // Modo alto rojo (S) activo

// Variables para control de posición en efectos direccionales
int leftPosition = NUM_LEDS - 1;  // Posición inicial para animación hacia izquierda
int rightPosition = 0;            // Posición inicial para animación hacia derecha

// Control de comandos
char lastCommand = ' ';      // Almacena el último comando para toggle (encender/apagar)

// Función de inicialización
void setup() {
  Serial.begin(115200);      // Iniciar comunicación serial a 115200 baudios
  delay(1000);               // Esperar 1 segundo por estabilidad
  
  // Configurar FastLED con los parámetros definidos
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);  // Establecer brillo global
  FastLED.clear();                     // Limpiar/apagar todos los LEDs
  FastLED.show();                      // Actualizar físicamente los LEDs
  
  // Inicializar el planificador
  scheduler.init();
  
  // Agregar y activar sólo la primera animación (arcoíris)
  scheduler.addTask(tRainbow);
  tRainbow.enable();
  
  // Mostrar instrucciones en el monitor serial
  Serial.println("Sistema inicializado - Comandos disponibles:");
  Serial.println("- 'B': Reversa (Parpadean los primeros y últimos 15 LEDs en blanco)");
  Serial.println("- 'I': Intermitentes (Parpadean los primeros y últimos 15 LEDs en ámbar)");
  Serial.println("- 'L': Izquierda (Animación direccional izquierda usando dos LEDs)");
  Serial.println("- 'R': Derecha (Animación direccional derecha usando dos LEDs)");
  Serial.println("- 'S': Alto (Enciende los primeros y últimos 15 LEDs en rojo)");
  Serial.println("Enviar el mismo comando para desactivar y volver a modo RGB");
}

// Función principal de ejecución continua
void loop() {
  // Verificar si hay datos disponibles en el puerto serial
  if (Serial.available() > 0) {
    char command = Serial.read();  // Leer un byte del puerto serial
    
    // Si el comando es igual al último recibido, desactivar modo y volver a RGB
    if (command == lastCommand) {
      // Resetear todos los modos especiales
      inBlinkMode = false;
      inReverseMode = false;
      inLeftMode = false;
      inRightMode = false;
      inStopMode = false;
      lastCommand = ' ';  // Limpiar el último comando
      
      // Volver a modo de animaciones RGB normales
      switchToNextEffect();
    } else {
      // Procesamiento de nuevo comando
      switch (command) {
        case 'B': case 'b':  // Reversa (blanco)
          switchToReverseEffect();
          lastCommand = 'B';
          break;
          
        case 'I': case 'i':  // Intermitente (ámbar)
          switchToBlinkEffect();
          lastCommand = 'I';
          break;
          
        case 'L': case 'l':  // Dirección izquierda
          switchToDirectionLeftEffect();
          lastCommand = 'L';
          break;
          
        case 'R': case 'r':  // Dirección derecha
          switchToDirectionRightEffect();
          lastCommand = 'R';
          break;
          
        case 'S': case 's':  // Alto (rojo)
          switchToStopEffect();
          lastCommand = 'S';
          break;
      }
    }
  }
  
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
  tBlink.disable();
  tReverse.disable();
  tDirectionLeft.disable();
  tDirectionRight.disable();
  tStop.disable();
  
  // Apagar todos los LEDs
  FastLED.clear();
}

// Función para activar el efecto de reversa (B)
void switchToReverseEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Activar bandera de modo reversa
  inReverseMode = true;
  
  // Habilitar la tarea específica
  scheduler.addTask(tReverse);
  tReverse.enable();
  
  Serial.println("Efecto: Reversa (blanco)");
}

// Función para activar el efecto intermitente (I)
void switchToBlinkEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Activar bandera de modo intermitente
  inBlinkMode = true;
  
  // Habilitar la tarea específica
  scheduler.addTask(tBlink);
  tBlink.enable();
  
  Serial.println("Efecto: Intermitente (ámbar)");
}

// Función para activar el efecto direccional izquierda (L)
void switchToDirectionLeftEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Activar bandera de modo izquierda
  inLeftMode = true;
  leftPosition = NUM_LEDS - 1;  // Reiniciar posición desde el final
  
  // Habilitar la tarea específica
  scheduler.addTask(tDirectionLeft);
  tDirectionLeft.enable();
  
  Serial.println("Efecto: Dirección izquierda");
}

// Función para activar el efecto direccional derecha (R)
void switchToDirectionRightEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Activar bandera de modo derecha
  inRightMode = true;
  rightPosition = 0;       // Reiniciar posición desde el inicio
  
  // Habilitar la tarea específica
  scheduler.addTask(tDirectionRight);
  tDirectionRight.enable();
  
  Serial.println("Efecto: Dirección derecha");
}

// Función para activar el efecto de alto (S)
void switchToStopEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Activar bandera de modo alto
  inStopMode = true;
  
  // Habilitar la tarea específica
  scheduler.addTask(tStop);
  tStop.enable();
  
  Serial.println("Efecto: Alto (rojo)");
}

// Función para cambiar al siguiente efecto RGB
void switchToNextEffect() {
  disableAllTasks();       // Primero desactivar todas las tareas
  
  // Calcular siguiente efecto (0-4) con módulo para ciclar
  currentEffect = (currentEffect + 1) % 5;
  
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

// Implementación del efecto intermitente ámbar (I)
void blinkTask() {
  static bool blinkState = false;  // Estado actual (encendido/apagado)
  
  // Invertir el estado en cada llamada (encendido->apagado o apagado->encendido)
  blinkState = !blinkState;
  
  if (blinkState) {  // Si está en estado encendido
    // Encender los primeros y últimos LEDs en ámbar
    int halfCount = min(15, NUM_LEDS / 2);  // Máximo 15 LEDs o la mitad si es menor
    
    // Definir color ámbar (mezcla de rojo y verde)
    CRGB amberColor = CRGB(255, 191, 0);  // Valores RGB para ámbar
    
    // Encender los primeros LEDs
    for (int i = 0; i < halfCount; i++) {
      leds[i] = amberColor;
    }
    
    // Encender los últimos LEDs
    for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
      leds[i] = amberColor;
    }
  } else {  // Si está en estado apagado
    // Apagar todos los LEDs
    FastLED.clear();
  }
}

// Implementación del efecto reversa blanco (B)
void reverseTask() {
  static bool reverseState = false;  // Estado actual (encendido/apagado)
  
  // Invertir el estado en cada llamada
  reverseState = !reverseState;
  
  if (reverseState) {  // Si está en estado encendido
    // Encender los primeros y últimos LEDs en blanco
    int halfCount = min(15, NUM_LEDS / 2);  // Máximo 15 LEDs o la mitad si es menor
    
    // Encender los primeros LEDs
    for (int i = 0; i < halfCount; i++) {
      leds[i] = CRGB::White;  // Color blanco predefinido
    }
    
    // Encender los últimos LEDs
    for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;  // Color blanco predefinido
    }
  } else {  // Si está en estado apagado
    // Apagar todos los LEDs
    FastLED.clear();
  }
}

// Implementación del efecto dirección izquierda (L)
void directionLeftTask() {
  // Limpiar todos los LEDs para efecto más claro
  FastLED.clear();
  
  // Dibujar dos LEDs desplazándose (verde y azul)
  leds[leftPosition % NUM_LEDS] = CRGB::Green;            // LED principal verde
  leds[(leftPosition - 1 + NUM_LEDS) % NUM_LEDS] = CRGB::Blue;  // LED secundario azul
  
  // Actualizar posición (hacia la izquierda con wrap-around)
  leftPosition = (leftPosition - 1 + NUM_LEDS) % NUM_LEDS;
  
  // Añadir pequeño retardo para suavizar la animación
  delay(50);  // 50ms adicionales (más lento que los 8.33ms del bucle principal)
}

// Implementación del efecto dirección derecha (R)
void directionRightTask() {
  // Limpiar todos los LEDs para efecto más claro
  FastLED.clear();
  
  // Dibujar dos LEDs desplazándose (rojo y azul)
  leds[rightPosition % NUM_LEDS] = CRGB::Red;         // LED principal rojo
  leds[(rightPosition + 1) % NUM_LEDS] = CRGB::Blue;  // LED secundario azul
  
  // Actualizar posición (hacia la derecha con wrap-around)
  rightPosition = (rightPosition + 1) % NUM_LEDS;
  
  // Añadir pequeño retardo para suavizar la animación
  delay(50);  // 50ms adicionales
}

// Implementación del efecto alto rojo (S)
void stopTask() {
  // Encender los primeros y últimos LEDs en rojo fijo
  int halfCount = min(15, NUM_LEDS / 2);  // Máximo 15 LEDs o la mitad si es menor
  
  // Limpiar LEDs primero para asegurarse de estado correcto
  FastLED.clear();
  
  // Encender los primeros LEDs en rojo
  for (int i = 0; i < halfCount; i++) {
    leds[i] = CRGB::Red;  // Color rojo predefinido
  }
  
  // Encender los últimos LEDs en rojo
  for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;  // Color rojo predefinido
  }
}