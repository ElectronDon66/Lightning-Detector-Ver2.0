/*
  This Lightning Detector Project uses the SparkFun AS3935 sensor ,Teensy 4.0 Microcontroller and a 320x200 TFT display

  By:ElectronDon66
    Date: October 29,2024
  License: This code is public domain 

Ver 1.12 This is working 6/23/24 but has bugs
Ver1.2 Lightning timer bug fixed, Energy level overwrite bug fixed 
VER 1.3 range select switch bug fixed. Seems most all bugs fixed.  This also seems to be working with the Audio FX board installed. 
Ver 1.4 Adding the sounds
Ver 1.5 Trouble shooting sound dying after all clear 

This code uses the Sparkfun Libraries for the AS3935, Make sure you fix the indoor/outdoor code in sparkfun library otherwise you can never select outdoors mode
Known bugs in "range to storm and the storm intensity read function" (probably a race condition when reading the registers )
*/

#include <SPI.h>
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include <U8g2lib.h>

#include "SparkFun_AS3935.h"
// For the Adafruit shield, these are the default.
//#define SD_CS  D3     // Chip Select for SD-Card , not used in this sketch
#define TFT_DC 9   // Data/Command pin for SPI TFT screen
#define TFT_CS 2     // Chip select for TFT screen
#define TFT_RST 4     //Reset line 
#define TFT_MOSI 11  // Master out slave in pin for spark fun and TFT
#define TFT_CLK 13  // TFT and spark fun  clock pin
#define TFT_MISO 12  // TFT and spark fun MISO
#define spiCS 6  // chip select for the Sparkfun lightning board


#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01  //Mask for noise 
#define BROWN      0x5140 //0x5960
#define SKY_BLUE   0x02B5 //0x0318 //0x039B //0x34BF
#define DARK_RED   0x8000
#define DARK_GREY  0x39C7
#define BLACK      0x0000
// Define the TFT lines for character display 
#define  LINE1  +0
#define  LINE2  30
#define  LINE3  50
#define  LINE4  70
#define  LINE5  125
#define  LINE6  150
#define  LINE7  200
#define  LINE8  220
#define INDOOR 0x12 
#define OUTDOOR 0xE
#define Rangesw 7  //this is the input pin for the range select switch
 // Values for modifying the IC's settings. All of these values are set to their
// default values. 
byte noiseFloor = 2;
byte watchDogVal = 2;
byte spike = 1;
byte lightningThresh = 1; 

/* These are the WAV files on the FX card, Use the pins to trigger them 
 woop T00 // woop is the sound for lightning detection
 gong T01 // Chinese gong is additional sound for lightning detection
woop T00  , Siren woop for lightning detection 
gong  T01  , Chinese gong sound 
harp T02 , strumming harp for the all clear sound
radar T03 , radar sound is for disturber detection 
*/
// Define the pins and sounds for lightning detection and All clear for the FX sound board
#define woop 23 // Teensy pin 23 is wired to 0 on FX board
#define gong 22 // Teensy pin 22 is wired to 1 on the FX board
#define harp 21 // Teensy pin 21 is wired to 2 on the FX board
#define radar 20 // Teensy pin 20 is wired to 3 on the FX board 
byte sound =0 ; //variable for passing sound selection to make_sound function 

SparkFun_AS3935 lightning;  //Start the sparkfun board
// Start the TFT display 
// Use hardware SPI on Teensy,  and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Interrupt pin for lightning detection 
const int lightningInt = 3; 


// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
int intVal = 0;
//int noiseFloor = 2; // Value between 1-7 
//int disturber = 2; // Value between 1-10
//int spike = 2;
//int lightningThreshold =2;
int enviVal = 0; //set this for indoor or outdoor

//Initialize a counter for time of last strike
//int counter=0;
//This should be how long we wait before declaring an all clear ~ 20 min or 1200000
const long  LTmax_time= 100000; //Lightning time out 100 sec for testing
// const long LTmax_time = 1200000; 20 min setting, comment out previous line if this is used
unsigned long LTstart_time ; //Start time in milliseconds for strike detection
unsigned long LT_current_time;
const long  distmax_time = 30000; // 30 secv time out
unsigned long diststart_time;//start time for disturber
unsigned long dist_current_time; 
bool LT = false; //Lightning timer active = true
bool DT = false; //Disturber timer active = true
// Make D7 the indor/outdoor range select input
 bool Rangeselectold = HIGH; 

