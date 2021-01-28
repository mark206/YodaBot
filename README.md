# YodaBot
Makes a Hasbro "The Child" Baby Yoda into an Alexa-controllable device.

Utilizes the Espalexa library to emulate a Phillips HUE lightbulb to Alexa, shows up under "Devices" in the Alexa App.  

To use this, create an Alexa routine, and have Alexa say something funny like "Tickle Tickle", then turn on the "Baby Yoda" light.  This will trigger Baby Yoda to play one of it's pre-recorded animations.

## Technical  Details
I created this using an ESP8266 board. The beauty of this solution is that Alexa talks directly with this device using the local network, and it can all be controlled using an Alexa routine. There are no Lambda functions or Alexa Skills to write to make this work.

The board I used was a Wemos D1 mini R2, placed inside of Yoda's body. I'm sure other ESP8266 or ESP32 boards would work, as long as they fit inside of Yoda. To trigger the animations, I emulated a "finger touch" on the capacative touch sensor on Yoda's head using a copper plate wired to one of the Wemos pins.

For initial wifi setup (or to reset wifi and reconfigure), YodaBot uses a momentary switch. To trigger the setup, push the switch 3 times quickly (within 5 seconds).

For status, an LED shows wifi and setup status:
-	SLOW FLASH - Trying to connect to configured wifi network
-	SOLID - Connected to wifi network
-	FAST FLASH - Network setup mode, connect to "Yoda Net" to configure wifi
-	SOLID - SHORT FLASH - SOLID - Just received Yoda ON command from Alexa

Enjoy!
