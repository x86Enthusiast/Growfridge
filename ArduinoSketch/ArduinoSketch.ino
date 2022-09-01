//Copyright (C) 2022  Michel Macke
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <DHT_U.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Constants
#define DEFAULTTEMPERATURE 10
#define DEFAULTHUMIDITY 0.5
#define DEFAULTLIGHTHOURS 12

// LCD: 20x4 LCD with i2c module
#define LCDADDR 0x27 // Serial address of the LCD
#define LCDCHARS 20  // Number of chars per row for our LCD
#define LCDROWS 4    // Number of rows for our LCD

// DHT22: Temperature and Humidity Sensor
#define DHTPIN 4 // Pin the sensor is connected to

// Buttons: 4 buttons for navigation of the menu, setting temperature and humidity
// Each is connected to an interrupt pin
#define BUTTON1 18
#define BUTTON2 19
#define BUTTON3 2
#define BUTTON4 3

#define BUTTON1ACTIVESTATE LOW
#define BUTTON2ACTIVESTATE LOW
#define BUTTON3ACTIVESTATE LOW
#define BUTTON4ACTIVESTATE LOW

// Maximum and Minimum Values for all of the Configurable Values
#define MAXTEMPERATURE 50
#define MINTEMPERATURE 0
#define MAXHUMIDITY (float)1
#define MINHUMIDITY (float)0
#define MAXLIGHTHOURS 24
#define MINLIGHTHOURS 0

// time between dht22 measurements
#define MILLISBETWEENMEASUREMENTS 6000

// Relais Pins
#define FRIDGEPIN 31
#define HUMIDIFIERPIN 33
#define LIGHTPIN 35
#define FANPIN 37

#define UNSIGNEDLONGMAXVALUE 4294967295

// Menu Text
// Base Menu
#define BASEMENU 0
#define BASEMENUTEXT_L1 "Configuration Menu: "
#define BASEMENUTEXT_L2 "A: Temperature      "
#define BASEMENUTEXT_L3 "B: Humidity C: Light"
#define BASEMENUTEXT_L4 "D: Close this Menu  "
// Temperature Menu
#define TEMPERATUREMENU 1
#define TEMPMENUTEXT_L1 "Set Max Temperature "
#define TEMPMENUTEXT_L2 "T"
#define TEMPMENUTEXT_L3 "A for +|B For -     "
#define TEMPMENUTEXT_L4 "C for Set|D for End "
#define TEMPMENUTEXT_SET "Max Temp was set  "
// Humidity Menu
#define HUMIDITYMENU 2
#define HUMIDITYMENUTEXT_L1 "Set Max Humidity    "
#define HUMIDITYMENUTEXT_L2 ""
#define HUMIDITYMENUTEXT_L3 "A for +|B For -     "
#define HUMIDITYMENUTEXT_L4 "C for Set|D for End "
#define HUMIDITYMENUTEXT_SET "Max Humidity was set"
// Light Menu
#define LIGHTMENU 3
#define LIGHTMENUTEXT_L1 "Set Light hours/day "
#define LIGHTMENUTEXT_L2 ""
#define LIGHTMENUTEXT_L3 "A for +|B For -     "
#define LIGHTMENUTEXT_L4 "C for Set|D for End "
#define LIGHTMENUTEXT_SET "Light hours/day set "

// Global Variables
LiquidCrystal_I2C lcd(0x27, LCDCHARS, LCDROWS); // set the LCD address to 0x27 for a 20 chars and 4 line display
DHT dht(DHTPIN, DHT22);                         //

// Current temperature and humidity
float humidity = 0;    // Humidity as Percent
float temperature = 0; // Temperature as Degrees Celsius

// The time since last Temperature and Humidity Measurement (DHT22)
unsigned long timeSinceLastMeasurement = 0;

// The target values for the temperature, humidity and light
float temperatureTarget = 10.0;
float humidityTarget = 0.5;
int dailyLight = 0;

// The modified target values for the temperature, humidity and light
float newTemperatureTarget = 0.0;
float newHumidityTarget = 0.0;
int newDailyLight = 0;

// Used to compute light hours
unsigned long currentTime;

// How many hours has the light been on/off
int lightHoursPassed = 0;
// should the light be on or off
bool lightOn = true;

