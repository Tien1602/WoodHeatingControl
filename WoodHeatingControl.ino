// --- Include section: ---
#include <OneWire.h> // Needed for DS18B20 sensors
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MAX31865.h>


// --- Application setup: ---
#define SERIAL_DEBUG_OUTPUT 1 // For debug output on serial communication set this to 1.

#define TEMPERATURE_LEVEL_PUMP_ON_AT_HEAT_UP             62 // degC
#define TEMPERATURE_HYSTERESIS_PUMP_ON_TO_OFF_AT_HEAT_UP  1 // degC
#define TEMPERATURE_LEVEL_PUMP_OFF_AT_HEAT_UP  TEMPERATURE_LEVEL_PUMP_ON_AT_HEAT_UP - TEMPERATURE_HYSTERESIS_PUMP_ON_TO_OFF_AT_HEAT_UP

#define PIN_PUMP_LED 4
#define STATE_PUMP_LED_ON HIGH
#define STATE_PUMP_LED_OFF LOW

#define PIN_PUMP_RELAIS 5
#define STATE_PUMP_ON  HIGH // Relais is off, but pump is on! This is done for security reasons.
#define STATE_PUMP_OFF LOW // Relais is on, but pump is off! This is done for security reasons.
uint8_t pumpPowerInPercentage_m = 100;


// --- Display setup: ---
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


// --- MAX31865 temperature sensor setup:
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13); // Use software SPI: CS, DI, DO, CLK
#define RREF      430.0 // The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RNOMINAL  100.0 // The 'nominal' 0-degrees-C resistance of the sensor, 100.0 for PT100, 1000.0 for PT1000


// --- DS18B20 temperature sensor setup: ---
#define ONE_WIRE_BUS 3 // Data wire is plugged into port 3 of controller.
#define TEMPERATURE_PRECISION 12 // The precision of the conected sensors are set to 12 bit.
OneWire oneWire(ONE_WIRE_BUS); // Setup the one wire bus.
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature driver.
float tempExhaust_m = 0;

// Definition of DS18B20 temperature sensor addresses:
DeviceAddress sensAddrHeatExchangerWaterTemp      = {0x28, 0x53, 0x0B, 0x79, 0xA2, 0x19, 0x03, 0x27};
DeviceAddress sensAddrHeatExchangerOutsideTopTemp = {0x28, 0xD7, 0xAA, 0xF5, 0x07, 0x00, 0x00, 0x5B};
DeviceAddress sensAddrHeatExchangerOutsideMidTemp = {0x28, 0x4F, 0xD7, 0x79, 0x97, 0x07, 0x03, 0xA5};
DeviceAddress sensAddrHeatExchangerOutsideBotTemp = {0x28, 0x6C, 0xAC, 0x79, 0x97, 0x07, 0x03, 0x57};
DeviceAddress sensAddrWaterInput1Temp             = {0x28, 0xFF, 0xAA, 0x6A, 0x55, 0x16, 0x03, 0xA7}; // TempSens02, left one, near oven, installed at 09.02.2019.
DeviceAddress sensAddrWaterInput2Temp             = {0x28, 0xFF, 0xEE, 0xC1, 0x58, 0x16, 0x04, 0xFF}; // TempSens01, right one, further away from the oven, installed at 09.02.2019.
DeviceAddress sensAddrWaterOutput1Temp            = {0x28, 0xFF, 0x64, 0x81, 0x55, 0x16, 0x03, 0xFA}; // TempSens03, left one , near oven, installed at 09.02.2019.
DeviceAddress sensAddrWaterOutput2Temp            = {0x28, 0xFF, 0xB2, 0xD5, 0x55, 0x16, 0x03, 0xEE}; // TempSens04, right one , further away from the oven, installed at 09.02.2019.
DeviceAddress sensAddrIntakeAirTemp               = {0x28, 0x89, 0xD3, 0x79, 0x97, 0x07, 0x03, 0x97};
float tempHeatExchangerWater_m = 0;
float tempHeatExchangerOutsideTop_m = 0;
float tempHeatExchangerOutsideMid_m = 0;
float tempHeatExchangerOutsideBot_m = 0;
float tempWaterInput1_m = 0;
float tempWaterInput2_m = 0;
float tempWaterOutput1_m = 0;
float tempWaterOutput2_m = 0;
float tempIntakeAir_m = 0;


