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
#define DOWN_BUTTON   23      // button is optional

// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69

#define SKETCH_VERSION  "1.0.1"       // version of the Homespan SomfyRTS sketch
#define REQUIRED VERSION(1,1,2)       // required version of the HomeSpan Library

#include "HomeSpan.h" 
#include "DEV_Identify.h"       
#include "DEV_Somfy.h"

void setup() {
 
  Serial.begin(115200);

  homeSpan.setLogLevel(1);

  homeSpan.begin(Category::Bridges,"Somfy-HomeSpan");

  DEV_Somfy::init();

  new SpanAccessory(1);  
    new DEV_Identify("Somfy Controller","HomeSpan","123-ABC","Multi-Channel RTS",SKETCH_VERSION,3);
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");

  CREATE_CHANNEL(1,21000,19000);          // add Somfy Channel #1 with raiseTime=21000 ms and lowerTime=19000ms
//  CREATE_CHANNEL(327,9000,8000);          // add Somfy Channel #327 with raiseTime=9000 ms and lowerTime=8000ms
     
} // end of setup()

//////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  DEV_Somfy::poll();
  
} // end of loop()
