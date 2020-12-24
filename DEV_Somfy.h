
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

char cBuf[128];                       // general string buffer for formatting output when needed

nvs_handle somfyNVS;                  // handle to NVS to store rolling codes

PushButton progButton(17);
PushButton upButton(26);
PushButton myButton(25);
PushButton downButton(21);

#define SOMFY_STOP    0   
#define SOMFY_RAISE   1
#define SOMFY_LOWER   2
#define SOMFY_PROGRAM 3

char *label[]={"STOPPING","RAISING","LOWERING","PROGRAMMING"};

struct DEV_Somfy : Service::WindowCovering {    

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *indicator;
   
  double velocity=0;
  uint32_t startTime=0;
  uint32_t ocTime;
  uint32_t address;
  char sAddr[11];
  uint16_t rollingCode=0xFF;                   // arbitrary starting code

  static vector<DEV_Somfy *> shadeList;        // store a list of all shades so we can scroll through selection with progButton
  static int selectedShade;                    // selected shade in shadeList
  
  DEV_Somfy(uint32_t address, uint32_t ocTime) : Service::WindowCovering(){       // constructor() method

    this->address=address&0xFFFFFF;                    // Somfy address (use only lower 3 bytes)
    this->ocTime=ocTime;                               // time (in milliseconds) to change from fully open or fully close
    current=new Characteristic::CurrentPosition(0);    // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition(0);      // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    new SpanRange(0,100,10);
    
    indicator=new Characteristic::ObstructionDetected(0);     // use this as a flag to indicate to user that this channel has been selected

    sprintf(sAddr,"RTS-%02X%02X%02X", this->address>>16 & 0xFF, this->address>>8 & 0xFF, this->address & 0xFF);

    size_t len;
    
    if(!nvs_get_blob(somfyNVS,sAddr,NULL,&len)){                        // Somfy address data found
      nvs_get_blob(somfyNVS,sAddr,&rollingCode,&len);                   // use existing rolling code
    } else {                                                            // new Somfy address
      nvs_set_blob(somfyNVS,sAddr,&rollingCode,sizeof(rollingCode));    // set rolling code to starting code
      nvs_commit(somfyNVS);
    }

    sprintf(cBuf,"Configuring Somfy Window Shade %s:  RollingCode=%04X  OpenCloseTime=%d ms\n",this->sAddr,rollingCode,this->ocTime);
    Serial.print(cBuf);

    shadeList.push_back(this);

  } // end constructor

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
        
      velocity=100.0/ocTime;
      startTime=millis();
      
    } else
    
    if(target->getNewVal() < estimatedPosition && velocity>=0){

      transmit(SOMFY_LOWER);

      if(velocity>0)
        current->setVal(estimatedPosition);
        
      velocity=-100.0/ocTime;
      startTime=millis();
    }
        
    return(true);
  
  } // update

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

      if(targetPosition<100 && targetPosition>0)      // only transmit stop signal if stopping midway (neither fully open or closed)
        transmit(SOMFY_STOP);
      
      current->setVal(target->getVal());
      velocity=0;
    }
    
  } // loop

  void transmit(uint8_t action){

    sprintf(cBuf,"** Somfy %s: %s  RC=%04X\n",sAddr,label[action],rollingCode++);
    Serial.print(cBuf);

    nvs_set_blob(somfyNVS,sAddr,&rollingCode,sizeof(rollingCode));    // save new rolling code
    nvs_commit(somfyNVS);
    
  } // transmit

  static void poll(){

    if(progButton.triggered(5,4000)){

      Serial.println("HERE");
      Serial.println(selectedShade);
      Serial.println(shadeList.size());
      
      if(progButton.type()==PushButton::SINGLE){
        if(selectedShade!=-1)
          shadeList[selectedShade]->indicator->setVal(0);
        selectedShade++;
        if(selectedShade==shadeList.size())
          selectedShade=-1;
        if(selectedShade!=-1){
          Serial.println("There");
          shadeList[selectedShade]->indicator->setVal(1);
        }
        
      } // SINGLE
      
    } // PROG BUTTON

  } // poll
  
};

vector<DEV_Somfy *> DEV_Somfy::shadeList;
int DEV_Somfy::selectedShade=-1;

////////////////////////////////////
