
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    9

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 30

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#include <math.h>
#include <PDM.h>
#include <EloquentTinyML.h>      // https://github.com/eloquentarduino/EloquentTinyML
#include "tf_lite_model_50.h"       // TF Lite model file


#define PDM_SOUND_GAIN     255   // sound gain of PDM mic
#define PDM_BUFFER_SIZE    256   // buffer size of PDM mic

#define SAMPLE_THRESHOLD   900   // RMS threshold to trigger sampling
#define FEATURE_SIZE       32    // sampling size of one voice instance
#define SAMPLE_DELAY       20    // delay time (ms) between sampling

#define NUMBER_OF_LABELS   3     // number of voice labels
const String words[NUMBER_OF_LABELS] = {"Vermelho", "Verde", "Azul"};  // words for each label


#define PREDIC_THRESHOLD   0.6   // prediction probability threshold for labels
#define RAW_OUTPUT         true  // output prediction probability of each label
#define NUMBER_OF_INPUTS   FEATURE_SIZE
#define NUMBER_OF_OUTPUTS  NUMBER_OF_LABELS
#define TENSOR_ARENA_SIZE  4 * 1024


Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> tf_model;
float feature_data[FEATURE_SIZE];
volatile float rms;
bool voice_detected;


// callback function for PDM mic
void onPDMdata() {

  rms = -1;
  short sample_buffer[PDM_BUFFER_SIZE];
  int bytes_available = PDM.available();
  PDM.read(sample_buffer, bytes_available);

  // calculate RMS (root mean square) from sample_buffer
  unsigned int sum = 0;
  for (unsigned short i = 0; i < (bytes_available / 2); i++) sum += pow(sample_buffer[i], 2);
  rms = sqrt(float(sum) / (float(bytes_available) / 2.0));
}

void setup() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.begin(115200);
  while (!Serial);

  PDM.onReceive(onPDMdata);
  PDM.setBufferSize(PDM_BUFFER_SIZE);
  PDM.setGain(PDM_SOUND_GAIN);

  if (!PDM.begin(1, 16000)) {  // start PDM mic and sampling at 16 KHz
    Serial.println("Failed to start PDM!");
    while (1);
  }

  pinMode(LED_BUILTIN, OUTPUT);

  // wait 1 second to avoid initial PDM reading
  delay(900);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  // start TF Lite model
  tf_model.begin((unsigned char*) model_data);

  Serial.println("=== Classifier start ===\n");



}

void loop() {

  // waiting until sampling triggered
  while (rms < SAMPLE_THRESHOLD);

  //digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < FEATURE_SIZE; i++) {  // sampling
    while (rms < 0);
    feature_data[i] = rms;
    delay(SAMPLE_DELAY);
  }
  digitalWrite(LED_BUILTIN, LOW);

  // predict voice and put results (probability) for each label in the array
  float prediction[NUMBER_OF_LABELS];
  tf_model.predict(feature_data, prediction);

  // print out prediction results;
  // in theory, you need to find the highest probability in the array,
  // but only one of them would be high enough over 0.5~0.6
  Serial.println("Predicting the word:");
  if (RAW_OUTPUT) {
    for (int i = 0; i < NUMBER_OF_LABELS; i++) {
      Serial.print("Label ");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(prediction[i]);
    }
  }
  voice_detected = false;
  for (int i = 0; i < NUMBER_OF_LABELS; i++) {
    if (prediction[i] >= PREDIC_THRESHOLD) {
      theaterChase(strip.Color(  0,   0, 0), 50);
      Serial.print("Word detected: ");
      Serial.println(words[i]);
      Serial.println("");
      voice_detected = true;

      if (words[i] == "Azul") {
        theaterChase(strip.Color(  0,   0, 127), 50); //azul
        colorWipe(strip.Color(  0,   0, 255), 50); // azul

      }
      else if (words[i] == "Verde") {
        theaterChase(strip.Color(0,   127,   0), 50); //verde
        colorWipe(strip.Color(  0, 255,   0), 50); // verde


      }
      else if (words[i] == "Vermelho") {
        theaterChase(strip.Color(127,   0,   0), 50); //vermelho
        colorWipe(strip.Color(255,   0,   0), 50); // Red



      }
    }
  }
  if (!voice_detected && !RAW_OUTPUT) {
    Serial.println("Word not recognized\n");
  }

  // wait for 1 second after one sampling/prediction
  delay(900);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);


}

void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; a++) { // Repeat 10 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}
