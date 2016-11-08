// To be done:
// Beeping the last 5 LEDS
// Start sequence animation
// NOISOMETER
// DEMO: set a sequence in particular
// Facilitate upgrade lcd routine to include text messages and refresh at a given rate


#include <Arduino.h>
#include "RunningMedian.h"
// Information about hardware connections, functions and definitions
#include "Definitions.h"

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define NUM_LEDS 144  // Number of leds in strip
#define BRIGHTNESS 50  // Set LEDS brightness
#define FRAMES_PER_SECOND  120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_LEDS];  // Define the array of leds
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
RTC_DS1307 RTC; // define the Real Time Clock object

uint8_t start_hour_1=6, start_minute_1=20, start_second_1=0, end_hour_1=7, end_minute_1=30, end_second_1=0, start_time_1_seconds, end_time_1_seconds;
//long start_hour_1=7, start_minute_1=54, start_second_1=0, end_hour_1=7, end_minute_1=56, end_second_1=0, start_time_1_seconds, end_time_1_seconds;
uint8_t start_hour_2=18, start_minute_2=50, start_second_2=0, end_hour_2=19, end_minute_2=50, end_second_2=0, start_time_2_seconds, end_time_2_seconds;

int current_second = 0;  // Used to check frequency of time update on LCD
long now_seconds, end_seconds, remaining_seconds, change_time=0;  // now in seconds since 00:00:00, end time in seconds, count down of remain seconds
float seconds_between_drops;
static uint8_t default_hue=50;
static uint8_t hue = default_hue;
/*
 * 50=yellow
 * 100=green
 * 160=blue
 * 0=red
 */
static uint8_t hue_step = 24;
static uint8_t tail = 180;
static int step = -1;
static uint8_t pos = NUM_LEDS;
static uint8_t turned_on_leds = 0;
static uint8_t wait_time = 2;
char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
bool red_button_pressed=0;
bool green_button_pressed=0;
bool blue_button_pressed=0;
int sound_level=0, sound_limit=740;

RunningMedian SoundLevelSamples = RunningMedian(5);

DateTime now;

