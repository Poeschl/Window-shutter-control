# Window Shutter Control

This is a ardoino script which controls a servo which presses the shutter switches.
It gets the commands via the mqtt protocol.

For more take a look here: [Blog](https://nerdweibweb.de/smarthome/iot-rolladensteuerung.html)

## Compiling

It should work on any `ESP8266` board and uses following libraries:

* Servo
* ESP8266WiFi
* PubSubClient

## Settings

Some adujustables settings are on top of the file. Most of them are prefilled with my working values.

_Important:_ Don't forget to insert your wifi and mqtt broker credentials.

If you have more than one controller in your configuration, change the `COVER_ID` to a unique id.

## ESPHome

If you already have `ESPHome` running, I also added the config for it.
