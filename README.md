# YodaBot
Makes a Hasbro "The Child" Baby Yoda into an Alexa-controllable device.

Utilizes the Espalexa library to emulate a Phillips HUE lightbulb to Alexa.  Once paired, "Baby Yoda" shows up under "Devices" in the Alexa App. 

Baby Yoda must be connected on the same local network as Alexa is. YodaBot uses the WiFiManager library to bring up the captive portal, allowing you to specify network connection details such as SSID and password.

To make Baby Yoda talk and play one of it's built-in animations, turn the "Baby Yoda" device to an ON state in the Alexa App (or via Alexa voice commands). For more fun interactions, you can create an Alexa routine where Alexa says something funny to Baby Yoda ("Tickle tickle"), then in the routine you turn on the "Baby Yoda" device, and Baby Yoda starts animating and talking as if responding to Alexa.

## Hardware
This project uses an ESP8266 based board, a Wemos D1 mini R2, placed inside of Yoda's body. Other ESP8266 or ESP32 boards should work also, as long as they fit inside of Yoda. To trigger the animations, the code emulates a "finger touch" on the capacative touch sensor on Yoda's head. This is done using a copper plate wired to one of the Wemos pins.  The copper plate is placed directly underneath the existing plate on Yoda's head.  To emulate a "finger touch" the pin is driven using PWM, which seems to work reliably.

For initial wifi setup (or to reset wifi and reconfigure), YodaBot uses a momentary switch. To trigger wifi setup, push the switch 3 times quickly (within 5 seconds). This will erase the existing wifi parameters and put YodaBot into AP mode using the "YodaNet" network.  Connect to this wifi network using your favorite device, and it should bring up a captive portal.

For status, an LED shows wifi and setup status:
-	SLOW FLASH - Trying to connect to configured wifi network
-	SOLID - Connected to wifi network
-	FAST FLASH - Network setup mode, connect to "YodaNet" to configure wifi
-	SOLID - SHORT FLASH - SOLID - Just received Yoda ON command from Alexa

## Other Approaches
My first approach to hacking Baby Yoda involved using MQTT on adafruit.io, and a custom Lambda function on AWS, triggered from a custom Alexa Skill. Yikes! So much code to write, and lots of complexity! Although that approach was probably overkill, you could do potentially anything as far as interaction between Alexa and Baby Yoda.  

The beauty of the Espalexa library approach is that the hardware acts like a device that Alexa already knows about. Alexa finds the device using the standard "discover devices", and then talks directly with this device using the local network. There are no lambda functions to write or Alexa Skills to write to make it work.

Enjoy!
