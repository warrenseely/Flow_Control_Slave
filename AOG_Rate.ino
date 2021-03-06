#include <ppmreader.h>
  
  #define LED_PIN 13   
  //#define   DIR_PIN    12  //PB4
  //#define   PWM_PIN    11  //PB3  
  //#define WORKSW_PIN A4  //PD4

  #define pressure_Pin A7
  #define valvePin 3
  #define valvedirection 4
  #define flowinPin 2 //bound to digital pin 2 and interrupt 0
  #define min_pressure 1.4 //1.4bar min pressure ~ 20.3 psi
  #define max_pressure 6 //6 bar max pressure ~ 87.5 psi
  
  #define RELAY1_PIN 5  //
  #define RELAY2_PIN 6  //
  #define RELAY3_PIN 7  //
  #define RELAY4_PIN 8  //
  #define RELAY5_PIN 9
  #define RELAY6_PIN 10
  #define RELAY7_PIN 11
  #define RELAY8_PIN 12  
  #define RelayPowerPin A5 //PC5
  #define rtFenceRowOut A4 //PC4
  #define ltFenceRowOut A3 //PC3
  #define rtFenceRowIn A2
  #define ltFenceRowIn A1
   
  //loop time variables in microseconds
  const unsigned int LOOP_TIME = 50; //20hz; 50 
  unsigned int lastTime = LOOP_TIME;
  unsigned int currentTime = LOOP_TIME;
  unsigned int dT = 50000;
  unsigned int count = 0;
  unsigned int watchdogTimer = 0;
  int flag = 0, rate_delay = 0, flag1 = 0;
  
  //program flow
  bool isDataFound = false, isSettingFound = false;
  int header = 0, tempHeader = 0, temp;

  byte relay = 0, fencerows = 0;
  float rateSetPoint = 0, groundSpeed = 0, accumulatedVolume = 90, litersPerMinFlow = 0; //speed from AgOpenGPS is multiplied by 4
  float calibrationFactor = 37.6, adjust_time = 0;                                 //calibrationFactor is the rated pulses per unit of measure(in my case, Raven 60P flowmeter is 720 pulses per 10gal)
  float reference_pressure = 40, reference_Flow = 0.75708; //from chart                  //This means that for liters we have: 72 / 3.78541 = 19.02 pulses per liter
                                                                                         //We are also counting flow meter pulses on a CHANGE so we will have X2 counted pulses per liter = 38.0407754
  
  float kp = 1000, oldtime1 = 0, pressure = 0, litersPerMinPressure = 0;
  //unsigned int  pulseCount = 0;
  byte sensorInterrupt = 0;  // 0 = digital pin 2
  
  PPMReader PPMReader; //create the ppm class

