//----------------------------------------------------------------------
//------------          SETUP FOR VOLTAGE MONITORING        ------------
//----------------------------------------------------------------------

#define VBATPIN A7

float measuredvbat; // variable for batter voltage

float measureVoltsFunc(){
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  // Serial.print("VBat: " ); Serial.println(measuredvbat);
  return measuredvbat;
}

//----------------------------------------------------------------------
//------------     SETUP FOR RTC                            ------------
//----------------------------------------------------------------------
#include <RTCZero.h>

/* Create an rtc object */
RTCZero intRTC;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 15;

/* Change these values to set the current initial date */
const byte day = 23;
const byte month = 9;
const byte year = 20;

#define GREEN 8 // Green LED on Pin #8
#define SampleIntSec 5 // RTC - Sample interval in seconds
#define SampleIntMin 5 // RTC - Sample interval in minutes // Changed to 1 min for testing

int NextAlarmSec; // variable to hold the next alarm time in seconds
int NextAlarmMin; // variable to hold the next alarm time in minutes
const int SampleIntSeconds = 500;   //Simple Delay used for testing, ms i.e. 1000 = 1
bool matched = false;

//Setup external RTC
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS3231 extRTC;

//----------------------------------------------------------------------
//------------     SETUP FOR TEMPERATURE SENSOR             ------------
//----------------------------------------------------------------------

#include <OneWire.h>
#include <DallasTemperature.h>
const int oneWireBus = 10
;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//----------------------------------------------------------------------
//------------     SETUP FOR DISTANCE SENSOR        ------------
//----------------------------------------------------------------------

#include <NewPing.h>


// Setup pins for ultrasonic
#define trigPin 5 // 
#define echoPin 6  // 
#define maxDist 400 // maximum measureable distance of the ultrasonic sensor
NewPing sonar(trigPin, echoPin, maxDist); // tell library setup pins and max distance
int iterations = 10; // number of distance measurements to take before selecting the median



//----------------------------------------------------------------------
//------------          SETUP FOR SD CARD                   ------------
//----------------------------------------------------------------------

#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 4;

void dataWriteFunc(float measuredvbat, float duration, float distance, float temp){
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  //Serial.println("card initialized.");
  
  // Open file on SD card
  File testDist = SD.open("dist.txt",FILE_WRITE);
  // If the file opened, append the latest measurment to the file & send data to serial monitor
  if(testDist) {   
    DateTime now = extRTC.now();
    
    String dataString = "";
    dataString += String(now.day(), DEC)+"/"+String(now.month(), DEC)+"/"+String(now.year(),DEC)+" "+String(now.hour(),DEC)+":"+String(now.minute(), DEC)+":"+String(now.second(), DEC);
    dataString += String(",");
    dataString +=String(measuredvbat);
    dataString += ",";
    dataString += String(duration);
    dataString += ",";
    dataString += String(distance);
    dataString += ",";
    dataString += String(temp);
    testDist.println(dataString);
    testDist.close();
    Serial.println(dataString);
     }
     else{
      Serial.print("Error opening file");
     }
}

void setup()
{
  Serial.begin(9600);

  //delay(1500);

  // Check external RTC has time running
    if (! extRTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initiate internal RTC
  intRTC.begin(); // initialize RTC

  // set the time on the internal RTC
  intRTC.setTime(hours, minutes, seconds);
  intRTC.setDate(day, month, year);

  intRTC.setAlarmTime(15, 01, 00);  // set the alarm trigger in 5 seconds
  intRTC.enableAlarm(intRTC.MATCH_SS);

  intRTC.attachInterrupt(alarmMatch);

  intRTC.standbyMode();
}

void loop()
{
  if(matched){
  matched = false;

  // blink LED used for testing code. Comment out when flashing for installation.
  digitalWrite(GREEN, HIGH);
  delay(500);
  digitalWrite(GREEN, LOW);
  delay(500);

  // Measure the voltage and write the data to the SD card.
  measuredvbat = measureVoltsFunc();
  float duration = sonar.ping_median(iterations);
  float distance = (duration*0.0343)/2;
  //measure temperature
  sensors.begin();
  pinMode(oneWireBus,OUTPUT);
  sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0);
  // write the measured volatage to the sd card
  dataWriteFunc(measuredvbat, duration, distance, temp);
  ///////// Interval Timing and Sleep Code ////////////////
  //delay(SampleIntSeconds);   // Simple delay for testing only interval set by const in header

  NextAlarmMin = (intRTC.getAlarmMinutes() + SampleIntMin) % 60;   // i.e. 65 becomes 5
  intRTC.setAlarmMinutes(NextAlarmMin); // RTC time to wake, currently seconds only
  intRTC.enableAlarm(intRTC.MATCH_MMSS); // match minutes and seconds
  intRTC.attachInterrupt(alarmMatch);
  //intRTC.enableAlarm(intRTC.MATCH_MMSS); // Match seconds only
  //intRTC.attachInterrupt(alarmMatch); // Attaches function to be called, currently blank
  //delay(50); // Brief delay prior to sleeping not really sure its required
  
  intRTC.standbyMode();    // Sleep until next alarm match
  }
}


void alarmMatch() // Do something when interrupt called
{
matched = true;
}
