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

int lastChannel=-1;

#include "HomeSpan.h" 
#include "DEV_Identify.h"       
#include "DEV_Somfy.h"     

#include <nvs_flash.h>

nvs_handle channelNVS;

#define NUM_CHANNELS            5
#define ADD_CHANNEL_BUTTON      23
#define DELETE_CHANNEL_BUTTON   5
#define CHANNEL_LED             17

struct { 
  uint8_t active;
  uint8_t pin;
} channelData[NUM_CHANNELS]={{0,13},{0,12},{0,27},{0,33},{0,15}};

DEV_Somfy *channels[NUM_CHANNELS];
char channelNumbers[NUM_CHANNELS][11];
char modelName[20];

PushButton addChannelButton(ADD_CHANNEL_BUTTON);
PushButton deleteChannelButton(DELETE_CHANNEL_BUTTON);
Blinker channelLED(CHANNEL_LED,1);

void setup() {
 
  Serial.begin(115200);

  homeSpan.setLogLevel(1);

  homeSpan.begin(Category::Bridges,"Somfy-HomeSpan");

  size_t len;
  nvs_open("CHANNELS",NVS_READWRITE,&channelNVS);
  if(!nvs_get_blob(channelNVS,"CHANNELDATA",NULL,&len)){                        // channel data found
    Serial.println("Found Somfy Channel Data");
    nvs_get_blob(channelNVS,"CHANNELDATA",channelData,&len);
  } else {
    Serial.println("Creating Somfy Channel Data");
    nvs_set_blob(channelNVS,"CHANNELDATA",channelData,sizeof(channelData));
    nvs_commit(channelNVS);
  }

  sprintf(modelName,"%d-Channel RTS",NUM_CHANNELS);
  new SpanAccessory();  
    new DEV_Identify("Somfy Controller","HomeSpan","123-ABC",modelName,"1.1",3);
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");

  for(int i=0;i<NUM_CHANNELS;i++){
    if(channelData[i].active){
      new SpanAccessory(i+2);
        sprintf(channelNumbers[i],"Channel-%02d",i+1);
        new DEV_Identify(channelNumbers[i],"HomeSpan",channelNumbers[i],"Somfy RTS","1.1",0);
        channels[i]=new DEV_Somfy(i);
    }
  }  

} // end of setup()

//////////////////////////////////////

void loop(){
  
  homeSpan.poll();

  if(deleteChannelButton.primed()){
    Serial.println("Delete-Channel Button Pressed...");
    channelLED.start(200,0.5,2,500);
    channels[lastChannel]->selected->setVal(true);
    return;
  }

  if(lastChannel>=0 && deleteChannelButton.triggered(3000,10000)){
    if(deleteChannelButton.type()==PushButton::SINGLE){
      Serial.println("Delete-Channel Button Cancelled");
      channelLED.off();;
      channels[lastChannel]->selected->setVal(false);
    } else {
      Serial.print("Deleting Channel ");
      Serial.print(lastChannel);
      Serial.println(" and restarting...");
      channelLED.on();
      channelData[lastChannel].active=0;
      nvs_set_blob(channelNVS,"CHANNELDATA",channelData,sizeof(channelData));
      nvs_commit(channelNVS);
      delay(2000);
      channelLED.off();
      ESP.restart();
    }
  }

  if(addChannelButton.triggered(9999,3000)){
    Serial.print("Add-Channel Button Pressed...");

    int nc;
    for(nc=0;nc<NUM_CHANNELS && channelData[nc].active;nc++);
    
    if(nc<NUM_CHANNELS){
      Serial.print("  Adding Channel: ");
      Serial.println(nc+1);
      channelLED.on();
      channelData[nc].active=1;       
      nvs_set_blob(channelNVS,"CHANNELDATA",channelData,sizeof(channelData));
      nvs_commit(channelNVS);
      delay(2000);
      channelLED.off();
      ESP.restart();
      } else {
      channelLED.start(200,0.5);
      Serial.print("  Can't add any more new channels.  Max=");
      Serial.println(NUM_CHANNELS);      
    }
    
    addChannelButton.wait();
    channelLED.off();
    addChannelButton.reset();
  } // add-channel button pressed
  
} // end of loop()
