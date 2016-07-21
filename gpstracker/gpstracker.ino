#include <Arduino.h>

/*
 * 
 * GPS Tracker for Arduino, first Adventure Labs project
 * Author: Tom Broughton
 * License: Creative Commons - share and share alike
 * 
 * A GPS tracker that trackes every few seconds and allows 
 * waypoints to be taken.  Data written to SD card and status 
 * displayed on an RGB LCD.
 */

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

//set up lcd

#include <Wire.h>
#include "rgb_lcd.h"


rgb_lcd lcd;

// set up SD

#include <SPI.h>
#include <SD.h>


// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;

File dataFile;

// end SD

//update delay
unsigned long lastdisplaytime = 0UL;
const long updatedelay = 15000;

//update waypoint button delay
unsigned long last_button_time = 0UL;
//update button delay

//update lcd last time pressed
unsigned long lcdbuttonlastpressed = 0UL;

/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const int RXPin = 5, TXPin = 6;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

//setup pointtypeto record
char pointtype = 'T';
int waypointcounter = 0;

const byte waypointbutton = 2;


//settings for turning LCD Off/On
boolean lcdon = true;
int lcdbuttonstate = LOW;
int debouncetime = 0;
int debouncelength = 50;
int lcdbutton = 7;



void setup()
{
  ss.begin(GPSBaud);

   // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
    
  lcd.setRGB(0, 0, 255);
  lcd.setCursor(0, 0);

  
  lcd.print(F("Init SD card..."));
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  delay(1000);

  
  pinMode(10, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    displayLCDError(F("Card failed"));
    // don't do anything more:
    while (1) ;
  }
  lcd.clear();
  lcd.print(F("card initialised"));
  delay(1000);

  // Open up the file we're going to log to!
 
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (!dataFile) {
    displayLCDError(F("err opening file"));
    // Wait forever since we cant write data
    while (1) ;
  }else{
    dataFile.println(F("-------------"));
    dataFile.println(F("type, latitude, longitude, alt, speed, course, date, time, satellite, name, description"));
  }
 
  //make sure we interrupt on button press
  
  pinMode(waypointbutton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(waypointbutton), setWaypoint, CHANGE);

  
  pinMode(lcdbutton, INPUT);
}

void loop()
{

  //check to see if lcd should be turned off
  
  int lcdbuttonreading = digitalRead(lcdbutton);
  if(lcdbuttonreading != lcdbuttonstate){
    debouncetime = millis();
  }
  
  if((millis() - debouncetime) > debouncelength){
    if(lcdbuttonreading != lcdbuttonstate){
      lcdbuttonstate = lcdbuttonreading;
  
      if(lcdbuttonstate == HIGH){
        lcdon = !lcdon;

        if(lcdon){
          turnOnLCD();
        }else{
            turnOffLCD();
          }
      }
    }
  }


  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
    if (gps.encode(ss.read())){
      if(millis() - lastdisplaytime > updatedelay){
        if(gps.location.isValid()){
          if(lcdon) 
            displayLCDInfo();
          saveToSD();
        }else{
          displayLCDError(F("Getting Loc..."));
        }
        lastdisplaytime = millis();
      }
    }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }

  if(pointtype == 'W'){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("waypoint "));
    lcd.print(waypointcounter);
    lcd.print(F(" set"));
    
    pointtype = 'T';
    
  }
}

void turnOnLCD(){
  lcd.display();
  lcd.setRGB(0, 0, 255);
}


void turnOffLCD(){
  lcd.noDisplay();
  lcd.setRGB(0, 0, 0);
}


void displayLCDInfo()
{
  lcd.clear();
  
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  
  //lcd.print(F("loc: ")); 
  if (gps.location.isValid())
  {
    
    lcd.setRGB(0, 255, 0);
    lcd.print(gps.location.lat(), 4);
    lcd.print(F(","));
    lcd.print(gps.location.lng(), 4);

    lcd.setCursor(0, 1);
    if (gps.time.isValid())
    {
      if (gps.time.hour() < 10) Serial.print(F("0"));
      lcd.print(gps.time.hour());
      lcd.print(F(":"));
      if (gps.time.minute() < 10) Serial.print(F("0"));
      lcd.print(gps.time.minute());
      lcd.print(F(":"));
      if (gps.time.second() < 10) Serial.print(F("0"));
      lcd.print(gps.time.second());
    }
    else
    {
      lcd.print(F("I"));
    }
  
  }
  else
  {
    displayLCDError(F("Getting Location"));
  }
 

  //Serial.println();
}

void displayLCDError(String msg){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.setRGB(255, 0, 0);
  lcd.print(msg);
}

void saveToSD(){
  
  
  
   if(dataFile){
   
    dataFile.print(pointtype); 
    dataFile.print(F(",")); 
    
    if (gps.location.isValid())
    {
      dataFile.print(gps.location.lat(), 6);
      dataFile.print(F(","));
      dataFile.print(gps.location.lng(), 6);
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
    
    
    dataFile.print(F(",")); 
    if (gps.altitude.isValid())
    {
      dataFile.print(gps.altitude.meters(), 6);
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
    
    dataFile.print(F(",")); 
    if (gps.speed.isValid())
    {
      dataFile.print(gps.speed.kmph(), 2);
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
    
    dataFile.print(F(",")); 
    if (gps.course.isValid())
    {
      dataFile.print(gps.course.deg(), 6);
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
  
    dataFile.print(F(","));
    if (gps.date.isValid())
    {
      dataFile.print(gps.date.year());
      dataFile.print(F("/"));
      dataFile.print(gps.date.month());
      dataFile.print(F("/"));
      dataFile.print(gps.date.day());
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
  
    dataFile.print(F(","));
    if (gps.time.isValid())
    {
      if (gps.time.hour() < 10) dataFile.print(F("0"));
      dataFile.print(gps.time.hour());
      dataFile.print(F(":"));
      if (gps.time.minute() < 10) dataFile.print(F("0"));
      dataFile.print(gps.time.minute());
      dataFile.print(F(":"));
      if (gps.time.second() < 10) dataFile.print(F("0"));
      dataFile.print(gps.time.second());
      dataFile.print(F("."));
      if (gps.time.centisecond() < 10) dataFile.print(F("0"));
      dataFile.print(gps.time.centisecond());
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
  
    dataFile.print(F(",")); 
    if (gps.satellites.isValid())
    {
      dataFile.print(gps.satellites.value());
    }
    else
    {
      dataFile.print(F("INVALID"));
    }
    
    //name (waypoint)
    dataFile.print(F(",")); 
    if (pointtype == 'W')
    {
      dataFile.print(F("Waypoint "));
      dataFile.print(waypointcounter);
    }
    else
    {
      dataFile.print(F(""));
    }
    
    //description (waypoint)
    dataFile.print(F(",")); 
    if (pointtype == 'W')
    {
      dataFile.print(F("<<AddDescription>>"));
    }
    else
    {
      dataFile.print(F(""));
    }
    dataFile.println();
    dataFile.flush();
  }

}

void setWaypoint(){

  //check to see if increment() was called in the last 500 milliseconds
  if (millis() - last_button_time > 500)
  {
    pointtype = 'W';
    waypointcounter++;
    saveToSD();
    last_button_time = millis();
    lastdisplaytime = millis();
  }
}