void setup()
{
 // lightning.clearStatistics(HIGH);
//void resetSettings();  ? Use this to reset all the registers
//declare the Indoor / Outdoor switch setting (presently this is only done on initial setup)
  pinMode (Rangesw, INPUT_PULLUP); //Setup pin 7 as range input open = range set to indoor
 pinMode(TFT_RST, OUTPUT);    // sets the digital pin RST as output
digitalWrite(TFT_RST, HIGH); // sets the digital pin 13 on
  delay(1000);            // waits for a second
 
 //Reset the display
 digitalWrite(TFT_RST, LOW); // sets the digital pin 13 on
  delay(100);            // waits for a second
  digitalWrite(TFT_RST, HIGH);  // sets the digital pin 13 off
  delay(100);            // waits for a second

  tft.begin();
display_meter(); //Clear display 
// set the pins for triggering sounds 
  pinMode (woop , OUTPUT_OPENDRAIN)  ; 
   pinMode (gong , OUTPUT_OPENDRAIN)  ; 
    pinMode (harp , OUTPUT_OPENDRAIN)  ; 
     pinMode (radar , OUTPUT_OPENDRAIN)  ; 
//Set the sound outputs high for no sound 
digitalWrite(woop , HIGH); 
digitalWrite(gong , HIGH);
digitalWrite(harp , HIGH);
digitalWrite(radar , HIGH);
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightningInt, INPUT); 

  Serial.begin(115200); 
  Serial.println("AS3935 Franklin Lightning Detector"); 

  SPI.begin(); 

  if( !lightning.beginSPI(spiCS ))
  
  
  
  
  
  { 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("Schmow-ZoW, Lightning Detector Ready!");

  // The lightning detector defaults to an indoor setting at 
  // the cost of less sensitivity, if you plan on using this outdoors 
  // uncomment the following line:
  
  //Set the registers
  //lightning.setIndoorOutdoor(INDOOR); 
  lightning.setNoiseLevel(noiseFloor);  
lightning.watchdogThreshold(watchDogVal); 

  int watchVal = lightning.readWatchdogThreshold();
  Serial.print("Watchdog Threshold is set to: ");
  Serial.println(watchVal);

lightning.spikeRejection(spike); 

  int spikeVal = lightning.readSpikeRejection();
  Serial.print("Spike Rejection is set to: ");
  Serial.println(spikeVal);

  // This setting will change when the lightning detector issues an interrupt.
  // For example you will only get an interrupt after five lightning strikes
  // instead of one. Default is one, and it takes settings of 1, 5, 9 and 16.   
  // Followed by its corresponding read function. Default is zero. 

  lightning.lightningThreshold(lightningThresh); 

  uint8_t lightVal = lightning.readLightningThreshold();
  Serial.print("The number of strikes before interrupt is triggerd: "); 
  Serial.println(lightVal); 

  delay (10);

//Run a  calibration as it just started up. 
if(lightning.calibrateOsc())
      Serial.println("Successfully Calibrated!");
    
    else
      Serial.println("Not Successfully Calibrated!");
 delay (1000);

// Enable below  if we want to see the tuner cap value on the monitor 
/*lightning.tuneCap(uint8_t _farad);
 print out the tuner cap value
Serial.print("Tuner Cap value  ");
 float x = lightning.readTuneCap();
Serial.println (x);
 delay (10);
 */

// "Disturbers" are events that are false lightning events. If you find
  // yourself seeing a lot of disturbers you can have the chip not report those
  // events on the interrupt lines. 
  lightning.maskDisturber(false); //Make this switch adjustable 
  int maskVal = lightning.readMaskDisturber();
  Serial.print("Are disturbers being masked: "); 
  if (maskVal == 1)
    Serial.println("YES"); 
  else if (maskVal == 0)
    Serial.println("NO"); 

// Determine if indoor or outdoor is set
//Read the Indoor / Outdoor switch setting (presently this is only done on initial setup)
digitalRead(Rangesw) ; //read the switch setting 
Rangeselectold=Rangesw;
if( digitalRead (Rangesw)== HIGH )
     lightning.setIndoorOutdoor(INDOOR);
       else if( digitalRead (Rangesw)== LOW )
    lightning.setIndoorOutdoor(OUTDOOR);

 enviVal = lightning.readIndoorOutdoor();
  Serial.print("Are we set for indoor or outdoor: "); 
  delay (10);  
 
 if( enviVal == INDOOR )
    Serial.println("Indoor.");  
  else if( enviVal == OUTDOOR )
    Serial.println("Outdoor."); 
 Serial.print("enviVal ="); 
  Serial.println(enviVal ,BIN);
 
 // Now update the TFT 
Range_Select(); 
 delay(10);
 
 draw_2(); // Set screen for all clear to start

}



