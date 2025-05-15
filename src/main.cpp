#include <Arduino.h>
#include <FastLED.h>
#include <TaskScheduler.h>

// Configuración de la tira LED
#define LED_PIN     D4      // Pin donde está conectada la tira LED
#define NUM_LEDS    60      // Número de LEDs en la tira
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  150     // Brillo (0-255)

// Arreglo de LEDs
CRGB leds[NUM_LEDS];

// Definición de tareas
void rainbowTask();
void colorWipeTask();
void meteorTask();
void breatheTask();
void twinkleTask();
void blinkTask();          // Intermitente (I) - parpadeo ámbar
void reverseTask();        // Reversa (B) - parpadeo blanco
void directionLeftTask();  // Izquierda (L) - animación direccional izquierda
void directionRightTask(); // Derecha (R) - animación direccional derecha
void stopTask();           // Alto (S) - LEDs rojos fijos
void switchToNextEffect();
void switchToBlinkEffect();
void switchToReverseEffect();
void switchToDirectionLeftEffect();
void switchToDirectionRightEffect();
void switchToStopEffect();

// Crear scheduler y tareas
Scheduler scheduler;
Task tRainbow(5000, TASK_FOREVER, &rainbowTask);
Task tColorWipe(5000, TASK_FOREVER, &colorWipeTask);
Task tMeteor(5000, TASK_FOREVER, &meteorTask);
Task tBreathe(5000, TASK_FOREVER, &breatheTask);
Task tTwinkle(5000, TASK_FOREVER, &twinkleTask);
Task tBlink(500, TASK_FOREVER, &blinkTask);           // Intermitente ámbar
Task tReverse(500, TASK_FOREVER, &reverseTask);       // Reversa blanco
Task tDirectionLeft(100, TASK_FOREVER, &directionLeftTask);   // Izquierda
Task tDirectionRight(100, TASK_FOREVER, &directionRightTask); // Derecha
Task tStop(1000, TASK_FOREVER, &stopTask);            // Alto rojo

// Variables para las animaciones
uint8_t gHue = 0;  // Tono rotativo para varios efectos
uint8_t currentEffect = 0;
uint8_t currentPalette = 0;
uint8_t meteorPosition = 0;
uint8_t wipePosition = 0;
uint8_t wipeColor = 0;
bool wipeDirection = true;
uint8_t breatheBrightness = 0;
bool breatheIncreasing = true;

// Variables de estado para modos especiales
bool inBlinkMode = false;       // I - Intermitente ámbar
bool inReverseMode = false;     // B - Reversa blanco
bool inLeftMode = false;        // L - Izquierda
bool inRightMode = false;       // R - Derecha
bool inStopMode = false;        // S - Alto rojo

// Posiciones para animaciones direccionales
int leftPosition = NUM_LEDS - 1;
int rightPosition = 0;

// Estado del último comando recibido
char lastCommand = ' ';

void setup() {
  Serial.begin(115200);
  delay(1000); // Retardo de seguridad
  
  // Inicializar FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  // Inicializar scheduler y agregar tareas
  scheduler.init();
  
  // Iniciar solo la primera animación
  scheduler.addTask(tRainbow);
  tRainbow.enable();
  
  Serial.println("Sistema inicializado - Comandos disponibles:");
  Serial.println("- 'B': Reversa (Parpadean los primeros y últimos 15 LEDs en blanco)");
  Serial.println("- 'I': Intermitentes (Parpadean los primeros y últimos 15 LEDs en ámbar)");
  Serial.println("- 'L': Izquierda (Animación direccional izquierda usando dos LEDs)");
  Serial.println("- 'R': Derecha (Animación direccional derecha usando dos LEDs)");
  Serial.println("- 'S': Alto (Enciende los primeros y últimos 15 LEDs en rojo)");
  Serial.println("Enviar el mismo comando para desactivar y volver a modo RGB");
}

void loop() {
  // Revisar si hay comandos en el puerto serial
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    // Si recibe el mismo comando que el último, volver al modo normal
    if (command == lastCommand) {
      // Resetear todos los modos
      inBlinkMode = false;
      inReverseMode = false;
      inLeftMode = false;
      inRightMode = false;
      inStopMode = false;
      lastCommand = ' ';
      
      // Volver a animaciones RGB
      switchToNextEffect();
    } else {
      // Procesar nuevo comando
      switch (command) {
        case 'B': case 'b':
          switchToReverseEffect();
          lastCommand = 'B';
          break;
          
        case 'I': case 'i':
          switchToBlinkEffect();
          lastCommand = 'I';
          break;
          
        case 'L': case 'l':
          switchToDirectionLeftEffect();
          lastCommand = 'L';
          break;
          
        case 'R': case 'r':
          switchToDirectionRightEffect();
          lastCommand = 'R';
          break;
          
        case 'S': case 's':
          switchToStopEffect();
          lastCommand = 'S';
          break;
      }
    }
  }
  
  scheduler.execute();
  FastLED.show();
  FastLED.delay(1000 / 120); // 120 FPS
}

