# fingerprint-scanner-arduino
Fingerprint scanner used to automatically send stored passwords to a PC USB keyboard.
Uses fingerprint scan module AS608 from Adafruit and Arduino Pro Micro, 3.3 Volt, 4 MHz version.

# Hardware
The hardware consists of an AS608 fingerprint scanner and Arduino Pro Micro board. Four connections between the Arduino and fingerprint scanner are required. The hardware is powered by the Arduino's USB connection to the host PC. The Arduino USB connection has serial communication functionality and also appears to the host PC as a USB keyboard.

# Wiring:

      Pro Micro   AS608
      TX          RX
      RX          TX
      Vcc         Vcc
      Gnd         Gnd
      
# Firmware
By default, the firmware waits for a valid fingerprint and sends the password associated with the particular fingerprint to the keyboard. So after initial setup, the hardware is left plugged in to the PC's USB port. Instead of typing, the unit sends the long, secure password, up to 63 characters, to the USB keyboard.

Initial setup is done through serial communication wth the device using a terminal program such as PuTTY. The user's fingerprints and asscoiated passwords can be enrolled using the serial port only. A seperate application may also be used to enroll fingerprint models by placing the device in passthru mode allowing direct access to the AS608 fingerprint scanner.  
