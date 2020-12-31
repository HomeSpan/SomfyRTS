# SomfyRTS
A universal, multi-channel, HomeKit Controller for Somfy RTS Motorized Window Shades and Patio Screens. Runs on an ESP32 device as an Arduino sketch using the Arduino [HomeSpan Library](https://github.com/HomeSpan/HomeSpan).

Hardware used for this project:

* An ESP32 board, such as the [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* An RFM69 Transceiver, such as this [RFM69HCW FeatherWing](https://www.sparkfun.com/products/10534) from Adafruit
* One regular-size pushbutton (normally-open) to serve as both the Somfy PROG button and the channel selector button
* Three regular-size pushbuttons (normally-open) to serve as the Somfy UP, DOWN, and MY buttons.
* One small pushbutton (normally-open) to serve as the HomeSpan Control Button (optional)

## Overview

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

* **Use HomeKit to set the absolute position of your window shade or screen!**  HomeKit natively supports a slider that allows you to specify the exact position of a window shade, from fully open (100%) to fully closed (0%) in increments of 1%.  Unfortunately, the Somfy RTS system does not generally support two way communications, nor do the motors transmit status about the position of the shade or screen.  However, some clever logic inside the sketch and a stopwatch is all that is needed to configure our HomeSpan remote to track and directly set the window shade to any position.

## Constructing the Hardware

In addition to an ESP32 board, our remote requires a "434 MHz" transmitter.  However, rather than using the standard carrier frequency of 433.92 MHz, Somfy RTS uses a carrier frequency of 433.42 MHz, which is 0.5 MHz lower than the standard.  Though it is possble to use a standard 433.92 MHz transmitter (such as the one used to construct a HomeSpan remote control for a [Zephyr Kitchen Vent Hood](https://github.com/HomeSpan/ZephyrVentHood)), there is no guarantee that the Somfy motor will accurately receive the RF signal, or that the range will allow for whole-home coverage.

Instead, this project uses an RFM69 *programmable* 434 MHz transceiver that can be configured to use a carrier frequency of 433.42 MHz, which exactly matches the Somfy RTS system.  The ESP32 communicates with the RFM69 via the ESP32's external SPI bus, which requires you to connect the MOSI, MISO, and SCK pins on your ESP32 to those same pins on your RFM69.  If you are using Adafruit's RFM69 FeatherWing in combination with Adafruit's ESP32 Feather Board, these connections are already hardwired for you.  In addition, you'll need to make 3 other connections from "output" pins on the ESP32 to "input" pins on the RFM69:

* The SPI Chip Select ("CS") Pin on the RMF69 needs to be connected to a pin on the ESP32 used to enable the RMF69 SPI bus.  This sketch uses GPIO pin 33 on the ESP32 for the RFM69 Chip Select.  If you are using the AdaFruit combination above, simply solder a jumper between the through-hole on the RFM69 FeatherWing labeled "CS" and the through-hole labeled "B" (which is conveniently hardwired to GPIO pin 33).

* The Reset Pin on the of the RFM69 needs to be connected to a pin on the ESP32 which will be used to reset the configuration of the RFM69 settings.  This sketch uses GPIO pin 27.  If you are using the AdaFruit combination above, simply solder a jumper between the through-hole on the RFM69 FeatherWing labeled "RST" and the through-hole labeled "A" (which is conveniently hardwired to GPIO pin 27).

* The DIO2 Pin on the RFM69 needs to be connected to a pin on the ESP32 that will be used to output the Somfy RF codes generated by our sketch and feed them into the RFM69, which converts them to 433.42 MHz signals.  This sketch uses GPIO pin 4.  If you are using the AdaFruit combination above, simply solder a jumper between the through-hole on the RFM69 FeatherWing labeled "DIO2" and the through-hole labeled "F" (which is conveniently hardwired to GPIO pin 4).

You can of course use different pins for any of the above connections.  Just make sure to update the pin definitions at the top of the sketch to match whatever pins you have chosen:

```C++
// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69
```
NOTE:  If instead of using an RFM69 you decide to try a standard, non-programmable, 433.92 MHz transmiter, you can skip all the connections above except for the RF Signal which should still be connected from pin 4 on the ESP32 (or any alternative pin you chose) to the signal input pin of your transmitter.  The sketch will warn you that it cannot find the RFM69 when it first runs, but should work fine without modification.

Our HomeSpan Somfy remote also makes use of 5 pushbutton switches (4 are optional, 1 required).  The required pushbutton performs double-duty and serves as the Somfy PROG button as well as the device's channel selector.  Three additional pushbutton switches serve as the Somfy UP, DOWN, and MY buttons.  These are optional and only need to be installed if you want to control a window shade or screen manually with pushbuttons in addition to using HomeKit.  The final pushbutton is used as the HomeSpan control button, and is also optional since all of its functions can be accessed from the Arduino Serial Monitor if needed.

Each pushbutton used should be installed to connect a particular ESP32 pin to ground.  HomeSpan takes case of debouncing the switches so no additional hardware is needed.  The pin definitions are defined in the sketch as follows:

```C++
// Assign pins for the physical Somfy pushbuttons

#define PROG_BUTTON   17      // must have a button to enable programming remote
#define UP_BUTTON     26      // button is optional
#define MY_BUTTON     25      // button is optional
#define DOWN_BUTTON   23      // button is optional
```

You can of course choose your own pins for any button provided you update the definitions accordingly.

HomeSpan uses GPIO pin 21 as the default for connecting the HomeSpan Control Button, but this too can be changed by calling `homeSpan.setControlPin(pin)` somewhere at the top of the sketch *before* the call to `homeSpan.begin()`.

## Configuring the Software

Apart from possibly changing the default pin definitions above, the only configuration required is to instantiate a Somfy Service for each window shade or screen you want to control, and specify a few parameters, using the CREATE_SOMFY macro:

`CREATE_SOMFY(uint32_t channel, uint32_t raiseTime, uint32_t lowerTime`

* *channel* - the channel number assigned to the window shade or screen.  Must be greater than zero and less than 2^32-1
* *raiseTime* - the time it takes (in milliseconds) for the shade or screen to raise from fully closed to fully open
* *lowerTime* - the time it takes (in milliseconds) for the shade or screen to lower from fully open to fully closed

Call CREATE_SOMFY for each shade or screen you want to control with HomeSpan Somfy as such:

```C++
CREATE_CHANNEL(1,21000,19000);          // assign a shade to Somfy Channel #1 with raiseTime=21000 ms and lowerTime=19000 ms
CREATE_CHANNEL(2,11000,10000);          // assign a shade to Somfy Channel #2 with raiseTime=11000 ms and lowerTime=10000 ms
CREATE_CHANNEL(607,11000,10000);        // assign a shade to Somfy Channel #607 (channel numbers do not need to be consecutive)
CREATE_CHANNEL(14,11000,10000);         // assign a shade to Somfy Channel #14 (channel numbers do not need to be in any order)
CREATE_CHANNEL(0,11000,10000);          // BAD! Cannot use zero as a channel number
```

You can add, remove, or modify your channel configuration at any time, even after HomeSpan Somfy has been paired with HomeKit.  Changes you make will automatically be reflected in the Home App on your iOS device.

## Operating your HomeSpan Somfy Device

HomeSpan Somfy is designed to operate just as any Somfy multi-channel remote, with the one exception that there are no LEDs or LCD displays to indicate which channel you have selected.  Instead, HomeSpan Somfy visually indicates the selected channel from within the Home App itself.  If you only have instantiated a single channel, there is nothing you need to select.  But if you created more than one channel, your next steps are to connect the device to your WiFi network and then pair the the device to HomeKit.  To so, follow the general instructions for all HomeSpan devices and configure the device either using the [HomeSpan Command Line Interface](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md) or using the HomeSpan Control Button (if you've installed one above) as described in the [HomeSpan Users Guide](https://github.com/HomeSpan/HomeSpan/blob/master/docs/UserGuide.md).


