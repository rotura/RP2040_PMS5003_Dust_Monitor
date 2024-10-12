#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Animations.h"

#define PIN 16
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

long lastMillis_Animation;
long lastMillis_Data;
int actualAnimation = 0;
const int animationLength = 5;
boolean serialReceived = false;
int serialBuffer[32]; 
int sum = 0;
short serialPosition = 0;

void setup() {
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.show();
  Serial.begin(115200);
  Serial1.setRX(1);
  Serial1.setTX(0);
  Serial1.begin(9600);
  delay(1500);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
  } 

  display.clearDisplay();
  display.dim(true);        // Contrast set to 31 instead of 127.
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("Starting..."));
  display.display();      
  delay(100);  

  analogReadResolution(12); 
  pixels.setPixelColor(0, pixels.Color(20, 0, 0));
  pixels.show();
  lastMillis_Animation = lastMillis_Data = millis();
  Serial.print(F("\nDust sensor init"));
  multicore_reset_core1();
  multicore_launch_core1(main2);
}

// Struct for PMS5003 sensor data
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_estandar, pm25_estandar, pm100_estandar;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particulas_03um, particulas_05um, particulas_10um, particulas_25um, particulas_50um, particulas_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;
struct pms5003data oldData;
void readPMSdata() {
  if (Serial1.available() > 0) { 
    int charReaded = Serial1.read();
    //Check for first byte of the message
    if(!serialReceived && charReaded == 66){
        serialReceived = true;
        serialBuffer[0] = charReaded;
        serialPosition = 1;
    } else if(serialReceived && serialPosition < 32){   // Get the full 32 bytes message
        serialBuffer[serialPosition++] = charReaded;
    } else if(serialReceived && serialPosition == 32){  // Transform the buffer to object    
        // Calculate checksum
        for (uint8_t i = 0; i < 30; i++) {
          sum += serialBuffer[i];
        }
        //Swap (Big-endian).
        uint16_t buffer_u16[15];
        for (uint8_t i = 0; i < 15; i++) {
            buffer_u16[i] = serialBuffer[2 + i * 2 + 1];
            buffer_u16[i] += (serialBuffer[2 + i * 2] << 8);
        }
        memcpy((void *)&oldData, (void *)&data, 30);    // Copy the last data for checksum fail
        memcpy((void *)&data, (void *)buffer_u16, 30);  // Copy the data to the struct
        if(sum != data.checksum){                       // If checksumm fail, restore old data
            memcpy((void *)&data, (void *)&oldData, 30);
        }
        serialReceived = false;                         // Reset flag to read next package
        sum = 0;
    } 
  }
}

void loop() {
  // Read sensor
  readPMSdata();
  // Send data every 5 second
  if (millis() - lastMillis_Data > 5000) {
    lastMillis_Data = millis();
    Serial.print(F("\n\nUnidades de Concentración (estándar)"));
    Serial.print(F("\nPM 1.0: ")); Serial.print(data.pm10_estandar);
    Serial.print(F("\nPM 2.5: ")); Serial.print(data.pm25_estandar);
    Serial.print(F("\nPM 10: ")); Serial.print(data.pm100_estandar);

    Serial.print(F("\nUnidades de Concentración (ambiental)"));
    Serial.print(F("\nPM 1.0: ")); Serial.print(data.pm10_env);
    Serial.print(F("\nPM 2.5: ")); Serial.print(data.pm25_env);
    Serial.print(F("\nPM 10: ")); Serial.print(data.pm100_env);

    Serial.print(F("\nParticulas > 0.3um   / 0.1L aire:")); Serial.print(data.particulas_03um); 
    Serial.print(F("\nParticulas > 0.5um   / 0.1L aire:")); Serial.print(data.particulas_05um);
    Serial.print(F("\nParticulas > 1.0um   / 0.1L aire:")); Serial.print(data.particulas_10um);
    Serial.print(F("\nParticulas > 2.5um   / 0.1L aire:")); Serial.print(data.particulas_25um);
    Serial.print(F("\nParticulas > 5.0um   / 0.1L aire:")); Serial.print(data.particulas_50um);
    Serial.print(F("\nParticulas > 10.0 um / 0.1L aire:")); Serial.print(data.particulas_100um);
    if (data.pm25_estandar < 12) {
      Serial.print(F("\nPM 2.5 Calidad Buena"));
      pixels.setPixelColor(0, pixels.Color(20, 0, 0));
      pixels.show();
    }
    if (data.pm25_estandar >= 12 && data.pm25_estandar < 35) {
      Serial.print(F("\nPM 2.5 Calidad Moderada"));
      pixels.setPixelColor(0, pixels.Color(20, 20, 0));
      pixels.show();
    }
    if (data.pm25_estandar >= 35 && data.pm25_estandar < 55) {
      Serial.print(F("\nPM 2.5 Calidad Mala"));
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
      pixels.show();
    }
    if (data.pm25_estandar >= 55 && data.pm25_estandar < 150) {
      Serial.print(F("\nPM 2.5 Calidad Muy mala"));
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
      pixels.show();
    }
    if (data.pm100_estandar < 55) {
      Serial.print(F("\nPM 10.0 Calidad Buena"));
      pixels.setPixelColor(0, pixels.Color(20, 0, 0));
      pixels.show();
    }
    if (data.pm100_estandar >= 55 && data.pm100_estandar < 155) {
      Serial.print(F("\nPM 10.0 Calidad Moderada"));
      pixels.setPixelColor(0, pixels.Color(20, 20, 0));
      pixels.show();
    }
    if (data.pm100_estandar >= 155 && data.pm100_estandar < 255) {
      Serial.print(F("\nPM 10.0 Calidad Mala"));
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
      pixels.show();
    }
    if (data.pm100_estandar >= 255 && data.pm100_estandar < 355) {
      Serial.print(F("\nPM 10.0 Calidad Muy mala"));
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
      pixels.show();
    }
  }
}

void main2() {
    setup2();
    while (1) {
        loop2();
    }
} 

void setup2() {
  
  } 

void loop2() {
    // Update oled every 300ms
    if(millis() - lastMillis_Animation > 300){
      lastMillis_Animation = millis();
      display.clearDisplay();
      display.setTextSize(1); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 5);
      
      display.print(F("PM10  env: ")); display.println(data.pm10_env); 
      display.print(F("PM25  env: ")); display.println(data.pm25_env); 
      display.print(F("PM100 env: ")); display.println(data.pm100_env); 
      display.println(F(""));
      display.print(F("PM10  std: ")); display.println(data.pm10_estandar); 
      display.print(F("PM25  std: ")); display.println(data.pm25_estandar); 
      display.print(F("PM100 std: ")); display.println(data.pm100_estandar);

      display.drawBitmap(128-13, 0, bat_array[voltageToPercent(readBatt())], 12, 8, WHITE);
      display.drawBitmap(128-38, 64-56, animation[actualAnimation], 38, 56, WHITE);
      actualAnimation++;
      if(actualAnimation > animationLength){
        actualAnimation = 0;
      }

      display.display();
    } 
  }

float readBatt(){
  int pinValue = analogRead(29);
  // Convert the raw data value (0 - 4095) to voltage (0.0V - 3.3V)
  return pinValue * (3.3 / 4096.0); 
}

int voltageToPercent(float v){
  if (v > 0.80)
    return 3;
  else if (v > 0.78)
    return 2;
  else if (v > 0.76)
    return 1;
  else if (v > 0.73)
    return 0;
  else if (v < 0.5)
    return 4;
  else
    return 0;
}