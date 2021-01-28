/*
 Name:		YodaBot.ino
 Created:	1/12/2021 10:24:49 AM
 Author:	markb

 Connection Diagram
 ------------------

 Connecting to WeMos D1 mini R2 board:

 Reset switch:
 D7 (RESET_PIN) -> momentary switch -> ground

 Capacative sensor activate:
 D6 (PLATE_PIN) -> plate on yoda's head, placed on top of sensor
 D2 (RELAY_PLATE_PIN) - Relay control to connect plate to ground
*/

#ifdef ARDUINO_ARCH_ESP32
#include <SDFSFormatter.h>
#include <SDFS.h>
#include <EspalexaDevice.h>
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)  printD (x)
#define DEBUG_PRINTLN(x) printlnD(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#include <SerialDebug.h>

#include <ESP8266WebServer.h>
#include <WiFiManager.h>     // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// #define ESPALEXA_DEBUG
#include <Espalexa.h>

// Circular buffer & timer is to help detect 3 presses on reset button
#include "CircularBuffer.h"
#include <TimeLib.h>


// SerialDebug Library

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
//#define DEBUG_DISABLED true

// Disable SerialDebug debugger ? No more commands and features as functions and globals
// Uncomment this to disable it 
//#define DEBUG_DISABLE_DEBUGGER true

// Define the initial debug level here (uncomment to do it)
//#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_VERBOSE

// Disable auto function name (good if your debug yet contains it)
//#define DEBUG_AUTO_FUNC_DISABLED true

// Include SerialDebug

#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

// For detecting request to reset WIFI parameters - push button 3 times in a row.
// See check_for_reset()
#define READINGS 3
#define TIMEFRAME 5L
CircularBuffer buttonPushBuffer(READINGS);

// Pins used 
#define RESET_PIN D7 // Push 3 times to reset wifi
#define PLATE_PIN D6 // Goes to plate on yoda's head - will trigger action
#define RELAY_PLATE_PIN D2
#define STATUS_LED LED_BUILTIN	// Status LED - this is the blue one built into ESP8266 chip

// WiFi configuration setup
#define CONFIG_NAME "Yoda Net"        // Configuration Name, used for Wifi Manager.  Needs to be unique.
unsigned long last_wifi_check_time = 0;
boolean wifiConnected = false;

// Espalexa setup
Espalexa espalexa;
EspalexaDevice* yoda_device;

/*************************** Sketch Code ************************************/
void setup() {
	Serial.begin(115200);
	delay(100);

	// Set up status LED (blue LED on chip)
	pinMode(STATUS_LED, OUTPUT);
	digitalWrite(STATUS_LED, HIGH); // turn off blue LED

	// Set up reset button
	pinMode(RESET_PIN, INPUT_PULLUP); // activates pullup resistor
	buttonPushBuffer.init(); // Initialize our buffer to detect 3 pushes in sequence

	// TODO: Remove this testing code
//	pinMode(PLATE_PIN, INPUT);
	pinMode(PLATE_PIN, OUTPUT);
	digitalWrite(PLATE_PIN, LOW);

	// Set up relay
	pinMode(RELAY_PLATE_PIN, OUTPUT);
	digitalWrite(RELAY_PLATE_PIN, LOW);

	DEBUG_PRINTLN("Calling connect_network()");

	// Initialise wifi connection
	wifiConnected = connect_network();

	if (wifiConnected) {
		// Define espalexa devices
		yoda_device = new EspalexaDevice("Baby Yoda", deviceChanged);
		espalexa.addDevice(yoda_device);
		espalexa.begin();
	}
	else
	{
		while (1) {
			printlnD("Cannot connect to WiFi. Please check data and reset the ESP.");
			delay(2500);
		}
	}

}

void loop() {
	espalexa.loop();
	delay(10);
	// Handle reset button
	check_for_reset();
}

