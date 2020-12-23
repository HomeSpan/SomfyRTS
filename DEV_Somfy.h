
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

struct DEV_Somfy : Service::WindowCovering {    

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *selected;
   
  double velocity=0;
  uint32_t startTime=0;
  uint32_t ocTime;
  uint8_t channel;
  
  DEV_Somfy(uint8_t channel, uint32_t ocTime) : Service::WindowCovering(){       // constructor() method

    this->channel=channel;
    this->ocTime=ocTime;                               // time (in milliseconds) to change from fully open or fully close
    current=new Characteristic::CurrentPosition(0);    // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition(0);      // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    new SpanRange(0,100,10);
    
    selected=new Characteristic::ObstructionDetected();     // use this as a flag to indicate to user that this channel has been selected for deletion
       
    Serial.print("Configuring Somfy Window Shade: Channel=");   // initialization message
    Serial.print(channel+1);
    Serial.print("  Open/Close Time=");
    Serial.print(ocTime);
    Serial.print(" (ms)\n");

  } // end constructor

  boolean update(){                              // update() method

    LOG1("Somfy Channel ");
    LOG1(channel+1);
    LOG1(": ");

    lastChannel=channel;

    int estimatedPosition;

    estimatedPosition=current->getVal<double>()+velocity*double(millis()-startTime);
    if(estimatedPosition>100)
      estimatedPosition=100;
    if(estimatedPosition<0)
      estimatedPosition=0;    
      
    if(target->getNewVal() > estimatedPosition && velocity<=0){

      LOG1("Raising Shade\n");     
      
      if(velocity<0)
        current->setVal(estimatedPosition);
        
      velocity=100.0/ocTime;
      startTime=millis();
      
    } else
    
    if(target->getNewVal() < estimatedPosition && velocity>=0){

      LOG1("Lowering Shade\n");

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

      LOG1("Stopping Shade\n");
      current->setVal(target->getVal());
      velocity=0;
    }
    
  } // loop
  
};

////////////////////////////////////