// minimum time between relais activations
unsigned long relaisProtectionTime = 15000;
// time of last relais activation
unsigned long timeSinceLastRelaisActication = 0;

// Variables used in the menu, get set with the buttons and interrupts
bool menuOn = false;
int menuState = 0;
bool incrementOne = false;
bool decrementOne = false;
bool applyValue = false;

// updates global temperature and humidity variables
void readDHT()
{

  // Since the sensor takes a while to get results, we wait a few ms between measurements
  // we do not use delay, as to keep the menu usable
  unsigned long timeBeforeRollover = 0;
  // If rollover occured, calculate the hours since last measurement with modified formula
  if (millis() < timeSinceLastMeasurement && timeSinceLastMeasurement != 0)
  {

   timeBeforeRollover = UNSIGNEDLONGMAXVALUE - timeSinceLastMeasurement;
    // Calculate  the amount of hours passed since the last measurement
    if  timeBeforeRollover + millis() >= MILLISBETWEENMEASUREMENTS)
    {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      timeSinceLastMeasurement = millis();
      // send to rpi
      Serial.println("Humidity: " + String(humidity));
      Serial.println("Temperature: " + String(temperature));
    }
  }
  else
  {
    if ((millis() - timeSinceLastMeasurement) >= MILLISBETWEENMEASUREMENTS)
    {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      timeSinceLastMeasurement = millis();
      // send to rpi
      Serial.println("Temperature: " + String(temperature) + "\n");
      Serial.println("Humidity: " + String(humidity) + "\n");
    }
  }
}

void handleTime()
{
  // Rollover, calculate time between now and when the rollover occured
  unsigned long timeBeforeRollover = 0;
  int hoursPassed = 0;
  // If rollover occured, calculate the hours since last measurement with modified formula
  if (millis() < currentTime)
  {
   timeBeforeRollover = UNSIGNEDLONGMAXVALUE - currentTime;
    // Calculate  the amount of hours passed since the last measurement
    hoursPassed = ( timeBeforeRollover + millis()) / 3600000);
  }
  else
  {
    // Calculate  the amount of hours passed since the last measurement
    hoursPassed = (millis() - currentTime) / 3600000;
  }
  // 1 hour passed, add that number to variable
  if (hoursPassed > 0)
  {
    currentTime = millis();
    lightHoursPassed++;
    // Send to rpi
    Serial.println("hoursPassed: " + String(hoursPassed));
  }
  // check if change in light should occur
  if (lightOn)
  {
    if (lightHoursPassed >= dailyLight)
    {
      lightOn = false;
    }
  }
  else
  {
    if (lightHoursPassed >= (24 - dailyLight))
    {
      lightOn = true;
    }
  }
}

void handleRelais()
{
  unsigned long compValue = millis() % relaisProtectionTime;
  if (compValue > timeSinceLastRelaisActication)
  {
    timeSinceLastRelaisActication = compValue;
  }
  else
  {
    timeSinceLastRelaisActication = 0;
    if (temperature > temperatureTarget)
    {
      digitalWrite(FRIDGEPIN, LOW);
    }
    else
    {
      digitalWrite(FRIDGEPIN, HIGH);
    }
    if (humidity < humidityTarget)
    {
      digitalWrite(HUMIDIFIERPIN, LOW);
      digitalWrite(FANPIN, LOW);
    }
    else
    {
      digitalWrite(HUMIDIFIERPIN, HIGH);
      digitalWrite(FANPIN, HIGH);
    }
    if (lightOn)
    {
      digitalWrite(LIGHTPIN, LOW);
    }
    else
    {
      digitalWrite(LIGHTPIN, HIGH);
    }
  }
}

void increment()
{
  incrementOne = true;
  delay(40);
}

void decrement()
{
  decrementOne = true;
  delay(40);
}

void apply()
{
  applyValue = true;
  delay(40);
}

void endMenu()
{
  menuOn = false;
  incrementOne = false;
  decrementOne = false;
  applyValue = false;
  // detach menu specific interrupts
  detachInterrupt(digitalPinToInterrupt(BUTTON1));
  detachInterrupt(digitalPinToInterrupt(BUTTON2));
  detachInterrupt(digitalPinToInterrupt(BUTTON3));
  detachInterrupt(digitalPinToInterrupt(BUTTON4));

  delay(40);
  // attach generic "open the menu" interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON1), menu, BUTTON1ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON3), menu, BUTTON2ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), menu, BUTTON4ACTIVESTATE);
}