void trigger_yoda()
{
	// Found this method from
	// https://electronics.stackexchange.com/questions/19435/triggering-a-capacitive-sensor-electronically

	// This should trigger a "touch" to the sensor
	//pinMode(PLATE_PIN, OUTPUT);
	digitalWrite(RELAY_PLATE_PIN, HIGH);
	/*
	digitalWrite(PLATE_PIN, HIGH);
	delay(10);
	digitalWrite(PLATE_PIN, LOW);
	delay(10);
	digitalWrite(PLATE_PIN, HIGH);
	delay(10);
	digitalWrite(PLATE_PIN, LOW);
	delay(10);
*/
	delay(75);
	digitalWrite(RELAY_PLATE_PIN, LOW);

	//pinMode(PLATE_PIN, INPUT);
}

// Callback function, called when Alexa sends us a "Yoda On" signal
void deviceChanged(uint8_t brightness) {

	if (brightness) {
		// Yoda trigger just received
		printlnD("Yoda ON Received, calling trigger_yoda().");
		digitalWrite(STATUS_LED, LOW); // turn ON blue LED

		trigger_yoda();
		yoda_device->setValue(0); // Turn off
		printlnD("Turning Yoda OFF");
		digitalWrite(STATUS_LED, HIGH); 
	}
	else {
		printlnD("Yoda OFF received");
		digitalWrite(STATUS_LED, HIGH); // turn off blue LED
	}
}

// Connects to our network
boolean connect_network() {
	// WiFiManager local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;
	DEBUG_PRINTLN("connect_network()");

	// For DEBUG only - wipes out saved wifi settings.  ONLY UNCOMMENT THIS LINE IF IT WON'T CONNECT
	// wifiManager.resetSettings();

	/*
	// Set initial static IP - user can always change this in the UI
	IPAddress _ip = IPAddress(192, 168, 1, 204);
	IPAddress _gw = IPAddress(192, 168, 1, 1);
	IPAddress _sn = IPAddress(255, 255, 255, 0);
	wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
	*/

	// Fetches ssid and pass from eeprom and tries to connect.
	// If it does not connect it starts an access point with the specified name
	// and goes into a blocking loop awaiting configuration
	if (!wifiManager.autoConnect(CONFIG_NAME)) {
		DEBUG_PRINTLN("Failed to connect, will try resetting to see if it connects");
		delay(3000);
		ESP.reset();
		delay(5000);
	}

	DEBUG_PRINT("My IP: ");
	DEBUG_PRINTLN(WiFi.localIP());
	// digitalWrite(STATUS_LED, LOW); // turn on blue LED to signify connected

	return (WiFi.status() == WL_CONNECTED);
	
}

// To reset our WIFI manager, you need to press the reset switch 3 times
void check_for_reset() {
	if (buttonPressed())
	{
		buttonPushBuffer.addElement(now());
		printlnD(now());
		if (buttonPushBuffer.eventsExceeded())
		{
			// They want us to reset our WIFI parameters
			DEBUG_PRINT("3 or more reset presses in the last ");
			DEBUG_PRINT(TIMEFRAME);
			DEBUG_PRINTLN(" seconds. Resetting.");
			reload_network_config();
		}
	}
}

bool buttonPressed(void)
{
	static byte lastPressed = 1;
	byte currentPress = digitalRead(RESET_PIN);
	if (currentPress != lastPressed)
	{
		if (currentPress == 0)
		{
			lastPressed = currentPress;
			return true;
		}
	}
	lastPressed = currentPress;
	delay(50);
	return false;
}


// Reloads network config
void reload_network_config() {

	DEBUG_PRINTLN("reload_network_config()");

	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;

	if (!wifiManager.startConfigPortal(CONFIG_NAME)) {
		DEBUG_PRINTLN("Failed to connect.  Delaying, resetting and trying again.");
		delay(3000);
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}

	//if you get here you have connected to the WiFi
	DEBUG_PRINT("Reconnected.  My IP: ");
	DEBUG_PRINTLN(WiFi.localIP());
}

