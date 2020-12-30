# SomfyRTS
 A universal, multi-channel, HomeKit Controller for Somfy RTS Motorized Window Shades and Patio Screens. Runs on an ESP32 device as an Arduino sketch using the Arduino [HomeSpan Library](https://github.com/HomeSpan/HomeSpan).

Hardware used for this project:

* An ESP32 board, such as the [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* An RFM69 Transceiver, such as this [RFM69HCW FeatherWing](https://www.sparkfun.com/products/10534) from Adafruit
* One small pushbutton (normally-open) to serve as both the Somfy PROG button and the channel selector button
* Three large pushbuttons (normally-open) to serve as the Somfy UP, DOWN, and MY buttons.
* One small pushbutton (normally-open) to serve as the HomeSpan Control Button (optional)

# Overview

Somfy motors are widely used in automated window shades, patios screens, and porch awnings.  And though there are many different models, almost all are controlled with a standardized system Somfy calls RTS, or [Radio Technology Somfy](https://asset.somfy.com/Document/dcb579ff-df8d-47d8-a288-01e06a4480ab_RTS_Brochure_5-2019.pdf) using Somfy RF controllers, such as the 5-channel [Somfy Tellis RTS](https://www.somfysystems.com/en-us/products/1810633/telis-rts).

All Somfy remotes feature:

* an UP button that typically raises the window shade or screen until it is fully opened;
* a DOWN buttonthat typicall lowers the window shade or screen until it is fully closed;
* a button labeled "MY" that serves two purposes - 
  * if the shade is moving, pressing the MY button stops the motor
  * if the shade it stopped, pressing the MY button moves the shade to a predefined position (the "MY" position)
* a PROG button that is used to put the motor into programming mode or "learn" mode so you can add additional remotes; and
* a channel selector, for remotes that allow the user to control more than one shade or screen from one remote.

Based on the **superb** work by [Pushstack](https://pushstack.wordpress.com/somfy-rts-protocol/) and other contributors for reverse-engineering and documenting the Somfy-RTS protcols (much thanks!), we can construct a fully-functional, *HomeKit-enabled*, multi-channel Somfy remote using an ESP32, a simple transmitter, and the Arduino HomeSpan Library.

Apart from the obvious benefit of having HomeKit control of your Somfy shades and screens, our HomeSpan version of the Somfy remote also includes two additional benefits:

* The remote allows for an arbitrary number of channels.  Have a 20 window shades spread across 5 rooms?  No problem - you can control all of them with this single device.
* Use HomeKit to *set the absolute position of your window shade or screen!*  HomeKit natively supports a slider that allows you to specity the exact position of a window shade, from fully open (100%) to fully closed (0%) in increments of 1%.  Unfortunately, the Somfy RTS system does not generally support two way communications, nor do the motors transmit status about the position of the shade or screen.  However, some clever logic inside the sketch and a stopwatch is all that is needed to configure our HomeSpan remote to track and directly set the window shade to any position.



