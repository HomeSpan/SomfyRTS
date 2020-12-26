/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2020 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/SomfyRTS
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/

// Assign pins for the physical Somfy pushbuttons

#define PROG_BUTTON   17      // must have a button to enable programming remote
#define UP_BUTTON     26      // button is optional
#define MY_BUTTON     25      // button is optional
#define DOWN_BUTTON   21      // button is optional

// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69

#define RF_FREQUENCY  433.42    // RF frequency (in MHz) for Somfy-RTS system

#include <nvs_flash.h>

#include "HomeSpan.h" 
#include "DEV_Identify.h"       
#include "DEV_Somfy.h"

void setup() {
 
  Serial.begin(115200);

  homeSpan.setLogLevel(1);

  homeSpan.begin(Category::Bridges,"Somfy-HomeSpan");

  rfm69.init();
  rfm69.setFrequency(RF_FREQUENCY);

  nvs_open("SOMFY_DATA",NVS_READWRITE,&somfyNVS);

  new SpanAccessory(1);  
    new DEV_Identify("Somfy Controller","HomeSpan","123-ABC","Multi-Channel RTS","1.1",3);
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");

  new SpanAccessory(2);
    new DEV_Identify("Screen Door","HomeSpan","E45A23","Somfy RTS","1.1",0);
    new DEV_Somfy(0xE45A23,21000,19000);
     
  new SpanAccessory(3);
    new DEV_Identify("Screen Door 2","HomeSpan","E45A23","Somfy RTS","1.1",0);
    new DEV_Somfy(0x838485,21000,19000);

  new SpanAccessory(4);
    new DEV_Identify("Screen Door 3","HomeSpan","E45A23","Somfy RTS","1.1",0);
    new DEV_Somfy(0x919293,21000,19000);

} // end of setup()

//////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  DEV_Somfy::poll();
  
} // end of loop()
