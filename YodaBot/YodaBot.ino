/*
 Name:		YodaBot.ino
 Created:	1/12/2021 10:24:49 AM
 Author:	markb

 Converts Hasbro's Baby Yoda doll into an Alexa-controllable device.  

 To use this, get a Wemos D1 mini R2 board, load this software, connect
 pins as below, and supply power to the board.

 CONNECTIONS:
 
 Power: Should be powered separately from Baby Yoda's battery pack. Simplest solution
 is to just plug the USB port on the Wemos D1 into a USB charger, which will supply
 5 volts. The ESP chip draws quite a bit of power, and since it can never go into
 standby mode, it would draw the battery pack down quickly.

 Ground: The ground of the Wemos D1 MUST be attached to the ground of the battery
 pack of yoda. This allows us to reliably activate the capacative touch sensor
 on the top of yoda's head.

 D7 (WIFI_RESET_PIN) -> momentary switch -> ground
 Reset switch.  Press this switch 3 times within 5 seconds for a wifi reset, which will
 start AP and bring up the captive portal wifi configuration screen.

 D6 (PLATE_PIN) -> plate on yoda's head, placed on top of (or under) sensor.
 Make sure this plate does not touch existing capacative sensor plate.  Use tape or
 plastic to ensure no contact with plate.

 D2 (EXTERNAL_STATUS_LED) -> 220 ohm (or greater) resistor -> LED Long Pin -> LED -> LED Short Pin -> Ground
 LED signifies status:
	SLOW FLASH - Trying to connect to configured wifi network
	SOLID - Connected to wifi network
	FAST FLASH - Network setup mode, connect to "Yoda Net" to configure wifi
	SOLID - SHORT FLASH - SOLID - Just received Yoda ON command from Alexa

 INTERNAL_STATUS_LED - Internal blue LED, no wiring needed. Signifies Alexa command received
 to turn yoda on.

 COMPILE INSTRUCTIONS:
 Compile using Visual Micro Arduino compiler. Install libraries shown below. 
 Select Board "LOLIN(WEMOS) D1 R2 & mini"
 
 */

#include <Time.h> // Built-in
#include <ESP8266WiFi.h> // General WIFI lib for ESP8266

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
// #define DEBUG_DISABLED true

#include <SerialDebug.h>	//https://github.com/JoaoLopesF/SerialDebug
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// #define ESPALEXA_DEBUG
// ESP Alexa - simulates a Philips Hue bulb
#include <Espalexa.h> // https://github.com/Aircoookie/Espalexa

// Circular buffer & timer is to help detect 3 presses on reset button
#include "CircularBuffer.h" // Local file
#include <TimeLib.h> // Built-in

// For detecting request to reset WIFI parameters - push button 3 times in a row.
CircularBuffer buttonPushBuffer(3);

// Pins used 
#define WIFI_RESET_PIN D7 // Push 3 times to reset wifi
#define PLATE_PIN D6 // Goes to plate on yoda's head - will trigger action
#define INTERNAL_STATUS_LED LED_BUILTIN	// Status LED - this is the blue one built into ESP8266 chip
#define EXTERNAL_STATUS_LED D2 // For reporting WIFI pairing status

//for LED status light flash
#include <Ticker.h>
Ticker ticker;

// WiFi configuration setup
#define CONFIG_NAME "YodaNet"        // Configuration Name, used for Wifi Manager.  Needs to be unique.
unsigned long last_wifi_check_time = 0;
boolean wifiConnected = false;

// Espalexa setup
Espalexa espalexa;
EspalexaDevice* yoda_device;

/*************************** Sketch Code ************************************/
void setup() {
	Serial.begin(115200);
	delay(100);

	WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

	printlnA(F("Initializing."));

	// Set up status LEDs
	pinMode(INTERNAL_STATUS_LED, OUTPUT);
	digitalWrite(INTERNAL_STATUS_LED, HIGH); // turn off blue LED
	pinMode(EXTERNAL_STATUS_LED, OUTPUT);
	digitalWrite(EXTERNAL_STATUS_LED, LOW); // turn off external status LED

	// Set up reset button
	pinMode(WIFI_RESET_PIN, INPUT_PULLUP); // activates pullup resistor
	buttonPushBuffer.init(); // Initialize our buffer to detect 3 pushes in sequence

	// Turn plate pin to INPUT.  This sets it to float = "no touch".
	pinMode(PLATE_PIN, INPUT);

	// Initialise wifi connection
	printlnA("Calling connect_network()");
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
			printlnA("Cannot connect to WiFi. Please check data and reset the ESP.");
			delay(2500);
		}
	}

}

