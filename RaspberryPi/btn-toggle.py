#!/usr/bin/python3
# ------------------------------------------------------------------------------
"""@package blink.py

Makes an RGP LED blink different colours using GPIO.
"""
# ------------------------------------------------------------------------------
#                  Kris Dunning ippie52@gmail.com 2020.
# ------------------------------------------------------------------------------
import wiringpi
from time import sleep

RED_GPIO = 0
GRN_GPIO = 2
BLU_GPIO = 3

LEDS = [RED_GPIO, GRN_GPIO, BLU_GPIO]

BTN_GPIO = 1

USE_PTM = False

wiringpi.wiringPiSetup()  # For GPIO pin numbering
for led in LEDS:
    wiringpi.pinMode(led, 1) 
    wiringpi.digitalWrite(led, 0) 
wiringpi.pinMode(BTN_GPIO, 0)

index = 0
running = True
last_state = 0
while running:
    try:
        btn_state_1 = wiringpi.digitalRead(BTN_GPIO)
        sleep(0.001)
        btn_state_2 = wiringpi.digitalRead(BTN_GPIO)
        if (btn_state_1 == btn_state_2) and (btn_state_1 != last_state):
            if not USE_PTM or last_state != 0:
                wiringpi.digitalWrite(LEDS[index], 0)
                index = (index + 1) % len(LEDS)
                wiringpi.digitalWrite(LEDS[index], 1) 
                # sleep(1)
        last_state = btn_state_1
    except KeyboardInterrupt:
        running = False
    except Exception as e:
        print('Error occurred:')
        print(e)
        raise e