void setup()
{
  pinMode(pressure_Pin, INPUT); //reads the pressure transducer
  pinMode(valvePin, OUTPUT); //turns 3way valve on and off
  digitalWrite(valvePin, 1); //TESTING*******************
  pinMode(valvedirection, OUTPUT); //direction of the 3way valve
  pinMode(flowinPin, INPUT); //reads teh flowmeter
  digitalWrite(flowinPin, HIGH); //trips on a change
  
  pinMode(LED_PIN, OUTPUT); //configure LED for output
  pinMode(RELAY1_PIN, OUTPUT); //configure RELAY1 for output //Pin 5
  pinMode(RELAY2_PIN, OUTPUT); //configure RELAY2 for output //Pin 6
  pinMode(RELAY3_PIN, OUTPUT); //configure RELAY3 for output //Pin 7
  pinMode(RELAY4_PIN, OUTPUT); //configure RELAY4 for output //Pin 8
  pinMode(RELAY5_PIN, OUTPUT); //configure RELAY5 for output //Pin 9
  pinMode(RELAY6_PIN, OUTPUT); //configure RELAY6 for output //Pin 10
  pinMode(RELAY7_PIN, OUTPUT); //configure RELAY7 for output //Pin 11
  pinMode(RELAY8_PIN, OUTPUT); //configure RELAY8 for output //Pin 12
  
  digitalWrite((PORTD & B11100000), HIGH); //Set all Port D relay outputs to HIGH so relays are OFF. Pull LOW to activate valve relay
  digitalWrite((PORTB & B11111), HIGH); // Set all Port B relay outputs to HIGH so relays are OFF. Pull LOW to activate valve relay

  /*Relay boards are an active LOW so a main power control ensures power must be on for valves to actuate*/
  pinMode(RelayPowerPin, OUTPUT); //main relay power control
  digitalWrite(RelayPowerPin, HIGH); //ensure power relay is on AFTER setting valve relay pins HIGH so valves are OFF

  pinMode(rtFenceRowOut, OUTPUT); //Fencerow out control
  pinMode(ltFenceRowOut, OUTPUT);
  pinMode(rtFenceRowIn, INPUT); //fencerow in controls
  pinMode(ltFenceRowIn, INPUT);
  
  //set up communication  
  Serial.begin(38400); 
    
  //PWM rate settings Adjust to desired PWM Rate
  //TCCR1B = TCCR1B & B11111000 | B00000010;    // set timer 1 divisor to     8 for PWM frequency of  3921.16 Hz
  TCCR1B = TCCR1B & B11111000 | B00000011;    // set timer 1 divisor to    64 for PWM frequency of   490.20 Hz (The DEFAULT)

  attachInterrupt(sensorInterrupt, pulseCounter, CHANGE); //read the flowmeter pulses on change
} 