void setup(void)
{
  HardwareSetup ();
  
  Serial.begin(9600);

  //lcd.noBacklight();
  //lcd.backlight(); 
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("*** Sabluino ***");
  lcd.setCursor(0,1);
  lcd.print("*** Laketanou **");
  delay(1000);
 
  lcd.setCursor(0,0);
  for (char k=0;k<16;k++)
  {
    lcd.scrollDisplayLeft();
    delay(200);
  }

  Wire.begin();  // connect to RTC
  
  // following line sets the RTC to the date & time this sketch was compiled
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);    

  start_time_1_seconds=start_hour_1*3600+start_minute_1*60+start_second_1;
  Serial.println(start_time_1_seconds);
  end_time_1_seconds=end_hour_1*3600+end_minute_1*60+end_second_1;
  Serial.println(end_time_1_seconds);
  start_time_2_seconds=start_hour_2*3600+start_minute_2*60+start_second_2;
  end_time_2_seconds=end_hour_2*3600+end_minute_2*60+end_second_2;  
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {sinelon, juggle, bpm, rainbow, rainbowWithGlitter, confetti };
char* SimplePatternNames[]={"sinelon", "juggle", "bpm", "rainbow", "rainbowWithGlitter", "confetti" };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void fadeall() {
  for (int i = turned_on_leds+1; i < NUM_LEDS; i++) {
    leds[i].nscale8(tail);
  }
}
void alloff() {
  for (int i = NUM_LEDS; i >=0; i--) {
    leds[i]=CRGB::Black;
    delay(20);
    FastLED.show();
  }
  //FastLED.show();
}
void Beep (long loud_duration, long loud_frequency, long silent_duration)  {   
  // Function to beep piezo - Used as recall alarm
  // All durations in milliseconds, frequency in Hz
  long nb_half_periods=loud_duration*2*loud_frequency/1000;
  long loud_half_period=1000000/(2*loud_frequency);  // microseconds
  for (int i; i<nb_half_periods; i++) { 
       HPToggle;
       delayMicroseconds (loud_half_period); // microseconds
     }
     delay (silent_duration);  // milliseconds
}
void UpdateTime(const DateTime& dt, const long remain) {
       lcd.clear();
       lcd.setCursor(0,0);
       //if(dt.month()<10){lcd.print("0");}
       lcd.print(dt.month(), DEC);
       lcd.print("/");
       //if(dt.day()<10){lcd.print("0");}
       lcd.print(dt.day(), DEC);
       lcd.print(" ");
       if(dt.hour()<10){lcd.print("0");}
       lcd.print(dt.hour(), DEC);
       lcd.print(":");
       if(dt.minute()<10){lcd.print("0");}
       lcd.print(dt.minute(), DEC);
       lcd.print(":");
       if(dt.second()<10){lcd.print("0");}
       lcd.print(dt.second(), DEC);
       lcd.print(' ');
       lcd.print(daysOfTheWeek[dt.dayOfTheWeek()]);
       if (remain>0){
         lcd.setCursor(0,1);
         lcd.print(remain/60);
         lcd.print(" min ");
         if(remain % 60<10){lcd.print("0");}
         lcd.print(remain % 60);
         lcd.print(" sec ");       
         }
       if (remain==-9999) {
         lcd.setCursor(0,1);
         lcd.print("Timer Demo Noise");
       }
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

enum {Idle,Count_down_time,Starting,Count_down,Timer,Ending,Demo,Noisometer,QuietPlease,Adjust_RTC} condition=Idle;

void loop(void)
{
  delay (wait_time);
  switch (condition) {
  case Idle:
     now = RTC.now(); // fetch the time
     
     now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());
     
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       UpdateTime(now,-9999);
     }
    
     if(now_seconds>=start_time_1_seconds & now_seconds<end_time_1_seconds-5 & daysOfTheWeek[now.dayOfTheWeek()]!="Sun" & daysOfTheWeek[now.dayOfTheWeek()]!="Sat"){
       condition=Count_down;    
       wait_time=20;
       //wait_time=2;
       Beep(2000,440,200);Beep(600,880,200);
       remaining_seconds=end_time_1_seconds-now_seconds;  
       seconds_between_drops=float(remaining_seconds)/float(NUM_LEDS);
       end_seconds=remaining_seconds+now_seconds;
       lcd.clear();
       break;
     }
     
     if(now_seconds>=start_time_2_seconds & now_seconds<end_time_2_seconds-5 & daysOfTheWeek[now.dayOfTheWeek()]!="Sun" & daysOfTheWeek[now.dayOfTheWeek()]!="Sat"){
       condition=Count_down;  
       wait_time=20;
       Beep(2000,440,200);Beep(600,880,200);
       remaining_seconds=end_time_2_seconds-now_seconds;  
       seconds_between_drops=float(remaining_seconds)/float(NUM_LEDS);
       end_seconds=remaining_seconds+now_seconds;
       lcd.clear();
       break;
     } 
         
     if(red_button_On){
      delay(100);
      if(!red_button_On){condition=Count_down_time;current_second=-1;break;}
     }
  
     if(green_button_On){
      delay(100);
      if(!red_button_On){condition=Demo;current_second=-1;break;}
      break;
     }

     if(blue_button_On){
      delay(100);
      if(!red_button_On){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.println("Noisometer         ");
        condition=Noisometer; 
        current_second=-1;
        break;
      }
      break;
     }
      
     break;
     
  case Count_down_time:
     now = RTC.now(); // fetch the time
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Set time");
       lcd.setCursor(0,1);
       lcd.print("2 5 10 minutes");
     }

     if(red_button_On){
       now = RTC.now(); // fetch the time
       now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());
       condition=Count_down;
       //condition=Timer;
       wait_time=2;
       Beep(600,440,200);Beep(600,880,200);
       remaining_seconds=120;
       seconds_between_drops=float(remaining_seconds)/float(NUM_LEDS);
       end_seconds=remaining_seconds+now_seconds;
       lcd.clear();
       break;
     }
  
     if(green_button_On){
       now = RTC.now(); // fetch the time
       now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());
       //condition=Count_down;
       condition=Timer;
       wait_time=2;
       Beep(600,440,200);Beep(600,880,200);
       remaining_seconds=300;
       seconds_between_drops=float(remaining_seconds)/float(NUM_LEDS);
       end_seconds=remaining_seconds+now_seconds;
       lcd.clear();
       break;
     }

     if(blue_button_On){
       now = RTC.now(); // fetch the time
       now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());     
       //condition=Count_down;
       condition=Timer;
       wait_time=2;
       Beep(600,440,200);Beep(600,880,200);
       remaining_seconds=600;
       seconds_between_drops=float(remaining_seconds)/float(NUM_LEDS);
       end_seconds=remaining_seconds+now_seconds;
       lcd.clear();
       break;
     }
  break;

case Count_down:
     now = RTC.now(); // fetch the time
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       remaining_seconds=end_seconds-(now.hour()*3600.0+now.minute()*60.0+now.second());
       UpdateTime(now,remaining_seconds);
     }
     
     if (remaining_seconds<((NUM_LEDS-turned_on_leds)*seconds_between_drops)){
       pos = pos + step;
     }

     if(red_button_On){
        red_button_pressed=1;
        hue=0;
     }
     if(green_button_On){
        green_button_pressed=1;
        hue=100;
     }
     if(blue_button_On){
        blue_button_pressed=1;
        hue=160;
     }

     if (pos==turned_on_leds) {
       pos = NUM_LEDS;
       turned_on_leds++;
     }

     leds[pos] = CHSV(hue, 255, 255);
   
     FastLED.show();
     fadeall();
     if (turned_on_leds>=NUM_LEDS){
      if(!red_button_pressed | !green_button_pressed | !blue_button_pressed){
        // Game over melody
        for (int i=27;i>=-27;i--){
          Beep(200,int(440.0*pow(2.0,float(i/12.0))),1);
        }
        Beep(3000,int(440.0*pow(2.0,float(-27.0/12.0))),1);
      }
      else {
        // Victory melody
        Beep(3000,int(440.0*pow(2.0,float(-27.0/12.0))),1);
        for (int i=-27;i<=27;i++){
          Beep(200,int(440.0*pow(2.0,float(i/12.0))),1);
        }
        Beep(3000,int(440.0*pow(2.0,float(27.0/12.0))),1);
      }
      red_button_pressed=0; green_button_pressed=0; blue_button_pressed=0; hue=default_hue;   // Reset buttons and color for next round
      alloff();condition=Ending;change_time=long(now.hour())*3600+long(now.minute())*60+long(now.second());  }
     