/// ==================================================================================================
/// @brief Arduino setup method.
///
/// ==================================================================================================
void setup()
{
  // Init serial comunication:
  Serial.begin(115200);

  // Init PumpLED and PumpRelais output:
  pinMode(PIN_PUMP_LED, OUTPUT);
  pinMode(PIN_PUMP_RELAIS, OUTPUT);

  // Switch Pump on:
  // This is done for safety purpose!
  setPumpPower(100);

  // Init MAX31865 library:
  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary

  // Init DS18B20 library:
  sensors.begin();
  sensors.setResolution(sensAddrHeatExchangerWaterTemp, 12);
  sensors.setResolution(sensAddrHeatExchangerOutsideTopTemp, 12);
  sensors.setResolution(sensAddrHeatExchangerOutsideMidTemp, 12);
  sensors.setResolution(sensAddrHeatExchangerOutsideBotTemp, 12);  
  sensors.setResolution(sensAddrWaterInput1Temp, 12);
  sensors.setResolution(sensAddrWaterInput2Temp, 12);
  sensors.setResolution(sensAddrWaterOutput1Temp, 12);
  sensors.setResolution(sensAddrWaterOutput2Temp, 12);
  sensors.setResolution(sensAddrIntakeAirTemp, 12);

  // Init display:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize the display with the I2C addr 0x3C and 3.3V internal.  
  display.setTextSize(1); // Set text size.
  display.setTextColor(WHITE); // Set text color.
  display.clearDisplay(); // Clear the display buffer.
  display.setCursor(0, 0); // Set the cursor position.
  display.println("====================");
  display.println(" WoodHeatingControl ");
  display.println("       v0.1.2       ");
  display.println("====================");
  display.display();

  // Wait a little bit:
  delay(1000);
    
  // Check the OneWire DS18B20 temperature sensors:
  if (checkOneWireTempSensors() == true)
  {
    display.clearDisplay(); // Clear the display buffer.
    display.setCursor(0, 0); // Set the cursor position.
    display.println("Error: TempSens");
    display.display();
    for (;;) {} // Endless loop! Let the pump running! Find and solve the problem!
  }

  // Wait a little bit:
  delay(1000);
}


/// ==================================================================================================
/// @brief Arduino loop method.
///
/// ==================================================================================================
void loop()
{
  sampleTemperatures();

  if (getTempOfHeatExchangerWater() == -127.0) // ERROR case: catch it in future at the sampling level!
  {
    setPumpPower(100);
  }
  else if (getTempOfHeatExchangerWater() >= TEMPERATURE_LEVEL_PUMP_ON_AT_HEAT_UP)
  {
    setPumpPower(100);
  }
  else if (getTempOfHeatExchangerWater() < TEMPERATURE_LEVEL_PUMP_OFF_AT_HEAT_UP)
  {
    setPumpPower(0);
  }
  else
  {
    // change nothing
  }

  outputDataToDisplay();
  //outputDataToSerialMonitor();
  outputDataToSerialPlotter();

  delay(200);
}


/// ==================================================================================================
/// @brief This method prints a DS18B20 device address to the serial communication.
///
/// ==================================================================================================
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


/// ==================================================================================================
/// @brief This method samples all temperatures of the DS18B20 sensors.
///
/// ==================================================================================================
void sampleTemperatures()
{
  // DS18B20 temperatures:
  sensors.requestTemperatures();
  tempHeatExchangerWater_m = sensors.getTempC(sensAddrHeatExchangerWaterTemp);
  tempHeatExchangerOutsideTop_m = sensors.getTempC(sensAddrHeatExchangerOutsideTopTemp);
  tempHeatExchangerOutsideMid_m = sensors.getTempC(sensAddrHeatExchangerOutsideMidTemp);
  tempHeatExchangerOutsideBot_m = sensors.getTempC(sensAddrHeatExchangerOutsideBotTemp);
  tempWaterInput1_m = sensors.getTempC(sensAddrWaterInput1Temp);
  tempWaterInput2_m = sensors.getTempC(sensAddrWaterInput2Temp);
  tempWaterOutput1_m = sensors.getTempC(sensAddrWaterOutput1Temp);
  tempWaterOutput2_m = sensors.getTempC(sensAddrWaterOutput2Temp);
  tempIntakeAir_m = sensors.getTempC(sensAddrIntakeAirTemp);

  // PT1000 temperatures:
  tempExhaust_m = thermo.temperature(RNOMINAL, RREF);

  // TODO: Return error codes in this method!
  // Check and print any faults
  //  uint8_t fault = thermo.readFault();
  //  if (fault) {
  //    Serial.print("Fault 0x"); Serial.println(fault, HEX);
  //    if (fault & MAX31865_FAULT_HIGHTHRESH) {
  //      Serial.println("RTD High Threshold"); 
  //    }
  //    if (fault & MAX31865_FAULT_LOWTHRESH) {
  //      Serial.println("RTD Low Threshold"); 
  //    }
  //    if (fault & MAX31865_FAULT_REFINLOW) {
  //      Serial.println("REFIN- > 0.85 x Bias"); 
  //    }
  //    if (fault & MAX31865_FAULT_REFINHIGH) {
  //      Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
  //    }
  //    if (fault & MAX31865_FAULT_RTDINLOW) {
  //      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
  //    }
  //    if (fault & MAX31865_FAULT_OVUV) {
  //      Serial.println("Under/Over voltage"); 
  //    }
  //    thermo.clearFault();
  //  }
  //  Serial.println();
}


