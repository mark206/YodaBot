'''
#
# Yoda Gadget
# 1/5/2021
# Mark Bader

Wiring:
Raspberry Pi:
    Pin 4 - 5V - to both relays
    Pin 6 - GND - to both relays
    Pin 8 - to signal line on GND_RELAY
    Pin 10 - to signal line on ONOFF_RELAY
    Pin 12 - PWM signal, goes to head capacative sensor

Yoda:
    Capacative sensor - wire touching sensor is connected to Pin 12 and also GND_RELAY
    Once we send PWM to the sensor (which simulates a finger touch), I've found that
    we need to "reset" it's capacatance by bringing that sensor to ground.  That's where
    the GND_RELAY comes into play - after we send PWM, we open up the relay which brings
    the sensor back to ground potential.  By doing it this way, we can trigger the sensor
    much more reliably.
    
    Ground - the ground line from yoda's battery goes to both the GND_RELAY input pin
    as well as the ONOFF_RELAY input pin.
    
    Electronics Ground - the ground wire from yoda's electronics (which used to go to the
    battery) now goes to the ONOFF_RELAY.  When this relay is energized, it fires up the
    electronics so yoda is now active.
'''

import logging
import sys
import RPi.GPIO as GPIO
import time
import threading 

from agt import AlexaGadget

logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger(__name__)

# PIN 8 = ground shunt relay - for grounding capacatance sensor to reset capacatance
# PIN 10 = on/off relay - for turning yoda on and off

ONOFF_RELAY = 10
GND_RELAY = 8
PIN_DRIVE = 12
GPIO.setmode(GPIO.BOARD)
GPIO.setup(ONOFF_RELAY, GPIO.OUT)
GPIO.setup(GND_RELAY, GPIO.OUT)
GPIO.output(ONOFF_RELAY, GPIO.LOW)
GPIO.output(GND_RELAY, GPIO.LOW)

GPIO.setup(PIN_DRIVE, GPIO.IN) # set to input so it's floating = no touch

class YodaGadget(AlexaGadget):
    
    def __init__(self):
        super().__init__()
        self.talking = 0 #set to 1 when talking
        
    # Timer callback to turn off power
    def poweroff(self):
        logger.info('timer fired, poweroff() called - turning off gadget')
        self.timer.cancel()
        self.talking = 0
        GPIO.output(ONOFF_RELAY, GPIO.LOW) #Turn off device
        GPIO.output(GND_RELAY, GPIO.LOW) #Turn off ground relay
        
    def on_alexa_gadget_statelistener_stateupdate(self, directive):
        for state in directive.payload.states:
            if state.name == 'wakeword':
                if state.value == 'active':
                    logger.info('Wake word active - turn on, timer started')
                    GPIO.output(ONOFF_RELAY, GPIO.HIGH)
                    # start timeout so we turn off after 15 sec
                    self.timer = threading.Timer(15, self.poweroff)
                    self.timer.start()
                elif state.value == 'cleared':
                    logger.info('Wake word cleared')
                    # do nothing

    def trigger_yoda(self):
        # Found this method from
        # https://electronics.stackexchange.com/questions/19435/triggering-a-capacitive-sensor-electronically
    
        # Send some PWM to indicate a touch action
        GPIO.setup(PIN_DRIVE, GPIO.OUT) # Set pin to output
        p = GPIO.PWM(PIN_DRIVE, 25) #frequency in hz
        duty_cycle = 50
        p.start(duty_cycle)
        time.sleep(0.5)
        p.stop()
        p.start(duty_cycle) 
        time.sleep(0.5)
        p.stop()
        GPIO.setup(PIN_DRIVE, GPIO.IN) # Back to input so it's floating = no touch
        # Now trigger our "reset" relay to reset the capacitance
        GPIO.output(GND_RELAY, GPIO.HIGH)

    def on_alexa_gadget_speechdata_speechmarks(self, directive):
        """
        Alexa.Gadget.SpeechData Speechmarks directive received.
        For more info, visit:
            https://developer.amazon.com/docs/alexa-gadgets-toolkit/alexa-gadget-speechdata-interface.html#Speechmarks-directive
        :param directive: Protocol Buffer Message that was send by Echo device.
        """
        # Only trigger head first time we get speech
        logger.info('Speech')
        if (self.talking == 0):
            logger.info('Talking Started')
            self.talking = 1
            self.trigger_yoda()
            
        
if __name__ == '__main__':
    try:
        YodaGadget().main()
    finally:
        logger.debug('Cleaning up GPIO')
        GPIO.cleanup()