// Función para deshabilitar todas las tareas
void disableAllTasks() {
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
  
  // Limpiar LEDs
  FastLED.clear();
}

// Función para cambiar al efecto de reversa (B)
void switchToReverseEffect() {
  disableAllTasks();
  
  // Activar modo reversa
  inReverseMode = true;
  
  // Habilitar tarea de reversa
  scheduler.addTask(tReverse);
  tReverse.enable();
  
  Serial.println("Efecto: Reversa (blanco)");
}

// Función para cambiar al efecto intermitente (I)
void switchToBlinkEffect() {
  disableAllTasks();
  
  // Activar modo intermitente
  inBlinkMode = true;
  
  // Habilitar tarea de parpadeo
  scheduler.addTask(tBlink);
  tBlink.enable();
  
  Serial.println("Efecto: Intermitente (ámbar)");
}

// Función para cambiar al efecto direccional izquierda (L)
void switchToDirectionLeftEffect() {
  disableAllTasks();
  
  // Activar modo izquierda
  inLeftMode = true;
  leftPosition = NUM_LEDS - 1;
  
  // Habilitar tarea de dirección izquierda
  scheduler.addTask(tDirectionLeft);
  tDirectionLeft.enable();
  
  Serial.println("Efecto: Dirección izquierda");
}

// Función para cambiar al efecto direccional derecha (R)
void switchToDirectionRightEffect() {
  disableAllTasks();
  
  // Activar modo derecha
  inRightMode = true;
  rightPosition = 0;
  
  // Habilitar tarea de dirección derecha
  scheduler.addTask(tDirectionRight);
  tDirectionRight.enable();
  
  Serial.println("Efecto: Dirección derecha");
}

// Función para cambiar al efecto de alto (S)
void switchToStopEffect() {
  disableAllTasks();
  
  // Activar modo alto
  inStopMode = true;
  
  // Habilitar tarea de alto
  scheduler.addTask(tStop);
  tStop.enable();
  
  Serial.println("Efecto: Alto (rojo)");
}

// Función para cambiar a la siguiente animación RGB
void switchToNextEffect() {
  disableAllTasks();
  
  // Cambiar a la siguiente animación
  currentEffect = (currentEffect + 1) % 5;
  
  // Habilitar la nueva animación
  switch (currentEffect) {
    case 0:
      scheduler.addTask(tRainbow);
      tRainbow.enable();
      Serial.println("Efecto: Arcoíris");
      break;
    case 1:
      scheduler.addTask(tColorWipe);
      wipePosition = 0;
      wipeColor = random8();
      tColorWipe.enable();
      Serial.println("Efecto: Barrido de color");
      break;
    case 2:
      scheduler.addTask(tMeteor);
      meteorPosition = 0;
      tMeteor.enable();
      Serial.println("Efecto: Meteorito");
      break;
    case 3:
      scheduler.addTask(tBreathe);
      breatheBrightness = 0;
      breatheIncreasing = true;
      tBreathe.enable();
      Serial.println("Efecto: Respiración");
      break;
    case 4:
      scheduler.addTask(tTwinkle);
      tTwinkle.enable();
      Serial.println("Efecto: Destello");
      break;
  }
}