void loop()
{
 
LT_current_time = millis();
dist_current_time = millis();

//Check the Range selector switch , if the setting changed update the range , if no change don't do anything here
if(digitalRead(Rangesw) != Rangeselectold){
 
 if( digitalRead (Rangesw)== HIGH )
     lightning.setIndoorOutdoor(INDOOR);
       else if( digitalRead (Rangesw)== LOW )
    lightning.setIndoorOutdoor(OUTDOOR);
Rangeselectold = digitalRead(Rangesw);// Update the range select register 
 enviVal = lightning.readIndoorOutdoor();
  Serial.print("Are we set for indoor or outdoor: "); 
  delay (10);  
 
 if( enviVal == INDOOR )
    Serial.println("Indoor.");  
  else if( enviVal == OUTDOOR )
    Serial.println("Outdoor."); 
 Serial.print("enviVal ="); 
  Serial.println(enviVal ,BIN);
 
 // Now update the TFT 
Range_Select(); 
 delay(10); 
// Since the range setting changed clear statastics  
lightning.clearStatistics(1);
}
else



 //Serial.println("checking lightning interrupt");
   
   // Hardware has alerted us to an event, now we read the interrupt register
  if(digitalRead(lightningInt) == HIGH){
    intVal = lightning.readInterruptReg();
    Serial.print("INTVAL"); //This is for checking what was detected
    Serial.println(intVal);
    if(intVal == NOISE_INT){
      Serial.println("Noise."); 
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      //lightning.setNoiseLevel(noiseFloor); 
    }
    else if(intVal == DISTURBER_INT){
      Serial.println("Disturber."); 
      tft.setCursor(10,LINE2);   // Set cursor to the top left for the title
  tft.setTextSize(2);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_YELLOW);
  tft.println("Disturber  "); // Display Disturber 
  make_sound(radar); //make disturber sound 
      diststart_time = millis(); //get the disturber start time
      DT = true; //Disturber  timer flag is set
      
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      //lightning.watchdogThreshold(disturber);  
    }
    else if(intVal == LIGHTNING_INT){
      //draw_3();
      LTstart_time = millis(); // get the start time for the lightning counter
      LT= true; //Lightning Timer flag is set 
      draw_3();// Indicate lightning strike
      draw_4(); //Set up the timer 
      make_sound (woop); //Generate a sound 
}
 
 }

//iF lIGHTNING HAS BEEN DETECTED DISPLAY A COUNTER 
 if (LT == true){
    draw_6(); // Display counter time 
   if  (((long) LT_current_time - (long) LTstart_time) >= LTmax_time){ 
 LT= false;
 }
 
 // If the lightning timer has timed out lets reset the display and clear the stastics 
 if (LT == false){
 draw_2(); // Gives all clear display
 lightning.clearStatistics(1); //Clear the statistics
 make_sound (harp); //Make all clear sound
 }
 }
  //Check to see if its time to turn off disturber indication
  // Gotta use long prefex or the math doesn't work
  if ((DT ==true)){               
      if (((long) dist_current_time - (long) diststart_time) >= distmax_time) 
  {   
      DT = false;      
      draw_5(); //Blank the display
  }
   
  }
  
   }

//End of the loop

void display_meter(void){
 // Serial.println( "reached display meter");
   tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK); //Blank the screen 
  delay (1000);
    tft.setCursor(0,0);   // Set coursor to the top left for the title
  tft.setTextSize(2);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_YELLOW);
  tft.println("Lightning Detector  "); // Display the title
  delay (1000);
}
void Range_Select(void){

   
    tft.setCursor(160,220);   // Set coursor to the lower right  Range 
  tft.setTextSize(1);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("Range = "); // Display the title
  //Blank the setting
 /* tft.setCursor (210,LINE7);
  tft.setTextSize(1);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_BLACK);
  tft.print("       ");*/
  //Print the setting 
  tft.setCursor(210,220);
  tft.setTextSize(1);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_YELLOW,ILI9341_BLACK);
  if( enviVal == INDOOR )
    tft.println("Indoor");  
  else if( enviVal == OUTDOOR )
    tft.println("Outdoor"); 
  
  
  delay (10);
}

