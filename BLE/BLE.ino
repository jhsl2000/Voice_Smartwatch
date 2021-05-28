#include <ArduinoBLE.h>

//----------------------------------------------------------------------------------------------------------------------
// BLE UUIDs
//----------------------------------------------------------------------------------------------------------------------

#define BLE_UUID_PIXEL_SERVICE          "01430000-9743-472D-E4F5-3D58CDD61275"
#define BLE_UUID_PIXEL_MOOD             "01430001-9743-472D-E4F5-3D58CDD61275"

//----------------------------------------------------------------------------------------------------------------------
// BLE
//----------------------------------------------------------------------------------------------------------------------

#define BLE_DEVICE_NAME                 "Arduino Nano 33 BLE"
#define BLE_LOCAL_NAME                  "Arduino Nano 33 BLE"

BLEService pixelService( BLE_UUID_PIXEL_SERVICE );
BLEByteCharacteristic moodCharacteristic( BLE_UUID_PIXEL_MOOD, BLEWrite );

//----------------------------------------------------------------------------------------------------------------------
// Color picker
//----------------------------------------------------------------------------------------------------------------------
#include <Arduino_APDS9960.h>

//----------------------------------------------------------------------------------------------------------------------
// RGB STRIP
//----------------------------------------------------------------------------------------------------------------------


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

//----------------------------------------------------------------------------------------------------------------------
// I/O and App
//----------------------------------------------------------------------------------------------------------------------





#define BLE_LED_PIN                     LED_BUILTIN
#define PIXEL_LED_PIN                   LED_PWR

#define MOOD_NONE                       -1
int8_t mood = MOOD_NONE;

void setup()
{
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.begin( 9600 );


  pinMode( BLE_LED_PIN, OUTPUT );
  pinMode( PIXEL_LED_PIN, OUTPUT );
  digitalWrite( PIXEL_LED_PIN, LOW );

  if ( !setupBleMode() )
  {
    Serial.println( "Failed to initialize BLE!" );
    while ( 1 );
  }
  else
  {
    Serial.println( "BLE initialized. Waiting for clients to connect." );
  }

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
  }
}

void loop()
{
  serialTask();
  bleTask();
  pixelTask();
}

void serialTask()
{
  if ( !Serial.available() )
  {
    return;
  }
  char c = Serial.read();
  if ( c == '\n' || c == '\r' )
  {
    return; // Ignore CR and NL if send by Serial Monitor
  }
  mood = c;
}

void pixelTask()
{
  switch ( mood )
  {
    case 0:
    case '0':
      colorWipe(strip.Color(127,   0,   0), 50);
      theaterChase(strip.Color(127,   0,   0), 50); //vermelho

      break;
    case 1:
    case '1':
      colorWipe(strip.Color(0,   127,   0), 50);
      theaterChase(strip.Color(0,   127,   0), 50); //verde
      break;
    case 2:
    case '2':
      colorWipe(strip.Color(0,   0,   127), 50);
      theaterChase(strip.Color(0,   0,   127), 50); //azul
      break;
    case 3:
    case '3':
      colorWipe(strip.Color(127,   0,   127), 50);
      theaterChase(strip.Color(127,   0,   127), 50); //roxo
      break;
    case 4:
    case '4':
      colorWipe(strip.Color(255,   255,   0), 50);
      theaterChase(strip.Color(255,   255,   0), 50); //amarelo
      break;
    case 5:
    case '5':
      rainbow(50);
      break;
    case 6: // Escolher cor com o sensor
    case '6':
      int r, g, b, c, p; //c->brightness; p->proximity
      float sum;

      // wait for proximity and color sensor data
      while (!APDS.colorAvailable() || !APDS.proximityAvailable()) {}

      // read the color and proximity data
      APDS.readColor(r, g, b, c);
      sum = r + g + b;
      p = APDS.readProximity();

      // if object is close and well enough illuminated
      if (p == 0 && c > 10 && sum > 0) {

        theaterChase(strip.Color(r,   g,   b), 100);
        colorWipe(strip.Color(r,   g,   b), 100);
      }
      break;

    case MOOD_NONE:
      break;
    default:
      Serial.print( "Unknown mood: " );
      Serial.println( mood, HEX );
      break;
  }
  mood = MOOD_NONE;
}

bool setupBleMode()
{
  if ( !BLE.begin() )
  {
    return false;
  }

  // set advertised local name and service UUID
  BLE.setDeviceName( BLE_DEVICE_NAME );
  BLE.setLocalName( BLE_LOCAL_NAME );
  BLE.setAdvertisedService( pixelService );

  // add characteristics
  pixelService.addCharacteristic( moodCharacteristic );

  // add service
  BLE.addService( pixelService );

  // set the initial value for the characeristics
  // not needed all characteristics are write only

  // set BLE event handlers
  BLE.setEventHandler( BLEConnected, blePeripheralConnectHandler );
  BLE.setEventHandler( BLEDisconnected, blePeripheralDisconnectHandler );

  // set service and characteristic specific event handlers
  moodCharacteristic.setEventHandler( BLEWritten, bleCharacteristicWrittenHandler );

  // start advertising
  BLE.advertise();

  return true;
}


void bleTask()
{
#define BLE_UPDATE_INTERVAL 10
  static uint32_t previousMillis = 0;

  uint32_t currentMillis = millis();
  if ( currentMillis - previousMillis >= BLE_UPDATE_INTERVAL )
  {
    previousMillis = currentMillis;
    BLE.poll();
  }
}


void bleCharacteristicWrittenHandler( BLEDevice central, BLECharacteristic bleCharacteristic )
{
  Serial.print( "BLE characteristic written. UUID: " );
  Serial.print( bleCharacteristic.uuid() );

  if ( bleCharacteristic.uuid() == (const char*) BLE_UUID_PIXEL_MOOD )
  {
    bleCharacteristic.readValue( mood );
    Serial.print( " Value: " );
    Serial.println( mood, HEX );
  }
}


void blePeripheralConnectHandler( BLEDevice central )
{
  digitalWrite( BLE_LED_PIN, HIGH );
  Serial.print( "Connected to central: " );
  Serial.println( central.address() );

}


void blePeripheralDisconnectHandler( BLEDevice central )
{
  digitalWrite( BLE_LED_PIN, LOW );
  Serial.print( "Disconnected from central: " );
  Serial.println( central.address() );

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

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}
