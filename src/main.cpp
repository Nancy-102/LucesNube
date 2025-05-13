#include <Arduino.h>
#include <FastLED.h>
#include <TaskScheduler.h>

// Configuración de la tira LED
#define LED_PIN     D4      // Pin donde está conectada la tira LED
#define NUM_LEDS    30      // Número de LEDs en la tira
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

// Crear scheduler y tareas
Scheduler scheduler;
Task tRainbow(5000, TASK_FOREVER, &rainbowTask);
Task tColorWipe(5000, TASK_FOREVER, &colorWipeTask);
Task tMeteor(5000, TASK_FOREVER, &meteorTask);
Task tBreathe(5000, TASK_FOREVER, &breatheTask);
Task tTwinkle(5000, TASK_FOREVER, &twinkleTask);

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
  
  Serial.println("Sistema inicializado");
}

void loop() {
  scheduler.execute();
  FastLED.show();
  FastLED.delay(1000 / 120); // Usando 120 FPS directamente
}

// Función para cambiar a la siguiente animación
void switchToNextEffect() {
  // Deshabilitar todas las tareas
  tRainbow.disable();
  tColorWipe.disable();
  tMeteor.disable();
  tBreathe.disable();
  tTwinkle.disable();
  
  // Limpiar LEDs
  FastLED.clear();
  
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

// Funciones de animación

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