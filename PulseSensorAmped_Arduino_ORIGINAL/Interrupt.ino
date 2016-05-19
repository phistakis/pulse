 

// const variables
volatile const int DEFAULT_P = 512;
volatile const int DEFAULT_T = 512;
volatile const int DEFAULT_THRESH = 525;

volatile boolean Pulse[6] = {false, false, false, false, false, false};     // "True" when we're inside a single beat. (false between beats)

volatile unsigned long last_beat_time[6] = {0, 0, 0, 0, 0, 0};           // used to find IBI
volatile int Peak[6];                      // used to find peak in pulse wave, seeded
volatile int T_min[6];                     // used to find trough in pulse wave, seeded
volatile int thresh[6];                // used to find instant moment of heart beat, seeded


// declaring local variable as global to save re-declaration time
volatile int amp;                   // used to hold amplitude of pulse waveform, seeded
volatile unsigned long last_beat_interval = 0;
volatile int Signal;                // holds the incoming raw data
// volatile word IBI_sum, IBI_avg;
volatile bool already_reset[6] = {false, false, false, false, false, false};

void interruptSetup(){    
  Timer1.initialize(2000);                      // a timer of 2000 microseconds
  Timer1.attachInterrupt(get_sensor_readings);  // get readings from sensors every 2 ms   
} 


void reset_all(int sensor){
    Serial.print("\nreset ALL ");
    Serial.println("A" + String(pulsePin[sensor]));
    already_reset[sensor] = true;
    thresh[sensor] = DEFAULT_THRESH;                          // set thresh default
    Peak[sensor] = DEFAULT_P;                               // set Peak default
    T_min[sensor] = DEFAULT_T;                               // set T default
    last_beat_time[sensor] = sample_time;          // bring the last_beat_time up to date, to avoid constant resets.
    /* live_beat_session[sensor] = false; */
    /* QS[sensor] = false;                              // set Quantified Self flag - will be done by user code in "loop"  */
    /*
    for (int i=0; i<10; ++i) {
      rates[sensor][i] = 0;
    }
    */
  }

  
void get_sensor_readings(void) {
  cli();                                      // disable interrupts while we do this
  sample_time = millis();                     // keep track of the time in ms with this variable
  // Serial.println(String(millis()) + "ms / " + String(sample_time) + " sample_time");
  for (int sensor=0; sensor < NUM_OF_SENSORS; ++sensor) {
    handle_sensor(sensor);
  }
  sei();                                   // enable interrupts when youre done!
}

volatile int counter = 0;

void handle_sensor(int sensor){
  bool sensor_enabled = not use_switches || (digitalRead(pulse_switch[sensor]) > 0);
  if (not sensor_enabled){
    /* sensor is disabled due to no hand on it - reset, log and return */
    handle_disabled_sensor(sensor);
    return;
  }
  
  Signal = analogRead(pulsePin[sensor]);              // read the Pulse Sensor 
  //Serial.print(Signal);
  if (verbose && sample_time % 50 == 0) { /* last sample time, but who cares */
      serialOutput(Signal);
  }

  last_beat_interval = sample_time - last_beat_time[sensor];       // monitor the time since the last beat to avoid noise

  //  find the peak and trough of the pulse wave
  if (Signal < T_min[sensor]){       //  && last_beat_interval > IBI[sensor]*3/5  -- avoid dichrotic noise by waiting 3/5 of last IBI 
      T_min[sensor] = Signal;                         // keep track of lowest point in pulse wave
      already_reset[sensor] = false;
  }

  if (Signal > max(thresh[sensor], Peak[sensor])){          // thresh condition helps avoid noise. TODO: how is thresh helpful?
    Peak[sensor] = Signal;
    already_reset[sensor] = false;
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (last_beat_interval > 250) {                                   // avoid high frequency noise
    if (not Pulse[sensor] && Signal > thresh[sensor]) {
      Pulse[sensor] = true;                               // set the Pulse flag when we think there is a pulse
      digitalWrite(blinkPin[sensor], HIGH);                // turn on Blinkpin
      last_beat_time[sensor] = sample_time;               // keep track of time for next pulse

      QS[sensor] = true;                              // set Quantified Self flag, NOT CLEARED INSIDE THE ISR
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
    } else {
      /* no pulse identified right now */
      
      if (Signal < thresh[sensor] && Pulse[sensor]){
	/* when the values are going down, the beat is over */
	digitalWrite(blinkPin[sensor], LOW);            // turn off blink pin
	Pulse[sensor] = false;                         // reset the Pulse flag so we can do it again
	thresh[sensor] = (Peak[sensor] + T_min[sensor]) / 2;                    // set thresh at 50% of the amplitude
	Peak[sensor] = (Peak[sensor] +thresh[sensor]) / 2;                            // reset these for next time
	T_min[sensor] = (T_min[sensor] + thresh[sensor]) / 2;
	already_reset[sensor] = false;
      }
    
      if (not already_reset[sensor] && last_beat_interval > 2500){                           // if 2.5 seconds go by without a beat
	Serial.println("2.5 seconds since last beat for A" + String(pulsePin[sensor]) + " - reseting it.");
	Serial.println("last_beat_time[A" + String(pulsePin[sensor]) +"] = " + String(last_beat_time[sensor]));
	reset_all(sensor);
      }
    }
  }
}

void handle_disabled_sensor(int sensor) {
  /* Signal = -1; */
  if (not already_reset[sensor]){
    Serial.println("Sensor A" + String(pulsePin[sensor]) + " is disabled - reseting it.");
    reset_all(sensor);
  }
}
