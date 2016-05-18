
// const variables
volatile const int DEFAULT_P = 512;
volatile const int DEFAULT_T = 512;
volatile const int DEFAULT_THRESH = 525;

volatile boolean Pulse[6] = {false, false, false, false, false, false};     // "True" when we're inside a single beat. (false between beats)
volatile boolean live_beat_session[6] = {false, false, false, false, false, false};     // "True" when we're in a beat session. (false when we lost the beat)


volatile int rates[6][10] = {                    // array to hold last ten IBI values
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
volatile unsigned long last_beat_time[6] = {0, 0, 0, 0, 0, 0};           // used to find IBI
volatile int Peak[6];                      // used to find peak in pulse wave, seeded
volatile int T_min[6];                     // used to find trough in pulse wave, seeded
volatile int thresh[6];                // used to find instant moment of heart beat, seeded


// declaring local variable as global to save re-declaration time
volatile int amp;                   // used to hold amplitude of pulse waveform, seeded
volatile unsigned long last_beat_interval = 0;
volatile int Signal;                // holds the incoming raw data
volatile word IBI_sum, IBI_avg;
volatile bool already_reset[6] = {false, false, false, false, false, false};

void interruptSetup(){    
  /* UNUSED - this is for arduino UNO and not MEGA. TimerOne is generic 
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A 
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED  */
  
  Timer1.initialize(2000);        // a timer of 2000 microseconds - should be 2ms
  Timer1.attachInterrupt(get_sensor_readings); // get readings from sensors every 2 ms   
} 


void reset_all(int sensor){
    Serial.print("\nreset ALL ");
    Serial.println(sensor);
    already_reset[sensor] = true;
    thresh[sensor] = DEFAULT_THRESH;                          // set thresh default
    Peak[sensor] = DEFAULT_P;                               // set Peak default
    T_min[sensor] = DEFAULT_T;                               // set T default
    last_beat_time[sensor] = sample_time;          // bring the last_beat_time up to date        
    live_beat_session[sensor] = false;
    QS[sensor] = false;                              // set Quantified Self flag 
    /*
    for (int i=0; i<10; ++i) {
      rates[sensor][i] = 0;
    }
    */
  }

  
void get_sensor_readings(void) {
  cli();                                      // disable interrupts while we do this
  for (int sensor=0; sensor < NUM_OF_SENSORS; ++sensor) {
    handle_sensor(sensor);
  }
  sei();                                   // enable interrupts when youre done!
}
// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE. 
// Timer 2 makes sure that we take a reading every 2 miliseconds
/*ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  for (int sensor=0; sensor < NUM_OF_SENSORS; ++sensor) {
    handle_sensor(sensor);
  }
  sei();                                   // enable interrupts when youre done!
}
*/


volatile int counter = 0;
volatile bool state = false;

void handle_sensor(int sensor){
  int should_work = true; //digitalRead(pulseSwitch[sensor]);
  if (should_work){
    if (state == false){
      state = true;
      Serial.print("yes! ");
      Serial.println(sensor);
    }
    Signal = analogRead(pulsePin[sensor]);              // read the Pulse Sensor 
  } else {
    if (state == true){
      state = false;
      Serial.print("no... :( ");
      Serial.println(sensor);
    }
    Signal = -1;
    if (not already_reset[sensor]){
      reset_all(sensor);
    }
    return;
  }
  //Serial.print(Signal);
  if (verbose && sample_time % 50 == 0) {
      serialOutput(Signal);
    }

  sample_time += 2;                         // keep track of the time in mS with this variable
  last_beat_interval = sample_time - last_beat_time[sensor];       // monitor the time since the last beat to avoid noise


    //  find the peak and trough of the pulse wave
  if(Signal < min(T_min[sensor], thresh[sensor]) && last_beat_interval > IBI[sensor]*3/5){       // avoid dichrotic noise by waiting 3/5 of last IBI
      T_min[sensor] = Signal;                         // keep track of lowest point in pulse wave 
  }

  if(Signal > max(thresh[sensor], Peak[sensor])){          // thresh condition helps avoid noise
    Peak[sensor] = Signal;
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (last_beat_interval > 250){                                   // avoid high frequency noise
    if ( not Pulse[sensor] && Signal > thresh[sensor] && last_beat_interval > (IBI[sensor]/5)*3 ){        
      Pulse[sensor] = true;                               // set the Pulse flag when we think there is a pulse
      digitalWrite(blinkPin[sensor],HIGH);                // turn on pin 13 LED
      IBI[sensor] = last_beat_interval;         // measure time between beats in mS
      last_beat_time[sensor] = sample_time;               // keep track of time for next pulse

      // this happens in the first beat of every session
      if(verbose && not live_beat_session[sensor]) {
        Serial.print("------------------------------------------------------------------first beat! ");
        Serial.println(sensor);
        Serial.println(last_beat_interval);
        Serial.println(last_beat_time[sensor]);
        Serial.println("*************** start ***************");
        Serial.println("Signal");
        Serial.println(Signal);
        Serial.println("sample_time");
        Serial.println(sample_time);
        Serial.println("T_min[sensor]");
        Serial.println(T_min[sensor]);
        Serial.println("Peak[sensor]");
        Serial.println(Peak[sensor]);
        Serial.println("thresh[sensor]");
        Serial.println(thresh[sensor]);
        Serial.println("last_beat_interval");
        Serial.println(last_beat_interval);
        Serial.println("IBI[sensor]");
        Serial.println(IBI[sensor]);
        Serial.println("*************** end ***************");
        return;                              // IBI value is unreliable so discard it
      }

    already_reset[sensor] = false;
    live_beat_session[sensor] = true;
    sei();                               // enable interrupts again
      /*
      // this happens in the second beat of every session
      if(rates[sensor][0] == rates[sensor][1] && rates[sensor][0] == 0){
        Serial.print("------------------------------------------------------------------second beat! ");
        Serial.println(sensor);
        for(int i=0; i<=9; i++){             // seed the running total to get a realisitic BPM at startup
          rates[sensor][i] = IBI[sensor];                      
        }
        IBI_avg = IBI[sensor];
      } else {
        // keep a running total of the last 10 IBI values
        Serial.println("third+ beat!");
        IBI_sum = 0;                  // clear the runningTotal variable    

        for(int i=0; i<9; ++i){                // shift data in the rate array
          rates[sensor][i] = rates[sensor][i+1];                  // and drop the oldest IBI value 
          IBI_sum += rates[sensor][i];              // add up the 9 oldest IBI values
        }
        rates[sensor][9] = IBI[sensor];                          // add the latest IBI to the rate array
        IBI_sum += rates[sensor][9];                // add the latest IBI to runningTotal
        IBI_avg = IBI_sum / 10;                     // average the last 10 IBI values 
      }

      BPM[sensor] = 60000/IBI_avg;               // how many beats can fit into a minute? that's BPM!
      */
      QS[sensor] = true;                              // set Quantified Self flag 
      if (verbose) {
        Serial.print("setting QS **************");
        Serial.println(sensor);
        Serial.print("sample_time: ");
        Serial.println(sample_time);
        Serial.print("Signal: ");
        Serial.println(Signal);
        Serial.println("T_min[sensor]");
        Serial.println(T_min[sensor]);
        Serial.println("Peak[sensor]");
        Serial.println(Peak[sensor]);
        Serial.println("thresh[sensor]");
        Serial.println(thresh[sensor]);
        Serial.print("done setting QS **************");
        Serial.println(sensor);
        }
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR

    }
  }

  if (Signal < thresh[sensor] && Pulse[sensor]){   // when the values are going down, the beat is over
    digitalWrite(blinkPin[sensor],LOW);            // turn off pin 13 LED
    Pulse[sensor] = false;                         // reset the Pulse flag so we can do it again
    thresh[sensor] = (Peak[sensor] + T_min[sensor]) / 2;                    // set thresh at 50% of the amplitude
    Peak[sensor] = (Peak[sensor] +thresh[sensor]) / 2;                            // reset these for next time
    T_min[sensor] = (T_min[sensor] + thresh[sensor]) / 2;
  }
  
  if (not already_reset[sensor] && last_beat_interval > 2500){                           // if 2.5 seconds go by without a beat
    reset_all(sensor);
  }
}