// Efecto de arcoíris
void rainbowTask() {
  static int counter = 0;
  
  // Crear efecto arcoíris
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
  
  // Actualizar color base
  EVERY_N_MILLISECONDS(20) { 
    gHue++; 
  }
  
  // Cambiar a la siguiente animación después de un tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Efecto de barrido de color
void colorWipeTask() {
  static int counter = 0;
  
  // Realizar el barrido de color
  if (counter % 5 == 0) {  // Ralentizar el efecto
    leds[wipePosition] = CHSV(wipeColor, 255, 255);
    
    wipePosition++;
    if (wipePosition >= NUM_LEDS) {
      wipePosition = 0;
      wipeColor += 30;  // Cambiar color para el siguiente barrido
    }
  }
  
  // Desvanecer el resto de los LEDs lentamente
  fadeToBlackBy(leds, NUM_LEDS, 10);
  
  // Cambiar a la siguiente animación después de un tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Efecto de meteorito
void meteorTask() {
  static int counter = 0;
  
  // Desvanecer todos los LEDs
  fadeToBlackBy(leds, NUM_LEDS, 64);
  
  // Dibujar meteorito
  int meteorSize = 3;
  for (int i = 0; i < meteorSize; i++) {
    if ((meteorPosition - i < NUM_LEDS) && (meteorPosition - i >= 0)) {
      leds[meteorPosition - i] = CHSV(gHue, 255, 255 - (i * 50));
    }
  }
  
  // Actualizar posición
  if (counter % 3 == 0) {  // Ralentizar el movimiento
    meteorPosition++;
    if (meteorPosition > NUM_LEDS + meteorSize) {
      meteorPosition = 0;
      gHue += 32;  // Cambiar color del próximo meteorito
    }
  }
  
  // Cambiar a la siguiente animación después de un tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Efecto de respiración
void breatheTask() {
  static int counter = 0;
  
  // Efecto de respiración aplicando brillo pulsante
  if (counter % 2 == 0) {  // Ralentizar el efecto
    if (breatheIncreasing) {
      breatheBrightness += 1;
      if (breatheBrightness >= 250) {
        breatheIncreasing = false;
      }
    } else {
      breatheBrightness -= 1;
      if (breatheBrightness <= 10) {
        breatheIncreasing = true;
        // Cambiar color cuando llega al mínimo
        gHue += 15;
      }
    }
  }
  
  // Aplicar color y brillo a todos los LEDs
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, breatheBrightness));
  
  // Cambiar a la siguiente animación después de un tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Efecto de destello
void twinkleTask() {
  static int counter = 0;
  
  // Desvanecer todos los LEDs gradualmente
  fadeToBlackBy(leds, NUM_LEDS, 10);
  
  // Añadir destellos aleatorios
  if (random8() < 50) {  // Probabilidad de destello
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, 255);
  }
  
  // Cambio gradual del tono base
  EVERY_N_MILLISECONDS(30) {
    gHue++;
  }
  
  // Cambiar a la siguiente animación después de un tiempo
  counter++;
  if (counter >= 250) {  // ~5 segundos
    counter = 0;
    switchToNextEffect();
  }
}

// Efecto de parpadeo intermitente (I - ámbar)
void blinkTask() {
  static bool blinkState = false;
  
  // Invertir el estado (encendido/apagado)
  blinkState = !blinkState;
  
  if (blinkState) {
    // Encender primeros y últimos 15 LEDs (o menos si la tira es más corta) en ámbar
    int halfCount = min(15, NUM_LEDS / 2);
    
    // Color ámbar (mezcla de rojo y verde)
    CRGB amberColor = CRGB(255, 191, 0);
    
    // Encender los primeros LEDs
    for (int i = 0; i < halfCount; i++) {
      leds[i] = amberColor;
    }
    
    // Encender los últimos LEDs
    for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
      leds[i] = amberColor;
    }
  } else {
    // Apagar todos los LEDs
    FastLED.clear();
  }
}

// Efecto de reversa (B - blanco)
void reverseTask() {
  static bool reverseState = false;
  
  // Invertir el estado (encendido/apagado)
  reverseState = !reverseState;
  
  if (reverseState) {
    // Encender primeros y últimos 15 LEDs (o menos si la tira es más corta) en blanco
    int halfCount = min(15, NUM_LEDS / 2);
    
    // Encender los primeros LEDs
    for (int i = 0; i < halfCount; i++) {
      leds[i] = CRGB::White;
    }
    
    // Encender los últimos LEDs
    for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;
    }
  } else {
    // Apagar todos los LEDs
    FastLED.clear();
  }
}

// Efecto de dirección izquierda (L)
void directionLeftTask() {
  // Limpiar todos los LEDs
  FastLED.clear();
  
  // Dibujar los dos LEDs de la animación
  leds[leftPosition % NUM_LEDS] = CRGB::Green;
  leds[(leftPosition - 1 + NUM_LEDS) % NUM_LEDS] = CRGB::Blue;
  
  // Retroceder la posición (hacia la izquierda)
  leftPosition = (leftPosition - 1 + NUM_LEDS) % NUM_LEDS;
  
  // Pequeño retardo para que el movimiento no sea demasiado rápido
  delay(50);
}

// Efecto de dirección derecha (R)
void directionRightTask() {
  // Limpiar todos los LEDs
  FastLED.clear();
  
  // Dibujar los dos LEDs de la animación
  leds[rightPosition % NUM_LEDS] = CRGB::Red;
  leds[(rightPosition + 1) % NUM_LEDS] = CRGB::Blue;
  
  // Avanzar la posición (hacia la derecha)
  rightPosition = (rightPosition + 1) % NUM_LEDS;
  
  // Pequeño retardo para que el movimiento no sea demasiado rápido
  delay(50);
}

// Efecto de alto (S - rojo fijo)
void stopTask() {
  // Encender primeros y últimos 15 LEDs en rojo
  int halfCount = min(15, NUM_LEDS / 2);
  
  // Limpiar LEDs primero
  FastLED.clear();
  
  // Encender los primeros LEDs
  for (int i = 0; i < halfCount; i++) {
    leds[i] = CRGB::Red;
  }
  
  // Encender los últimos LEDs
  for (int i = max(halfCount, NUM_LEDS - halfCount); i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
}