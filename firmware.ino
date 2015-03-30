#include <Servo.h> 
#include <OneWire.h>
#include "DallasTemperature.h"
#include <TM1638.h>
#include <EEPROM.h>

enum EEPROM_ENUM
  {
    EEPROM_DELAY,
		EEPROM_LOW_TEMPERATURE,
		EEPROM_LOW_TEMPERATURE_SIGN, // if true then negative
		EEPROM_HIGHT_TEMPERATURE,
		EEPROM_HIGHT_TEMPERATURE_SIGN, // if true then negative
		EEPROM_INVERSE,
		EEPROM_SERVO_LIMIT,
    EEPROM_THE_END
  };

enum MODES
	{
		MODE_AUTO,
		MODE_MANUAL,
		MODE_CFG_LOW_TEMP,
		MODE_CFG_HIGHT_TEMP,
		MODE_CFG_SERVO_LIMIT,
		MODE_THE_END
	};

byte mode = 0;
byte inverse = 0;
byte servo_limit = 0;

#define BTN_CHANGE_MODE 1
#define BTN_DEC 				2
#define BTN_INC 				4
#define BTN_INVERSE 		8
#define BTN_SET_LOW_TEMP 		16
#define BTN_SET_HIGTH_TEMP 		32
#define BTN_SET_SERVO_LIMIT 	64
#define BTN_SAVE 				128

#define PAUSE 1000
#define SMALLPAUSE 100

#define TICK_TIMER 1
#define READ_TEMPERATURE_TIMER 100 
byte temperature_timer;
#define CHANGE_SERVO_TIMER 15
byte change_servo_timer;
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
 
int pos = 0;    // variable to store the servo position 
int curr_pos = 0;
#define DELTA_POS 1
int hTemperature = 30;
int lTemperature = 0;

byte update_display = 0;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

// temperature
float temperature;

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
 

// define a module on data pin 8, clock pin 9 and strobe pin 7
TM1638 module(10, 11, 12);

byte loop_delay = 20;
 
void setup() 
{ 
  loop_delay = EEPROM.read(EEPROM_DELAY);
  lTemperature = EEPROM.read(EEPROM_LOW_TEMPERATURE);
	if(EEPROM.read(EEPROM_LOW_TEMPERATURE_SIGN))
		lTemperature *= -1;

  hTemperature = EEPROM.read(EEPROM_HIGHT_TEMPERATURE);
	if(EEPROM.read(EEPROM_HIGHT_TEMPERATURE_SIGN))
		hTemperature *= -1;

  inverse = EEPROM.read(EEPROM_INVERSE);
  servo_limit = EEPROM.read(EEPROM_SERVO_LIMIT);

	if (lTemperature == hTemperature)
		{
		 	hTemperature = 30;
		 	lTemperature = 0;
		}

	if (servo_limit > 179)
		servo_limit = 166;

	temperature_timer = 1;
	change_servo_timer = 5;

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 

  // start serial port
  Serial.begin(115200);
  Serial.println("generator gate controller");

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

  // Method 1:
  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  //if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices,
  // or you have already retrieved all of them.  It might be a good idea to
  // check the CRC to make sure you didn't get garbage.  The order is
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();
  
  module.setupDisplay(true,2);
  
  // display a hexadecimal number and set the left 4 dots
//  module.setDisplayToHexNumber(0x1234ABCD, 0xF0);
//  byte values[] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (int j=0; j<1; j++)
    for (int i=1; i<0xff; i=i<<1)
      {
        byte values[] = {i,i,i,i,i,i,i,i};
        module.setDisplay(values);
        delay(50);
      }
} 

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
//  Serial.print(" Temp F: ");
//  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  
  module.setDisplayToSignedDecNumber(tempC * 10, 0x02, false);
}

