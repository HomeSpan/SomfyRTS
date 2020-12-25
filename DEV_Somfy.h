
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

#include "extras/RFControl.h"
#include "RFM69.h"

char cBuf[128];                       // general string buffer for formatting output when needed

nvs_handle somfyNVS;                  // handle to NVS to store rolling codes

PushButton progButton(PROG_BUTTON);
PushButton upButton(UP_BUTTON);
PushButton myButton(MY_BUTTON);
PushButton downButton(DOWN_BUTTON);

#define SOMFY_STOP    0   
#define SOMFY_RAISE   1
#define SOMFY_LOWER   2
#define SOMFY_PROGRAM 3

char *label[]={"STOPPING","RAISING","LOWERING","PROGRAMMING"};

RFControl rf(RFM_SIGNAL_PIN);
RFM69 rfm69(RFM_CHIP_SELECT,RFM_RESET_PIN);

//////////////////////////////////////

struct DEV_Somfy : Service::WindowCovering {    

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *indicator;
   
  double velocity=0;
  uint32_t startTime=0;
  uint32_t raiseTime;
  uint32_t lowerTime;
  uint32_t address;
  char sAddr[11];
  uint16_t rollingCode=0xFF;                   // arbitrary starting code

  static vector<DEV_Somfy *> shadeList;        // store a list of all shades so we can scroll through selection with progButton
  static int selectedShade;                    // selected shade in shadeList

//////////////////////////////////////
  
  DEV_Somfy(uint32_t address, uint32_t raiseTime, uint32_t lowerTime) : Service::WindowCovering(){       // constructor() method

    this->address=address&0xFFFFFF;                    // Somfy address (use only lower 3 bytes)
    this->raiseTime=raiseTime;                         // time (in milliseconds) to fully open
    this->lowerTime=lowerTime;                         // time (in milliseconds) to fully close
    current=new Characteristic::CurrentPosition(0);    // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition(0);      // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    
    indicator=new Characteristic::ObstructionDetected(0);     // use this as a flag to indicate to user that this channel has been selected

    sprintf(sAddr,"RTS-%02X%02X%02X", this->address>>16 & 0xFF, this->address>>8 & 0xFF, this->address & 0xFF);

    size_t len;
    
    if(!nvs_get_blob(somfyNVS,sAddr,NULL,&len)){                        // Somfy address data found
      nvs_get_blob(somfyNVS,sAddr,&rollingCode,&len);                   // use existing rolling code
    } else {                                                            // new Somfy address
      nvs_set_blob(somfyNVS,sAddr,&rollingCode,sizeof(rollingCode));    // set rolling code to starting code
      nvs_commit(somfyNVS);
    }

    sprintf(cBuf,"Configuring Somfy Window Shade %s:  RollingCode=%04X  RaiseTime=%d ms  LowerTime=%d ms\n",this->sAddr,rollingCode,this->raiseTime,this->lowerTime);
    Serial.print(cBuf);

    shadeList.push_back(this);

  } // end constructor

//////////////////////////////////////

  boolean update(){                              // update() method

    int estimatedPosition;

    estimatedPosition=current->getVal<double>()+velocity*double(millis()-startTime);
    if(estimatedPosition>100)
      estimatedPosition=100;
    if(estimatedPosition<0)
      estimatedPosition=0;    
      
    if(target->getNewVal() > estimatedPosition && velocity<=0){

      transmit(SOMFY_RAISE);
           
      if(velocity<0)
        current->setVal(estimatedPosition);
        
      velocity=100.0/raiseTime;
      startTime=millis();
      
    } else
    
    if(target->getNewVal() < estimatedPosition && velocity>=0){

      transmit(SOMFY_LOWER);

      if(velocity>0)
        current->setVal(estimatedPosition);
        
      velocity=-100.0/lowerTime;
      startTime=millis();
    }
        
    return(true);
  
  } // update

//////////////////////////////////////

