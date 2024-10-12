# DustSensor
 RP2040 PMS5003 dust monitor.
 Works with a SSD1106 for display the data in the field.
 
 Project use the 2 cores. One for displaying the data n the OLED, and the other for reading senso data and send it through Serial every 5 seconds.
 Project also use 2 serial. One for comunicating with the PMS5003 and another to send data through the RP2040 USB-C.
 
# RGB led 
 The RGB led from the Waveshare RP2040 Zero (pin 16) is used to display the air quality (Green, orange and red). 

# Battery monitor
 Voltage sensor is calibrated for a 1s LiPo battery connected to pin 29 through a voltage divider (3k3 & 1k).
 If the divider read less than 0.5v it assume is working through the USB-C.

# Changelog
 * **v1.0.0** - Initial release 