// This function draws the all clear information
  void draw_2(void) {
 // Serial.println ("draw2 just executed");//trouble shooting 
 // delay (10) ;
 display_meter();
 Range_Select();
 
 tft.setCursor(0,LINE3); // set print position
 tft.fillRoundRect(0,LINE3,250,50,10, ILI9341_BLACK ); //erase line 3 previous value and any lightning info
 delay (10);
 tft.setCursor(10,LINE3+7); // set print position
tft.setTextColor(ILI9341_GREEN);
tft.setTextSize(4);  
tft.println("All Clear"  );
tft.drawRoundRect(0, LINE3, 250, 50, 10, ILI9341_GREEN);     // draws frame with rounded edges
  delay (10);
 }

void draw_3(void) {

// Lightning interrupt was  detected read the IntVal registers and update the display 

   
    
     
 
      Serial.println("Lightning Strike Detected!"); 
      delay (10);  
      tft.setCursor(0,LINE3); // set print position
 tft.fillRoundRect(0,LINE3,250,200,10, ILI9341_BLACK ); //erase bottom of the display
 delay (10);
 tft.setCursor(10,LINE3); // set print position
tft.setTextColor(ILI9341_RED);
tft.setTextSize(4);  
tft.println("Lightning"  );
tft.println(" Detected"  );
tft.drawRoundRect(0, LINE3, 250, 70, 10, ILI9341_RED);     // draws frame with rounded edges
  delay (10);
      // Lightning! Now how far away is it? Distance estimation takes into
      // account previously seen events. 
      int distance = lightning.distanceToStorm(); 
      Serial.print("Approximately: "); 
      delay (10);  
  Serial.print(distance); 
      delay (10);  
      Serial.println("km away!"); 
      delay (10);  
  tft.setTextSize(2);    
   tft.setCursor(10,LINE5); // set print position 
   tft.fillRect(0,LINE5,320,40,ILI9341_BLACK ); //erase previous distance  
      tft.print("Distance     "); 
      delay (10);  
      byte mi = distance *.62;
      tft.print(mi );
      delay (10);
      tft.println("  mi away!"); 
      delay (10);  

      // "Lightning Energy" and I do place into quotes intentionally, is a pure
      // number that does not have any physical meaning. 
      
      long lightEnergy = lightning.lightningEnergy(); 
      Serial.print("Lightning Energy: "); 
      delay (10);  
      Serial.println(lightEnergy); 
      delay (10);
tft.setCursor(0,LINE6); // set print position   
      tft.fillRect(0,LINE6,320,40,ILI9341_BLACK ); //erase previous count 
      tft.print("Lightning Energy: "); 
      delay (10);  
      tft.println(lightEnergy);
      
      delay (10);        


  
  }
  //Displays time since last lightning strike
  void draw_4(void){
    
  tft.setCursor(0,LINE7); // set print position   
     //Erase line 7 (Timer)
 tft.fillRect(0,LINE7,320,40,ILI9341_BLACK ); //erase previous count 
  delay (10); 
       tft.setTextColor(ILI9341_GREEN);
      tft.print("Time since last strike     "); 
      delay (10);  
      
      /*
      tft.print ( ( (  (long) LT_current_time  - (long) LTstart_time)/1000)    ) ;
      tft.println(" Sec"); 
      delay (10);  
*/
  
  }
// Blank disturber line
void draw_5(void){
tft.setCursor(10,LINE2);   // Set cursor to the top left for the title
  tft.setTextSize(2);   // Set text size to normal for the ILI9341
  tft.setTextColor(ILI9341_BLACK);
  tft.println("Disturber  "); //Blank it
}
//Update the timer count 
void draw_6(void) {

tft.setCursor(20,LINE8); // set print position  
tft.setTextColor(ILI9341_GREEN,BLACK);
long totalSeconds= ( ( (  (long) LT_current_time  - (long) LTstart_time)/1000)    ) ;
int minutes = totalSeconds / 60; //Get minutes
int seconds = totalSeconds % 60; //Get remaining seconds 

    tft.print(minutes);
    tft.print ("  min  ");
    tft.print (seconds) ;
      tft.println(" sec ");

      delay (10);  

}
void make_sound(byte P){
byte nsound = P;
digitalWrite(nsound , LOW); // sets the pin low
  delay(250);            // waits for a 200 milli second, any less time and the FX boards stops triggering 
  digitalWrite(nsound , HIGH);  // sets the digital  off
  delay(10);            // waits for a second
//Serial.print("making sound."); //test to see if sound is being commanded 
//Serial.println(P); 
}