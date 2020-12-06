
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

struct DEV_Somfy : Service::WindowCovering {    

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *selected;
   
  boolean moving=false; 
  uint8_t channel;
  
  DEV_Somfy(uint8_t channel) : Service::WindowCovering(){       // constructor() method

    this->channel=channel;
    current=new Characteristic::CurrentPosition();     // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition();       // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    new SpanRange(0,100,50);                           // set the allowable target-position range to 0-100 in steps of 50 (allowable values are thus 0, 50, and 100)
    
    selected=new Characteristic::ObstructionDetected();     // use this as a flag to indicate to user that this channel has been selected for deletion
       
    Serial.print("Configuring Somfy Window Shade: Channel=");   // initialization message
    Serial.print(channel+1);
    Serial.print("\n");

  } // end constructor

  boolean update(){                              // update() method

    LOG1("Somfy Channel ");
    LOG1(channel+1);
    LOG1(": ");

    lastChannel=channel;

    switch(target->getNewVal()){
      case 0:
        LOG1("Lowering Shade\n");
        moving=true;
      break;

      case 50:
        LOG1("Setting Shade to MY position\n");
        moving=true;
      break;

      case 100:
        LOG1("Raising Shade\n");
        moving=true;
      break;
    }
        
    return(true);
  
  } // update

  void loop(){                                   // loop() method

    if(moving && target->timeVal()>2000){
      moving=false;
      current->setVal(target->getVal());
    }
    
  } // loop
  
};

////////////////////////////////////