void loop()
{ 
  currentTime = millis();
  unsigned int time = currentTime;

  if (currentTime - lastTime >= LOOP_TIME) //read flow every 50 ms  
  {   
    dT = currentTime - lastTime;
    lastTime = currentTime;

    //fencerows = (digitalRead(rtFenceRowIn) << 1) | digitalRead(ltFenceRowIn); //get the state of A1 and A2
    
    SetRelays(); //turn on/off sections and fencerows  
    
    //if((!flag && (relay & 255) && (millis() - rate_delay >= 3000)) || fencerows) //IF we are not coming directly from an "all off" condition and at least one relay is on OR a fencerow is on
    //{
      litersPerMinFlow = PPMReader.get_ppm() / calibrationFactor; //get the average liters per minute since last checked
      //adjust_time = kp * abs(rateSetPoint - litersPerMinFlow); //time to adjust valve is (kp milliseconds) * (difference in target rate vs applied rate)
      //flag1 = 1; //this ensures we are not always writing to turn the valve off outside of the timed loop even after the valve has been shut off
    //}
    //else if (relay == 0) //all relays off
//    {
//      flag = 1; //set flag so we know we are coming from an "all off" condition
//      adjust_time = 0; //no need for valve movement when all are off
//      //maybe put a 3way valve open condition here to allieviate pressure spike from all sections coming off
//    }
    //else
//    {
//      rate_delay = millis(); //preserve the current time since want to wait a couple seconds after flipping all sections off for rate to stabilize
//      adjust_time = 0;
//      flag = 0; //reset flag
//    }
                                                                                           
    //digitalWrite(valvePin, 0); //make sure valve is shut off while we change directions to avoid damage to H-bridge
    
//    if((litersPerMinFlow > rateSetPoint) && (pressure >= min_pressure)) //putting on too much product, adjust valve only if it won't bring system pressure down too far
//    {
//      digitalWrite(valvedirection, 1); //set valve direction to close
//      digitalWrite(valvePin, 1); //start the valve adjusting
//    }
//    else if ((litersPerMinFlow < rateSetPoint) && (pressure < max_pressure)) //not applying enough product, adjust valve only if it won't bring system pressure too high
//    {
//      digitalWrite(valvedirection, 0); //set valve direction to open
//      digitalWrite(valvePin, 1); //start the valve adjusting    
//    }
//    else
//    {
//      digitalWrite(valvePin, 0); //rate is correct, pressure is correct, do nothing, stop valve
//    }

    //oldtime1 = millis(); //preserve the last time we checked and changed valve
    
      //20 = 1 second
      //200 = 10 seconds
      //240 = 12 seconds ~ when rate = 0
    if (count++ > 20) //maybe make the 20 a variable to introduce rate "smoothing" ?
    {
      PPMReader.reset();
      //Send to agopenGPS, once per second
      Serial.print(dT); 
      Serial.print(",");
      Serial.print(groundSpeed); 
      Serial.print(","); 
      Serial.print(litersPerMinFlow); //liters per minute rate being output
      Serial.print(","); 
      Serial.println(accumulatedVolume); 
      //Serial.print(","); 
      //Serial.println(pressure);
      
      Serial.flush();   // flush out buffer
      count = 0;
    }
    
    if (watchdogTimer++ > 10)
    {
      //here goes the code to shut everything down, AgOpenGPS went asleep, lost communication
      relay = 0;
      SetRelays();        
    }
      
  } //end of timed loop
  
  //need to convert the 0-1023 reading from AnalogRead to a bar reading
  //Raven and Ag Leader sensors are 16mv/psi OR 230mv/bar
  //Dickey-John sensors are 45mv/psi OR 650 mv/bar

  /*The analog reading goes from 0 to 1023 for (in this case) a 5 volt supply. 
   * The sensor delivers 1 volt for 0 bar and 5 volts for 17 bar according to the data sheet.
   * 1 volt is an analog reading of 204  (rounded) for 0 bar
   * 5 volts is an analog reading of 1023  (rounded) for 17 bar
    * Pressure (bar) = ( Analog Reading - 204 ) * 17 /  ( 1023 - 204 )*/
    //pressure = ((analogRead(pressure_Pin) - 204) * 17 / (1023 - 204));

    //litersPerMinPressure = reference_Flow * (sqrt(pressure) / sqrt(reference_pressure)); //see your preferred spray nozzle chart for the reference flow 
                                                                                         //and pressure for your chosen nozzle. All units in metric.  
    
//  if(((millis() - oldtime1)  >= adjust_time) && (flag1 == 1)) //shut the valve off. may not be needed??
//  {
//    digitalWrite(valvePin, 0); //shut the valve off
//    flag1 = 0; //reset flag
//  }
     
  
  
  
    //****************************************************************************************
    //This runs continuously, outside of the timed loop, keeps checking UART for new data  
    // header high/low, relay byte, speed byte, rateSetPoint hi/lo
    if (Serial.available() > 0 && !isDataFound && !isSettingFound) //find the header, 127H + 250L = 32762
    {
      int temp = Serial.read();
      header = tempHeader << 8 | temp;               //high,low bytes to make int
      tempHeader = temp;                             //save for next time
      if (header == 32762) isDataFound = true;     //Do we have a match? 
      if (header == 32760) isSettingFound = true;     //Do we have a match? 
    }
    
    //Data Header has been found, so the next 4 bytes are the data
    if (Serial.available()> 3 && isDataFound)
    {  
      isDataFound = false;    
      relay = Serial.read();   // read relay control from AgOpenGPS     
      groundSpeed = Serial.read()>>2;  //actual speed times 4, single byte
  
      // sent as 10 times value in liters per minute, LPM * 0.264172 to get US GPM
      rateSetPoint = ((float)(Serial.read() << 8 | Serial.read()))* 0.1;   //high,low bytes

      //reset watchdog as we just heard from AgOpenGPS
      watchdogTimer = 0;
    }  
  
    //Settings Header has been found, 8 bytes are the settings
    if (Serial.available() > 7 && isSettingFound)
    {        
      isSettingFound = false;  //reset the flag
  
      //change the factors as required for your own PID values
      //steerSensorCounts = Serial.read()*0.1; //sent as 10 times the setting displayed in AOG
    }
}

//ISR
void pulseCounter()
{
  // Increment the pulse counter
  //pulseCount++;
  PPMReader.on_trigger();
}