void loop() 
{ 
  byte keys = module.getButtons();

  if (temperature_timer)
    temperature_timer--;

  if (change_servo_timer)
    change_servo_timer--;

  if(!temperature_timer)
    {
      temperature_timer = READ_TEMPERATURE_TIMER;
      // call sensors.requestTemperatures() to issue a global temperature
      // request to all devices on the bus
    //  Serial.print("Requesting temperatures...");
      sensors.requestTemperatures(); // Send the command to get temperatures
    //  Serial.println("DONE");

      // It responds almost immediately. Let's print out the data
      temperature = sensors.getTempC(insideThermometer);
      
      update_display = 1;
      
      Serial.print("Temp C: ");
      Serial.println(temperature);
    }
    
	if (!change_servo_timer)
		{
      change_servo_timer = CHANGE_SERVO_TIMER;

			if (mode == MODE_AUTO)
      	pos = map(temperature, hTemperature, lTemperature, 0, servo_limit);
			else
				if (mode == MODE_MANUAL)
					{
						if (keys == BTN_DEC)
							pos--;
					
						if (keys == BTN_INC)
							pos++; 
					}

      if (pos < 0) pos = 0;
      if (pos > servo_limit) pos = servo_limit;

      if (curr_pos > pos)
        curr_pos -= DELTA_POS;
      else
        if (curr_pos < pos)
          curr_pos += DELTA_POS;

      if (curr_pos < 0) curr_pos = 0;
      if (curr_pos > servo_limit) curr_pos = servo_limit;

			if (inverse)
      	myservo.write(curr_pos);              // tell servo to go to position in variable 'pos'
			else
      	myservo.write(179 - curr_pos);              // tell servo to go to position in variable 'pos'
				
      
      update_display = 1;
  }
  
  // light the first 4 red LEDs and the last 4 green LEDs as the buttons are pressed
  module.setLEDs(keys);

  if (update_display && mode <= MODE_MANUAL)
    {
      update_display = 0;
      
      String ind_str;

			char tmp_str[7];
			dtostrf(temperature, 0, 1, tmp_str); 

      ind_str = String(tmp_str);
      ind_str += " ";
      ind_str += pos;
			ind_str += "  ";
      
//      module.clearDisplay();
      module.setDisplayToString(ind_str);
    }

	if (mode == MODE_CFG_LOW_TEMP)
		{
			if (keys == BTN_DEC)
				lTemperature--;
			
			if (keys == BTN_INC)
				lTemperature++;

			if (lTemperature < -30)
				lTemperature = -30;

			if (lTemperature > 60)
				lTemperature = 60;

      module.clearDisplay();
			String ind_str = String("lTmp ");
			ind_str += lTemperature;
      module.setDisplayToString(ind_str);
			delay(SMALLPAUSE);
		}

	if (mode == MODE_CFG_HIGHT_TEMP)
		{
			if (keys == BTN_DEC)
				hTemperature--;
			
			if (keys == BTN_INC)
				hTemperature++;

			if (lTemperature < -30)
				hTemperature = -30;

			if (lTemperature > 60)
				hTemperature = 60;

      module.clearDisplay();
			String ind_str = String("hTmp ");
			ind_str += hTemperature;
      module.setDisplayToString(ind_str);
			delay(SMALLPAUSE);
		}

	if (mode == MODE_CFG_SERVO_LIMIT)
		{
			if (keys == BTN_DEC)
				servo_limit--;
			
			if (keys == BTN_INC)
				servo_limit++;

			if (servo_limit < 10)
				servo_limit = 10;

			if (servo_limit > 179)
				servo_limit = 179;

      module.clearDisplay();
			String ind_str = String("SLIM ");
			ind_str += servo_limit;
      module.setDisplayToString(ind_str);
			delay(SMALLPAUSE);
		}


	if (keys == BTN_CHANGE_MODE)
		{
			if (mode != MODE_AUTO)
				{
					mode = MODE_AUTO;
      		module.clearDisplay();
      		module.setDisplayToString("MD: AUTO");
					delay(PAUSE);
      		module.clearDisplay();
				}
			else
				{
					mode = MODE_MANUAL;
      		module.clearDisplay();
      		module.setDisplayToString("MD: MAN");
					delay(PAUSE);
      		module.clearDisplay();
				}
		}

	if (keys == BTN_INVERSE)
		{
			if (inverse)
				{
					inverse = 0;
      		module.clearDisplay();
      		module.setDisplayToString("INV ON ");
					delay(PAUSE);
      		module.clearDisplay();
				}
			else
				{
					inverse = 1;
      		module.clearDisplay();
      		module.setDisplayToString("INV OFF ");
					delay(PAUSE);
      		module.clearDisplay();
				}
		}

	if (keys == BTN_SET_LOW_TEMP)
		if (mode != MODE_CFG_LOW_TEMP)
			{
				mode = MODE_CFG_LOW_TEMP;
				module.clearDisplay();
				module.setDisplayToString("low temp");
				delay(PAUSE);
				module.clearDisplay();

				String ind_str = String("lTmp ");
				ind_str += lTemperature;
				module.setDisplayToString(ind_str);
			}
		else
			{
				mode = MODE_AUTO;
				module.clearDisplay();
				module.setDisplayToString("AUTO");
				delay(PAUSE);
				module.clearDisplay();
			}

	if (keys == BTN_SET_HIGTH_TEMP)
		if (mode != MODE_CFG_HIGHT_TEMP)
			{
				mode = MODE_CFG_HIGHT_TEMP;
				module.clearDisplay();
				module.setDisplayToString("hi temp");
				delay(PAUSE);
				module.clearDisplay();
	
				String ind_str = String("hTmp ");
				ind_str += hTemperature;
      	module.setDisplayToString(ind_str);
			}
		else
			{
				mode = MODE_AUTO;
				module.clearDisplay();
				module.setDisplayToString("AUTO");
				delay(PAUSE);
				module.clearDisplay();
			}
	
	if (keys == BTN_SET_SERVO_LIMIT)
		if (mode != MODE_CFG_SERVO_LIMIT)
			{
				mode = MODE_CFG_SERVO_LIMIT;
				module.clearDisplay();
				module.setDisplayToString("SERV LIM");
				delay(PAUSE);
				module.clearDisplay();
	
				String ind_str = String("SLIM ");
				ind_str += servo_limit;
      	module.setDisplayToString(ind_str);
			}
		else
			{
				mode = MODE_AUTO;
				module.clearDisplay();
				module.setDisplayToString("AUTO");
				delay(PAUSE);
				module.clearDisplay();
			}
	
	if (keys == BTN_SAVE)
		{
			EEPROM.write(EEPROM_DELAY, loop_delay);

			EEPROM.write(EEPROM_LOW_TEMPERATURE, abs(lTemperature));
			EEPROM.write(EEPROM_LOW_TEMPERATURE_SIGN, lTemperature < 0 ? 1 : 0);
			
			EEPROM.write(EEPROM_HIGHT_TEMPERATURE, abs(hTemperature));
			EEPROM.write(EEPROM_HIGHT_TEMPERATURE_SIGN, hTemperature < 0 ? 1 : 0);

			EEPROM.write(EEPROM_INVERSE, inverse);
			EEPROM.write(EEPROM_SERVO_LIMIT, servo_limit);

			module.clearDisplay();
			module.setDisplayToString("SAVE");
			delay(PAUSE);
			module.clearDisplay();
		}

  delay(TICK_TIMER);
} 
