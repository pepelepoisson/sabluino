// Definition of ports used and corresponding names
#include <Arduino.h>

// To be used with DS1307 RTC and LCD with I2C backpack
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include "FastLED.h"
FASTLED_USING_NAMESPACE

//  Declare pins
#define DATA_PIN 2  // Strip data pin
#define bHP 6 // Piezo buzzer
#define HPOn   bitSet  (PORTD,bHP) 
#define HPOff  bitSet  (PORTD,bHP)
#define HPToggle  PORTD ^= (1<<bHP)
#define blue_button 7 // D7
#define blue_button_On (!digitalRead(blue_button))
#define green_button 8 // D8
#define green_button_On (!digitalRead(green_button))
#define red_button 9 // D9
#define red_button_On (!digitalRead(red_button))
#define mic_digital 5 // D5
#define mic_analog 6 // A6

void HardwareSetup () {
  pinMode(bHP,OUTPUT);
  pinMode(blue_button,INPUT);
  digitalWrite(blue_button, HIGH);  // Apply pull-up resitor
  pinMode(green_button,INPUT);
  digitalWrite(green_button, HIGH);  // Apply pull-up resitor
  pinMode(red_button,INPUT);
  digitalWrite(red_button, HIGH);  // Apply pull-up resitor 
  pinMode(mic_digital,INPUT);
  pinMode(mic_analog,INPUT);
}