void loop() {
	debugHandle(); // for DebugSerial events
	espalexa.loop(); // Handle Alexa messages
	delay(10);
	check_for_reset(); // Handle reset button
}

void trigger_yoda()
{
	// Found this method from
	// https://electronics.stackexchange.com/questions/19435/triggering-a-capacitive-sensor-electronically

	// The method below triggers a "touch" event to the Yoda board. 

	// To do this, connect the plate pin to a metal plate, placed BENEATH the existing capacative touch
	// sensor on the head of Yoda. I've had success using copper tape on a piece of plastic, covered 
	// in scotch tape.  Make sure the copper does not make electrical contact with the existing
	// plate. Our plate should simulate a "touch" by inducing a signal that looks like a finger
	// on their capacative plate.  The best way to do this is to create a PWM signal on our plate.

	pinMode(PLATE_PIN, OUTPUT);
	digitalWrite(PLATE_PIN, HIGH);
	analogWriteFreq(25); // Frequency in hz

	analogWrite(PLATE_PIN, 50); // Duty cycle
	delay(500);
	analogWrite(PLATE_PIN, 0);

	analogWrite(PLATE_PIN, 50); // Duty cycle
	delay(500);
	analogWrite(PLATE_PIN, 0);

	digitalWrite(PLATE_PIN, LOW);

	pinMode(PLATE_PIN, INPUT); // go back to float (= "no touch")
}

// Callback function, called when Alexa sends us a "Yoda On" signal
void deviceChanged(uint8_t brightness) {

	if (brightness) {
		// Yoda trigger just received
		printlnD("Yoda ON Received, calling trigger_yoda().");
		digitalWrite(INTERNAL_STATUS_LED, LOW); // turn ON blue LED
		digitalWrite(EXTERNAL_STATUS_LED, !digitalRead(EXTERNAL_STATUS_LED)); // toggle state

		trigger_yoda();
		yoda_device->setValue(0); // Turn off
		printlnD("Turning Yoda OFF");
		digitalWrite(INTERNAL_STATUS_LED, HIGH);
		digitalWrite(EXTERNAL_STATUS_LED, !digitalRead(EXTERNAL_STATUS_LED)); // toggle state

	}
	else {
		printlnD("Yoda OFF received");
		digitalWrite(INTERNAL_STATUS_LED, HIGH); // turn off blue LED
		digitalWrite(EXTERNAL_STATUS_LED, !digitalRead(EXTERNAL_STATUS_LED)); // toggle state
	}
}

// Connects to our network
boolean connect_network() {

	// start LED ticker with 0.5 because we start in AP mode and try to connect
	ticker.attach(0.5, tick);

	// WiFiManager local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;
	printlnA("connect_network()");

	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);

	// wifiManager.setDebugOutput(false);

	// For DEBUG only - wipes out saved wifi settings.  O
	// ONLY UNCOMMENT THIS LINE IF IT WON'T CONNECT
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
		printlnA("Failed to connect, will try resetting to see if it connects");
		delay(3000);
		ESP.reset();
		delay(5000);
	}

	printA("My IP: ");
	printlnA(WiFi.localIP());

	//if you get here you have connected to the WiFi
	ticker.detach();
	// turn on external LED to signify WIFI CONNECTED
	digitalWrite(EXTERNAL_STATUS_LED, HIGH);

	return (WiFi.status() == WL_CONNECTED);
}

// To reset our WIFI manager, you need to press the reset switch 3 times
void check_for_reset() {
	if (buttonPressed())
	{
		buttonPushBuffer.addElement(now());
		printlnA(now());
		if (buttonPushBuffer.eventsExceeded())
		{
			// They want us to reset our WIFI parameters
			//buttonPushBuffer.
			printA("3 or more reset presses in the last 5 seconds. Resetting.");
			reload_network_config();
		}
	}
}

bool buttonPressed(void)
{
	static byte lastPressed = 1;
	byte currentPress = digitalRead(WIFI_RESET_PIN);
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

// Reloads network configuration - wipes out existing wifi settings, and 
// brings up config portal
void reload_network_config() {

	printlnA("reload_network_config()");

	digitalWrite(EXTERNAL_STATUS_LED, LOW); // Status led OFF

	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;

	Serial.println("Button Held");
	Serial.println("Erasing Config, restarting");
	wifiManager.resetSettings();
	ESP.restart();
}

// Toggles status LED
void tick()
{
	//toggle state
	digitalWrite(EXTERNAL_STATUS_LED, !digitalRead(EXTERNAL_STATUS_LED)); 
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {
	printA("Entered config mode");
	printlnA(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	printlnA(myWiFiManager->getConfigPortalSSID());
	//entered config mode, make led toggle faster
	ticker.attach(0.2, tick);
}