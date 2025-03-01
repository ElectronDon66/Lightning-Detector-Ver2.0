Version 2.0 of AS 3935 Franklin Lightning Detector



Version 2.0 of the Lightning Detector is a continuation of “AS3935 Lightning Detector with TFT Display 3” which is posted on YouTube and GitHub. The original project was a protoboard for testing. In this version the circuitry was put in an aluminum enclosure, a larger 2.8” TFT display was added as well as sound to indicate the type of event detected. The detectable events are Lightning strike, electrical disturber which may or may not be lightning and an elapsed  timer sound and display indicating no lightning strikes have been detected for 20 min.  Typical disturber events are a static electricity spark near the detector. 
A Teensy 4.0 microcontroller handles the processing, communicating with the Sparkfun AS3935, the 2.8” 320x240 TFT display, and an Adafruit FX sound board.  The audio section which includes a discreet design amplifier is optional and can just be omitted if sound isn’t desired. The discreet  design amplifier was necessary because the cheap LM386 style amplifiers commonly available are prone to noise pickup from the TFT display updates. A youtube video is available at https://youtu.be/5M5g60FlnEE 

Circuitry description:
The TFT and  Teensy 4.0 use 3.3V. 5V controllers such as the Arduino nano and Mega caused detector interference. I ultimately chose the Teensy 4.0 as it is 3.3V and operates at such a high clock frequency (600 Mhz) that it didn’t really interfere with the AS3935’s 500kHz tank circuit band width. I also found that trying to use I2C for the AS-3935 was not reliable. Sometimes it worked some times not.  This caused a lot of wasted time (testing) so I finally ditched I2C and implemented the SPI interface.  I also wanted a fancy TFT color display for lightning  detection , range display , and storm strength indications. I also added a disturber detection function as I found some real lightning strikes get classified as a disturber probably because the strike waveform is outside of the AMS secret detection algorithm requirements. Initially adding the TFT display caused 500kHz interference to the detector when I was writing to the display. I also added a red LED on the detectors interrupt line so I could easily see when the interrupt line was indicating an event when none should be present. The SPI communication trick here was to isolate the Sparkfun AS 3935 SPI pins from the TFT SPI pins. This is accomplished with the 74HC244 buffer IC. The HC version is 3.3V compatible, so don’t use a 74HCT 244 or 74244 as it won’t work. I also found I had to move the Spark fun AS 3935 PCB away from the other components. Six  inches minimum separation seemed to be OK.   In the AMS data sheet they recommend keeping the AS-3935 away from switching power supplies and other electrical gear. This little AMS chip has a 500kHz tank circuit that is surprisingly sensitive to ANY local 500kHz noise. While doing my real lightning testing I was turning on my short wave receiver to also watch and hear 500kHz lightning signals (for strike verification). { A short wave receiver tuned to 500kHz will indicate a static crash sound during lightning strikes. ) Every time I turned on my SW receiver the lighting detector stopped working . I finally tracked it down to the switching power supply that runs the SW receiver. Even when placed 100 ft away from the detector it still blocked the lightning signals. However, the lightning detector would work fine up to about 2 ft away from my Dell desktop computer. 
I also found a coding error in the Sparkfun AS3935 library. That error will not let you ever change the AS3935 sensitivity to outdoors. So if you want outdoors mode to work you will need to fix SparkFun’s  AS3935.CPP code 
To fix the SparkFun_AS3935_Lightning_Detector_Arduino_Library look in the sparkfun library src folder and open the Sparkfun_AS3935.cpp file with MS notepad. Find the section :
// REG0x00, bits [5:1], manufacturer default: 10010 (INDOOR).
// This function changes toggles the chip's settings for Indoors and Outdoors.
void SparkFun_AS3935::setIndoorOutdoor(uint8_t _setting)
{
    if (((_setting != INDOOR) && (_setting != OUTDOOR)))
       return
--------------------------------------
Change the if statement line  to : if (((_setting != INDOOR) || (_setting != OUTDOOR)))
Save the file back to the library. 
In the metal enclosure build I placed the AS3935 lightning detector on top of the aluminum enclosure in a small plastic box. This provides additional electrostatic shielding from the microcontroller and the Adafruit FX sound board.  A thin slot cut in top of the aluminum chassis allowed a flat cable pass through.  I also chose a 5V linear supply to power the box. Switching supplies were found to be too noisy and blocked lightning detection. 5V is used for the Teensy 4.0 power input and also to power the analog sound amplifier and the Adafruit FX sound board. 
Analog circuitry:
When an event is detected the Teensy 4.0 toggles a digital output (one of 3) which trigger the Adafruit FX sound board. The event sounds are wave files (.wav) that drive the audio amplifier board. The WAV files need to be loaded into the FX board via its mini USB connector. A simple open drain signal from the Teensy 4.0 pulls down the trigger input (250ms) causing the sound to play.  I also had to add a (LC) low pass filter (hash filter) on the 5V line to the amplifier board to block Teensy 4.0 digital noise from bleeding into the  audio amplifier. My WAV sound files are also included on Github . Once the FX sound board triggers it cannot be retriggered until the sound file finishes. So I noted that if a disturber triggers the sound board and a second later a lightning strike is detected the sound board will not play the lightning event sound; however the display will show both events. 

A few more notes:
* For testing I set the ALLCLEAR timer to 100 sec (Line 98 100000) for 20 min set line 98 to 1200000
* While doing testing I found that The piezo electric candle lighters don’t work well they trigger a disturber event but not lightning.  You may very rarely get a simulated strike. 
* If you get the lighter too close to the AS 3935 IC you can destroy it. I had this happen and the failure mode is insidious. The IC still appears to work as you can read and write registers, but it  won’t catch real lightning as the receiver circuitry was what got destroyed. After a few real thunder storms passed I realized what happened as nothing was detected.  
* I bought and used a SEN-39002 lightning simulator. I had to modify it to get it to work decently. I had to remove the Inductor L2 and place it at the end of two wires about 17 inches away from the SEN-39002 . This is so the Arduino Uno running the SEN-39002 doesn’t bleed its digital noise into the Sparkfun detector. So I place the inductor about 2” (50mm)  away from the SparkFun detector and the Arduino as far away as possible for testing.  
* A lot of hobbyists have complained the AS 3935 doesn’t work. It does work fine, but you really need to work around the RFI issues which I have done. It was really exciting watching the multiple lightning strikes get detected from several big storms that I monitored this last summer. 

 

