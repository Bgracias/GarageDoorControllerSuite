# GarageDoorControllerSuite
Home Garage door controller suite using Arduino and ESP32 boards.

GarageController - Auto closes Garage Door in about 5 minutes.

GarageFlasher - 10 seconds before the Garage Door is closed, this device will trigger the connected light bulb in the garage(110v or 230v) to flash rapidly.

HomeDisplay2024 - Displays current state of the Garage Door.

Details:

GarageController:
  Hardware - ESP32, Ultrasonic Sensor, Speaker, pnp transistor.
  Schematic - TODO.
  Installation/Setup - TODO.
  How it works: This device is installed on the railing(or ceiling) of the garage(door) in such a position that wheneveer utrasonic sensor detects a distance greater than 15"(configurable), it means there is no car in the garage and hence a timer gets activated. Timer is set to 5 minutes. Once 5 minutes are up and the distance is still greater than 15", it means garage is still open and mostly likely owner forgot to close garage. A transistor used as a switch is activated which shorts the 2 pins on my garage door opener(basic garage door opener) whcih does the actual closing of the garage door. A simple wire runs from esp32 to the ends of the 2 pins. The device constantly sends a message to the other 2 ESP32 devices below about its state using ESPNow. 
  
GarageFlasher:
  Hardware - ESP32, Relay to turn on or off the 110v light bulb.
  Schematic - TODO.
  Installation/Setup - TODO.

HomeDisplay2024:
  Hardware - ESP32, Display Screen, Speaker.
  Schematic - TODO.
  Installation/Setup - TODO.

