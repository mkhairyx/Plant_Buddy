///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Note to reviewers: I can't imrpove this code as I do not have the hardware available, this is the best I can do.  //
// I'm willing to utilize the flow meter to measure the water in litres and stop at a specified value,               //
// but in order to do so, I need the module physically available in order to calibrate it with known amounts of      //
// water. Also, I will a function to check if there's no water flow while the pump is running                        //
// which alerts the user that there's no water available, and stops working until told to continue.                  //
// In addition to that, I will be using deep sleep, external interupts (in case user sends a message inquiring       //
// about the device's status), and timer based interupts/wake up. (to water the plant)                               //
// All of this is yet to come in the Plant Buddy V2 firmware.                                                        //
// Thank you for taking the time to review this.                                                                     //
// https://github.com/mkhairyx                                                                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Preferences.h>

Preferences prefs;

String number = "+xxxxxxxxxxxx"; // Owner's phone number

#define Battery 33 // Use your specific ADC GPIO pin
#define soil 32
#define pump 5
#define period 20 // 20 days (max 30 days)
#define wet 1000
#define dry 2000
 

unsigned long water_interval; // days in ms
int water_time = 5000; // for now, I will be watering the plant for 5 seconds. In V2, I'll use the flow sensor.
int BATP; // battery percentage
int humidity;
unsigned long days;

/*
gpio 33 for battery voltage - no pinMode() needed as it's attached to the adc
gpio 32 for soil readings - no pinMode() needed as analogread() is used
gpio 5 for pump motor - pinMode(pump, OUTPUT);
gpio 3 for gsm_tx -
gpio 1 for gsm_rx -
gpio 13 for flow 
gpio 25 for ring ping 
*/



void setup() 
{
  pinMode(pump, OUTPUT);
  digitalWrite(pump, LOW);
  prefs.begin("periodic", false);
  if (prefs.getInt("cause", 2) == 2)
  {
    delay(30000); //delay for 30 seconds to make sure the module boots up
  }
  Serial.begin(9600);
  analogSetPinAttenuation(Battery, ADC_6db);
  analogSetWidth(12);
  BATP = constrain(((analogRead(Battery)*100)/3908), 0, 100); // battery percentages
  humidity = constrain(map(analogRead(soil), dry, wet, 0, 100), 0, 100);
  days = period;
  // Send message with values:
  Serial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  Serial.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
  delay(500);
  Serial.println("The device just booted up!");
  Serial.print("The battery is at ");
  Serial.println(BATP);
  Serial.println("%");
  Serial.print("The soil humidity is ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("The period between each time the plant is watered is ");
  Serial.print(days);
  Serial.println(" days.");
  Serial.print("The boot cause was ");
  switch (prefs.getInt("cause", 2)) {
    case 0: // faulty restart
        Serial.println("an unknown fault...");
        break;
    case 1: // on purpose restart.
        Serial.println("new cycle started (just finished watering!).");
        Serial.println("Watering again in ");
        Serial.print(days);
        Serial.println(" days.");
        break;
    case 2:
        Serial.println("First boot!");
        break;
    default:
        Serial.println("N/A. Please contact the firmware provider.");
        break;
  }
  Serial.print((char)26); // SEND MESSAGE (ASCII FOR CTRL+Z)
  if (prefs.getInt("cause", 2) == 2) // First boot!
  {
    digitalWrite(pump, HIGH);
    delay(water_time);
    digitalWrite(pump, LOW);
  }
  prefs.putInt("cause", 0); // if anything happens (as an undervoltage or system shutdown during the delay, the boot/restart cause will be an unknown fault, ie case 0)
  prefs.end();
  water_interval = days * 86400000;  // = 24 hours * 60 mins * 60 secs * 1000 ms
}

void loop() 
{               
  if (millis() >= water_interval) // using millis to allow multitasking in future version of this code. 
  {
    prefs.begin("periodic", false);
    prefs.putInt("cause", 1); // restarting to water the plant
    prefs.end();
    // int asap = millis();
    digitalWrite(pump, HIGH);
    delay(water_time);
    digitalWrite(pump, LOW);
    ESP.restart();
  }
  // future multi tasking stuff (like user calling for status), or maybe I will put the esp into deep sleep and use interupts.
}
