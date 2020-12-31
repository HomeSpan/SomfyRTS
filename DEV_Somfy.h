
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

#include "extras/RFControl.h"
#include "RFM69.h"
#include <sodium.h>
#include <nvs_flash.h>

char cBuf[128];                       // general string buffer for formatting output when needed

PushButton progButton(PROG_BUTTON);
PushButton upButton(UP_BUTTON);
PushButton myButton(MY_BUTTON);
PushButton downButton(DOWN_BUTTON);

#define RF_FREQUENCY  433.42    // RF frequency (in MHz) for Somfy-RTS system

#define SOMFY_STOP    0   
#define SOMFY_RAISE   1
#define SOMFY_LOWER   2
#define SOMFY_PROGRAM 3

#define DISPLAY_NAME_FORMAT      "Channel-%lu"
#define DISPLAY_ADDRESS_FORMAT   "RTS-%06X"

const char *label[]={"STOPPING","RAISING","LOWERING","PROGRAMMING"};

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
  char *sName;
  char *sAddr;
  uint16_t rollingCode=0xFF;                   // arbitrary starting code

  static vector<DEV_Somfy *> shadeList;        // store a list of all shades so we can scroll through selection with progButton
  static int selectedShade;                    // selected shade in shadeList
  static unsigned char hashKey[16];            // key for hashing channel numbers into Somfy addresses
  static nvs_handle somfyNVS;                  // handle to NVS to store rolling codes and hashKey for addresses

//////////////////////////////////////
  
  DEV_Somfy(uint32_t address, char *displayName, char *displayAddress, uint32_t raiseTime, uint32_t lowerTime) : Service::WindowCovering(){       // constructor() method

    this->address=address;                             // Somfy address (only lower 3 bytes)
    this->raiseTime=raiseTime;                         // time (in milliseconds) to fully open
    this->lowerTime=lowerTime;                         // time (in milliseconds) to fully close
    current=new Characteristic::CurrentPosition(0);    // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition(0);      // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    
    indicator=new Characteristic::ObstructionDetected(0);     // use this as a flag to indicate to user that this channel has been selected

    sAddr=displayAddress;
    sName=displayName;
    
    nvs_get_u16(somfyNVS,sAddr,&rollingCode);          // get rolling code for this Somfy address, if it exists (otherwise initialized value above is used)
   
    sprintf(cBuf,"Configuring Somfy Window Shade %s (%s):  RollingCode=%04X  RaiseTime=%d ms  LowerTime=%d ms\n",sName,sAddr,rollingCode,this->raiseTime,this->lowerTime);
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
        sprintf(cBuf,"** Somfy %s: Fully Open\n",sName);
        LOG1(cBuf);
      } else if(targetPosition<0){
        sprintf(cBuf,"** Somfy %s: Fully Closed\n",sName);
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
  
    sprintf(cBuf,"** Somfy %s: %s  RC=%04X\n",sName,label[action],++rollingCode);
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
         
    rf.start(3,1);                    // start transmission!  Repeat 3 times; Tick size=1 microseconds

    rfm69.setRegister(0x01,0x04);                 // re-enter stand-by mode

    nvs_set_u16(somfyNVS,sAddr,rollingCode);      // save updated rolling
       
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
        sprintf(cBuf,"** Somfy %s: Selected\n",ss->sName);
        LOG1(cBuf);
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
  
////////////////////////////////////

  static void init(){
    nvs_open("SOMFY_DATA",NVS_READWRITE,&somfyNVS);

    size_t len;
    
    if(!nvs_get_blob(somfyNVS,"HASHKEY",NULL,&len)){       // if found HASH KEY for Somfy addresses
      nvs_get_blob(somfyNVS,"HASHKEY",hashKey,&len);       // retrieve HASH KEY
    } else {
      Serial.print("Creating new Hash Key for Somfy Controller Addresses\n");
      crypto_shorthash_keygen(hashKey);
      nvs_set_blob(somfyNVS,"HASHKEY",hashKey,sizeof(hashKey));
      nvs_commit(somfyNVS);
    }   

    rfm69.init();
    rfm69.setFrequency(RF_FREQUENCY);
    
  } // init
  
////////////////////////////////////

  static uint32_t createAddress(uint32_t channelNum){     // create a 3-byte Somfy RTS address from channel number

  uint32_t code[2];
  
  crypto_shorthash((unsigned char *)&code, (unsigned char *)&channelNum, 4, hashKey);
  return(code[0]&0xFFFFFF);
  }

////////////////////////////////////

  static char *createName(char *format, uint32_t val){

  int nChars=snprintf(NULL,0,format,val);
  char *cBuf=(char *)malloc(nChars+1);
  sprintf(cBuf,format,val);
  return(cBuf);    
  }

////////////////////////////////////

}; // DEV_Somfy()

////////////////////////////////////

#define CREATE_CHANNEL(channelNum,raiseTime,lowerTime) { \
  uint32_t address=DEV_Somfy::createAddress(channelNum); \
  char *displayName=DEV_Somfy::createName(DISPLAY_NAME_FORMAT,channelNum); \
  char *displayAddress=DEV_Somfy::createName(DISPLAY_ADDRESS_FORMAT,address); \  
  new SpanAccessory(channelNum+1); \
  new DEV_Identify(displayName, "HomeSpan", displayAddress, displayName, SKETCH_VERSION, 0); \
  new DEV_Somfy(address,displayName,displayAddress,raiseTime,lowerTime); \
}

////////////////////////////////////

vector<DEV_Somfy *> DEV_Somfy::shadeList;
int DEV_Somfy::selectedShade=0;
unsigned char DEV_Somfy::hashKey[16];
nvs_handle DEV_Somfy::somfyNVS;