break;
    
case Timer:
     //Serial.println("Count_down");
     now = RTC.now(); // fetch the time
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       remaining_seconds=end_seconds-(now.hour()*3600.0+now.minute()*60.0+now.second());
       UpdateTime(now,remaining_seconds);
     }
     
     if (remaining_seconds<((NUM_LEDS-turned_on_leds)*seconds_between_drops)){
       pos = pos + step;
     }

     if (pos==turned_on_leds) {
       if (pos % hue_step==0){
         hue=hue+hue_step;
       }
       pos = NUM_LEDS;
       turned_on_leds++;
     }

     leds[pos] = CHSV(hue, 255, 255);
   
     FastLED.show();
     fadeall();
     
     //if(red_button_On){condition=Idle;Beep(200,440,200);Beep(200,440,200);break;}

     if (turned_on_leds>=NUM_LEDS){Beep(1000,440,500);Beep(1000,220,10);alloff();condition=Ending;change_time=long(now.hour())*3600+long(now.minute())*60+long(now.second());  }
     
     break;

  case Ending:
     // Call the pattern function once, updating the 'leds' array
     juggle();
     //bpm();
     
     now = RTC.now(); // fetch the time
     now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());
     
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println("**TIME TO GO!!**");  
       lcd.setCursor(0,1);
       lcd.print("*IL EST L'HEURE*");   
     }
     
     // send the 'leds' array out to the actual LED strip
     FastLED.show();  
     // insert a delay to keep the framerate modest
     FastLED.delay(1000/FRAMES_PER_SECOND); 

     // do some periodic updates
     EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
     //EVERY_N_SECONDS( 5 ) { Beep(500,440,0); } // Beep periodically
     if (now_seconds>change_time+60){condition=Idle;alloff();turned_on_leds = 0;pos=NUM_LEDS;current_second = 0;hue = default_hue; }

     if(blue_button_On){condition=Idle;break;}
     break;

case Demo:
     // Call the current pattern function once, updating the 'leds' array
     gPatterns[gCurrentPatternNumber]();

     now = RTC.now(); // fetch the time
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println("Demo            ");  
       lcd.setCursor(0,1);
       lcd.print(SimplePatternNames[gCurrentPatternNumber]);  
       lcd.println("                ");  
     }
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically

     if(green_button_On){condition=Idle;break;}
     break;
     
  case Noisometer:
     sound_level=analogRead(mic_analog);
     SoundLevelSamples.add(sound_level);
     Serial.println(SoundLevelSamples.getHighest());
     if (SoundLevelSamples.getHighest()>sound_limit){
       condition=QuietPlease;
       now = RTC.now();
       change_time=long(now.hour())*3600+long(now.minute())*60+long(now.second());
       SoundLevelSamples.clear();
     }
     delay(50);

     if(green_button_On){condition=Demo;break;}
     if(red_button_On){condition=Count_down_time;break;}
     
     break;
     
case QuietPlease:
     // Call the pattern function once, updating the 'leds' array
     bpm();
     
     now = RTC.now(); // fetch the time
     now_seconds=long(now.hour())*3600+long(now.minute())*60+long(now.second());
     
     // Write to LCD
     if(now.second()>current_second){
       current_second=now.second();
       if(current_second>=59){current_second=0;}
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println("*QUIET PLEASE!!*");  
       lcd.setCursor(0,1);
       lcd.print("**SILENCE SVP!**");   
     }
     
     // send the 'leds' array out to the actual LED strip
     FastLED.show();  
     // insert a delay to keep the framerate modest
     FastLED.delay(1000/FRAMES_PER_SECOND); 

     // do some periodic updates
     EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
     EVERY_N_SECONDS( 1 ) { Beep(100,440,0); } // Beep periodically
     if (now_seconds>change_time+5){
       condition=Noisometer;
       alloff();
       turned_on_leds = 0;
       pos=NUM_LEDS;
       current_second = 0;
       hue = default_hue;
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println("Noisometer         "); 
     }

     if(green_button_On){condition=Demo;break;}
     if(red_button_On){condition=Count_down_time;break;}
     break;

     
  case Adjust_RTC:
     Serial.println("Adjust_RTC");
     break;
  }
}