void lightMenu()
{
  Serial.println("humMenu  - 1");

  lcd.setCursor(0, 0);
  lcd.print(LIGHTMENUTEXT_L1);
  lcd.setCursor(0, 1);
  lcd.print(String(newDailyLight) + " H");
  lcd.setCursor(0, 2);
  lcd.print(LIGHTMENUTEXT_L3);
  lcd.setCursor(0, 3);
  lcd.print(LIGHTMENUTEXT_L4);

  if (incrementOne)
  {
    if (newDailyLight + 1 < (int)MAXLIGHTHOURS)
    {
      newDailyLight += 1;
    }
    incrementOne = false;
    return;
  }
  if (decrementOne)
  {
    if (newDailyLight - 1 > (int)MINLIGHTHOURS)
    {
      newDailyLight -= 1;
    }
    decrementOne = false;
    return;
  }
  if (applyValue)
  {
    if (dailyLight != newDailyLight)
    {
      dailyLight = newDailyLight;
      Serial.println("lightTarget: " + String(dailyLight));
    }
    applyValue = false;
    menuState = BASEMENU;
    return;
  }
  return;
}

void tempMenu()
{
  lcd.setCursor(0, 0);
  lcd.print(TEMPMENUTEXT_L1);
  lcd.setCursor(0, 1);
  lcd.print(String(newTemperatureTarget) + " C");
  lcd.setCursor(0, 2);
  lcd.print(TEMPMENUTEXT_L3);
  lcd.setCursor(0, 3);
  lcd.print(TEMPMENUTEXT_L4);

  if (incrementOne)
  {
    if (newTemperatureTarget + 1 < MAXTEMPERATURE)
    {
      newTemperatureTarget += 1;
    }
    incrementOne = false;
    return;
  }
  if (decrementOne)
  {
    if (newTemperatureTarget - 1 > MINTEMPERATURE)
    {
      newTemperatureTarget -= 1;
    }
    decrementOne = false;
    return;
  }
  if (applyValue)
  {
    if (temperatureTarget != newTemperatureTarget)
    {
      lcd.clear();
      temperatureTarget = newTemperatureTarget;
      Serial.println("tempTarget: " + String(temperatureTarget));
    }
    applyValue = false;
    menuState = BASEMENU;
    return;
  }
  return;
}

void humMenu()
{
  Serial.println("humMenu  - 1");
  lcd.setCursor(0, 0);
  lcd.print(HUMIDITYMENUTEXT_L1);
  lcd.setCursor(0, 1);
  String printval = String(((int)newHumidityTarget) * 100);
  lcd.print( printval + "%");
  
  lcd.setCursor(0, 2);
  lcd.print(HUMIDITYMENUTEXT_L3);
  lcd.setCursor(0, 3);
  lcd.print(HUMIDITYMENUTEXT_L4);

  if (incrementOne)
  {
    if ((newHumidityTarget + (float)0.01 <= (float) MAXHUMIDITY))
    {
      newHumidityTarget += (float)0.01;
    }
    else
    {
      Serial.println("Could not raise, value is: ");
      Serial.println(String(newHumidityTarget));
      Serial.println("New value would be : ");
      Serial.println(String(newHumidityTarget + (float)0.01));
    }
    incrementOne = false;
    return;
  }
  if (decrementOne)
  {
    if ((newHumidityTarget - (float)0.01 >= (float)MINHUMIDITY))
    {
      newHumidityTarget -= (float)0.01;
    }
    decrementOne = false;
    return;
  }
  if (applyValue)
  {
    if (humidityTarget != newHumidityTarget)
    {
      humidityTarget = newHumidityTarget;
      // send to rpi
      Serial.println("humTarget: " + String(humidityTarget));
    }
    applyValue = false;
    menuState = BASEMENU;
    return;
  }
  return;
}