  void loop(){                                   // loop() method

    if(velocity==0)
      return;

    int estimatedPosition=current->getVal<double>()+velocity*double(millis()-startTime);

    int targetPosition=target->getVal();

    if(targetPosition==100)         // if request for fully open or fully close, overshoot target to make sure shade really is fully open or closed
      targetPosition=120;           
    if(targetPosition==0)
      targetPosition=-20;
    
    if((velocity>0 && estimatedPosition > targetPosition) || (velocity<0 && estimatedPosition < targetPosition)){

      if(targetPosition>100){
        sprintf(cBuf,"** Somfy %s: Fully Open\n",sAddr);
        LOG1(cBuf);
      } else if(targetPosition<0){
        sprintf(cBuf,"** Somfy %s: Fully Closed\n",sAddr);
        LOG1(cBuf);
      } else {
        transmit(SOMFY_STOP);
      }
      
      current->setVal(target->getVal());
      velocity=0;
    }
    
  } // loop

//////////////////////////////////////

  void transmit(uint8_t action){

    rfm69.setRegister(0x01,0x0c);           // enable transmission mode
    delay(10);
  
    sprintf(cBuf,"** Somfy %s: %s  RC=%04X\n",sAddr,label[action],++rollingCode);
    LOG1(cBuf);

    uint8_t b[7];
  
    b[0]=0xA0;
    b[1]=1<<(4+action);
    b[2]=rollingCode >> 8;
    b[3]=rollingCode & 0xFF;
    b[4]=(address >> 16) & 0xFF;
    b[5]=(address >> 8) & 0xFF;
    b[6]=(address) & 0xFF;
  
    uint8_t checkSum=0; 
    for(int i=0;i<7;i++)
     checkSum ^= b[i] ^ (b[i] >> 4);
     
    b[1] |= checkSum & 0x0F;  
  
    char c[64];
    sprintf(c,"Transmitting: %02X %02X %02X %02X %02X %02X %02X\n",b[0],b[1],b[2],b[3],b[4],b[5],b[6]);
    LOG2(c);
  
    for(int i=1;i<7;i++)
      b[i] ^= b[i-1];
  
    sprintf(c,"Obfuscated:   %02X %02X %02X %02X %02X %02X %02X\n",b[0],b[1],b[2],b[3],b[4],b[5],b[6]);
    LOG2(c);
  
    rf.clear();
    
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(4550,604);
    
    for(int i=0;i<7;i++){
      for(int j=128;j>0;j=j>>1){
        rf.phase(604,(b[i]&j)?0:1);
        rf.phase(604,(b[i]&j)?1:0);
      }
    }
  
    rf.phase(30415,0);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
         
    rf.start(3,1);

    rfm69.setRegister(0x01,0x04);           // re-enter stand-by mode

    nvs_set_blob(somfyNVS,sAddr,&rollingCode,sizeof(rollingCode));    // save new rolling code
    nvs_commit(somfyNVS);
    
  } // transmit

//////////////////////////////////////

  static void poll(){

    DEV_Somfy *ss=shadeList[selectedShade];

    if(progButton.triggered(5,4000)){
     
      if(progButton.type()==PushButton::SINGLE){
        if(ss->indicator->getVal()){
          ss->indicator->setVal(0);
          selectedShade=(selectedShade+1)%shadeList.size();
          ss=shadeList[selectedShade];
        }
        ss->indicator->setVal(1);
        return;        
      } // Single Press
      
      if(progButton.type()==PushButton::LONG){
        ss->indicator->setVal(0);
        ss->transmit(SOMFY_PROGRAM);
        return;        
      } // Long Press

    } // progButton

    if(upButton.triggered(5,1000) && upButton.type()==PushButton::SINGLE && ss->target->getVal()<100){
      ss->target->setVal(100);
      ss->indicator->setVal(0);
      ss->update();
    } else

    if(downButton.triggered(5,1000) && downButton.type()==PushButton::SINGLE && ss->target->getVal()>0){
      ss->target->setVal(0);
      ss->indicator->setVal(0);
      ss->update();
    } else

    if(myButton.triggered(5,1000) && downButton.type()==PushButton::SINGLE && ss->velocity!=0){
      ss->indicator->setVal(0);
      int estimatedPosition=ss->current->getVal<double>()+ss->velocity*double(millis()-ss->startTime);
      if(estimatedPosition>100)
        estimatedPosition=100;
      else if(estimatedPosition<0)
        estimatedPosition=0;
      ss->target->setVal(estimatedPosition);
      ss->loop();
    }

  } // poll
  
};

////////////////////////////////////

vector<DEV_Somfy *> DEV_Somfy::shadeList;
int DEV_Somfy::selectedShade=0;