/// ==================================================================================================
/// @brief This method returns the temperatures of HeatExchangerWater.
///
/// ==================================================================================================
float getTempOfHeatExchangerWater()
{
  return tempHeatExchangerWater_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of HeatExchangerWater.
///
/// ==================================================================================================
float getTempOfHeatExchangerOutsideTop()
{
  return tempHeatExchangerOutsideTop_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of HeatExchangerWater.
///
/// ==================================================================================================
float getTempOfHeatExchangerOutsideMid()
{
  return tempHeatExchangerOutsideMid_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of HeatExchangerWater.
///
/// ==================================================================================================
float getTempOfHeatExchangerOutsideBot()
{
  return tempHeatExchangerOutsideBot_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of WaterInput1.
///
/// ==================================================================================================
float getTempOfWaterInput1()
{
  return tempWaterInput1_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of WaterInput2.
///
/// ==================================================================================================
float getTempOfWaterInput2()
{
  return tempWaterInput2_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of WaterOutput1.
///
/// ==================================================================================================
float getTempOfWaterOutput1()
{
  return tempWaterOutput1_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of WaterOutput2.
///
/// ==================================================================================================
float getTempOfWaterOutput2()
{
  return tempWaterOutput2_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of IntakeAir.
///
/// ==================================================================================================
float getTempOfIntakeAir()
{
  return tempIntakeAir_m;
}


/// ==================================================================================================
/// @brief This method returns the temperatures of Exhaust.
///
/// ==================================================================================================
float getTempOfExhaust()
{
  return tempExhaust_m;
}


/// ==================================================================================================
/// @brief This method sets the power of the pump in percentage.
///
/// ==================================================================================================
void setPumpPower(uint8_t pumpPowerInPercentage)
{
  pumpPowerInPercentage_m = pumpPowerInPercentage;

  if (pumpPowerInPercentage == 0)
  {
    digitalWrite(PIN_PUMP_LED, STATE_PUMP_LED_OFF);
    digitalWrite(PIN_PUMP_RELAIS, STATE_PUMP_OFF);
  }
  else
  {
    digitalWrite(PIN_PUMP_LED, STATE_PUMP_LED_ON);
    digitalWrite(PIN_PUMP_RELAIS, STATE_PUMP_ON);
  }
}


/// ==================================================================================================
/// @brief This method returns the power of the pump in percentage.
///
/// ==================================================================================================
uint8_t getPumpPower()
{
  return pumpPowerInPercentage_m;
}


/// ==================================================================================================
/// @brief This method outputs curent data to the display.
///
/// ==================================================================================================
void outputDataToDisplay()
{
  // Clear the buffer.
  display.clearDisplay();

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.print("Ofen:      ");
  display.print(getTempOfHeatExchangerWater());
  display.print(" ");
  display.write(0xF7);
  display.println("C");

  display.print("Zulauf:    ");
  display.print(getTempOfWaterInput1());
  display.print(" ");
  display.write(0xF7);
  display.println("C");

  display.print("Rucklauf:  ");
  display.print(getTempOfWaterOutput1());
  display.print(" ");
  display.write(0xF7);
  display.println("C"); 

  display.print("Abgas:    ");
  display.print(getTempOfExhaust());
  display.print(" ");
  display.write(0xF7);
  display.println("C"); 
  
  display.display();
}


/// ==================================================================================================
/// @brief This method outputs the curent data to the RS232 serial communication.
///
/// ==================================================================================================
void outputDataToSerialMonitor()
{
  Serial.print("Ofen innen:  ");
  Serial.print(getTempOfHeatExchangerWater());
  Serial.println(" °C");

  Serial.print("Ofen oben:   ");
  Serial.print(getTempOfHeatExchangerOutsideTop());
  Serial.println(" °C");

  Serial.print("Ofen mitte:  ");
  Serial.print(getTempOfHeatExchangerOutsideMid());
  Serial.println(" °C");
  
  Serial.print("Ofen unten:  ");
  Serial.print(getTempOfHeatExchangerOutsideBot());
  Serial.println(" °C");
  
  Serial.print("Vorlauf 1:   ");
  Serial.print(getTempOfWaterInput1());
  Serial.println(" °C");

  Serial.print("Vorlauf 2:   ");
  Serial.print(getTempOfWaterInput2());
  Serial.println(" °C");
  
  Serial.print("Rücklauf 1:  ");
  Serial.print(getTempOfWaterOutput1());
  Serial.println(" °C");
  
  Serial.print("Rücklauf 2:  ");
  Serial.print(getTempOfWaterOutput2());
  Serial.println(" °C");
    
  Serial.print("Abgas:      ");
  Serial.print(getTempOfExhaust());
  Serial.println(" °C");
      
  Serial.print("Einlassluft: ");
  Serial.print(getTempOfIntakeAir());
  Serial.println(" °C");

  Serial.print("Pumpe: ");
  Serial.print(getPumpPower());
  Serial.println(" %");

  Serial.println("");
}


/// ==================================================================================================
/// @brief This method outputs the curent data to the RS232 serial communication for plotter.
///
/// ==================================================================================================
void outputDataToSerialPlotter()
{
  Serial.print(getTempOfHeatExchangerWater());
  Serial.print(",");

  Serial.print(getTempOfHeatExchangerOutsideTop());
  Serial.print(",");

  //Serial.print(getTempOfHeatExchangerOutsideMid());
  //Serial.print("0.0");
  //Serial.print(",");
  
  //Serial.print(getTempOfHeatExchangerOutsideBot());
  //Serial.print(",");
  
  Serial.print(getTempOfWaterInput1());
  Serial.print(",");

  //Serial.print(getTempOfWaterInput2());
  //Serial.print(",");
  
  Serial.print(getTempOfWaterOutput1());
  Serial.print(",");
  
  //Serial.print(getTempOfWaterOutput2());
  //Serial.print(",");
    
  Serial.print(getTempOfExhaust());
  Serial.print(",");
      
  //Serial.print(getTempOfIntakeAir());
  //Serial.print(",");

  if (getPumpPower() == 100)
  {
     Serial.print("10,");
  }
  else
  {
    Serial.print("0,");
  }
  Serial.println("");
}


/// ==================================================================================================
/// @brief This method checks the OneWire DS18B20 temperature sensors.
///
/// ==================================================================================================
bool checkOneWireTempSensors()
{
  bool errorReturnValue = false;

  if (sensors.isConnected(sensAddrHeatExchangerWaterTemp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrHeatExchangerWaterTemp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrHeatExchangerOutsideTopTemp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrHeatExchangerOutsideTopTemp!");
    }
#endif
  }

//  if (sensors.isConnected(sensAddrHeatExchangerOutsideMidTemp) == false)
//  {
//    errorReturnValue = true;
//#if (SERIAL_DEBUG_OUTPUT == 1)
//    {
//      Serial.println("ERROR: No address for sensAddrHeatExchangerOutsideMidTemp!");
//    }
//#endif
//  }

  if (sensors.isConnected(sensAddrHeatExchangerOutsideBotTemp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrHeatExchangerOutsideBotTemp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrIntakeAirTemp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrIntakeAirTemp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrWaterInput1Temp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrHeatExchangerWaterTemp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrWaterInput2Temp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrWaterInput1Temp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrWaterOutput1Temp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrWaterOutput1Temp!");
    }
#endif
  }

  if (sensors.isConnected(sensAddrWaterOutput2Temp) == false)
  {
    errorReturnValue = true;
#if (SERIAL_DEBUG_OUTPUT == 1)
    {
      Serial.println("ERROR: No address for sensAddrWaterOutput2Temp!");
    }
#endif
  }

  return errorReturnValue;

}