void baseMenu()
{
  lcd.setCursor(0, 0);
  lcd.print(BASEMENUTEXT_L1);
  lcd.setCursor(0, 1);
  lcd.print(BASEMENUTEXT_L2);
  lcd.setCursor(0, 2);
  lcd.print(BASEMENUTEXT_L3);
  lcd.setCursor(0, 3);
  lcd.print(BASEMENUTEXT_L4);

  if (incrementOne)
  {
    lcd.clear();
    menuState = TEMPERATUREMENU;
    newTemperatureTarget = temperatureTarget;
    incrementOne = false;
    decrementOne = false;
    applyValue = false;
    return;
  }
  if (decrementOne)
  {
    lcd.clear();
    menuState = HUMIDITYMENU;
    newHumidityTarget = humidityTarget;
    incrementOne = false;
    decrementOne = false;
    applyValue = false;
    return;
  }
  if (applyValue)
  {
    lcd.clear();
    menuState = LIGHTMENU;
    newDailyLight = dailyLight;
    incrementOne = false;
    decrementOne = false;
    applyValue = false;
    return;
  }
}

void detachAllInterrupts()
{
  detachInterrupt(digitalPinToInterrupt(BUTTON1));
  detachInterrupt(digitalPinToInterrupt(BUTTON2));
  detachInterrupt(digitalPinToInterrupt(BUTTON3));
  detachInterrupt(digitalPinToInterrupt(BUTTON4));
}

void menuFunction()
{
  menuState = 0;
  delay(40);
  attachInterrupt(digitalPinToInterrupt(BUTTON1), increment, BUTTON1ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), decrement, BUTTON2ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON3), apply, BUTTON3ACTIVESTATE);

  newTemperatureTarget = temperatureTarget;
  newHumidityTarget = humidityTarget;
  newDailyLight = dailyLight;

  // Button 4 is used to close the menu
  attachInterrupt(digitalPinToInterrupt(BUTTON4), endMenu, BUTTON4ACTIVESTATE);
  while (menuOn)
  {
    // Detach generic "open the menu" interrupt
    // Buttons 1 - 3 increment decrement and apply values

    readDHT();
    handleTime();
    handleRelais();
    switch (menuState)
    {
    case BASEMENU:
      baseMenu();
      break;
    case TEMPERATUREMENU:
      tempMenu();
      break;
    case HUMIDITYMENU:
      humMenu();
      break;
    case LIGHTMENU:
      lightMenu();
      break;
    default:
      break;
    }
  }
  lcd.clear();
}

// Menu to configure the temperatureTarget humidityTarget, dailyLight and currentTime.
void menu()
{
  detachAllInterrupts();
  menuOn = true;
}

void displayTempHum()
{
  lcd.setCursor(0, 0);
  lcd.print("Current Humidity:");
  lcd.setCursor(0, 1);
  lcd.print(String(humidity, 1) + "%");
  lcd.setCursor(0, 2);
  lcd.print("Current Temperature");
  lcd.setCursor(0, 3);
  lcd.print(String(temperature, 1) + " C");
}

void setup()
{
  Serial.begin(19200);
  Serial.println("started");
  // Initialize the LCD, turn on backlight, set cursor to 0,0
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  dht.begin();
  lcd.setCursor(0, 1);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  pinMode(BUTTON4, INPUT_PULLUP);

  // set pinmode for relay pins
  pinMode(FRIDGEPIN, OUTPUT);
  pinMode(HUMIDIFIERPIN, OUTPUT);
  pinMode(LIGHTPIN, OUTPUT);
  pinMode(FANPIN, OUTPUT);

  // Attach interrrupts to the pins of each button. Open menu when button is pressed
  // sleep to minimize input bleed from input button
  delay(10);

  attachInterrupt(digitalPinToInterrupt(BUTTON1), menu, BUTTON1ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), menu, BUTTON4ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON3), menu, BUTTON2ACTIVESTATE);
  attachInterrupt(digitalPinToInterrupt(BUTTON4), menu, BUTTON2ACTIVESTATE);


  temperatureTarget = DEFAULTTEMPERATURE;
  humidityTarget = DEFAULTHUMIDITY;
  dailyLight = DEFAULTLIGHTHOURS;
  currentTime = millis();
}

void loop()
{
  if (menuOn)
  {
    lcd.clear();
    menuFunction();
  }
  else
  {
    readDHT();
    handleTime();
    handleRelais();
    displayTempHum();
  }
}
