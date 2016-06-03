// const variables
const int DEFAULT_PEAK = 600;
const int DEFAULT_MIN = 400;
const int DEFAULT_THRESH = 480;
const int MIN_BEAT_INTERVAL = 330;

volatile boolean Pulse[6] = {false, false, false, false, false, false};     // "True" when we're inside a single beat. (false between beats)

volatile unsigned long last_beat_time[6] = {0, 0, 0, 0, 0, 0};           // used to find IBI
volatile int Peak[6];                      // used to find peak in pulse wave, seeded
volatile int T_min[6];                     // used to find trough in pulse wave, seeded
volatile int thresh[6];                // used to find instant moment of heart beat, seeded


// declaring local variable as global to save re-declaration time
volatile unsigned long last_beat_interval = 0;
volatile int Signal;                // holds the incoming raw data
volatile bool already_reset[6] = {false, false, false, false, false, false};

void interruptSetup(){    
  Timer1.initialize(2000);                      // a timer of 2000 microseconds
  Timer1.attachInterrupt(get_sensor_readings);  // get readings from sensors every 2 ms   
} 


void reset_all(int sensor){
  if (not sound) {
    Serial.print("\nreset ALL ");
    Serial.println("A" + String(pulsePin[sensor]));
  }
  already_reset[sensor] = true;
  thresh[sensor] = DEFAULT_THRESH;                          // set thresh default
  Peak[sensor] = DEFAULT_PEAK;                               // set Peak default
  T_min[sensor] = DEFAULT_MIN;                               // set T default
  last_beat_time[sensor] = sample_time;          // bring the last_beat_time up to date, to avoid constant resets.
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
  bool sensor_enabled = not use_pressure_sensors || (analogRead(pressure_sensor_pin[sensor]) > 80);
  if (not sensor_enabled){
    /* sensor is disabled due to no hand on it - reset, log and return */
    handle_disabled_sensor(sensor);
    return;
  }
  Signal = analogRead(pulsePin[sensor]);              // read the Pulse Sensor 
  //Serial.print(Signal);
  // FIXME!
  if (verbose && sample_time % 25 == 0) { /* last sample time, but who cares */
    serialOutput(Signal);
    /* Serial.println("thres: " + String(thresh[sensor]) + " high: " + String(Peak[sensor]) + " low: " + String(T_min[sensor])); */
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
  if (last_beat_interval > MIN_BEAT_INTERVAL) {                                   // avoid high frequency noise
    if (not Pulse[sensor] && Signal > thresh[sensor]) {
      Pulse[sensor] = true;                               // set the Pulse flag when we think there is a pulse
      /* digitalWrite(blinkPin[sensor], HIGH);                // turn on Blinkpin */
      last_beat_time[sensor] = sample_time;               // keep track of time for next pulse

      QS[sensor] = true;                              // set Quantified Self flag, NOT CLEARED INSIDE THE ISR
      if (not sound && verbose) {
        Serial.print("setting QS for sensor ");
        Serial.print(String(pulsePin[sensor]));
        Serial.print(" - Signal: ");
        Serial.print(Signal);
        Serial.print(" - T_min[sensor]");
        Serial.print(T_min[sensor]);
        Serial.print(" - Peak[sensor]");
        Serial.print(Peak[sensor]);
        Serial.print(" - thresh[sensor]");
        Serial.println(thresh[sensor]);
      }
      
    } else {
      /* no pulse identified right now */
      
      if (Signal < thresh[sensor] && Pulse[sensor]){
	/* when the values are going down, the beat is over */
	/* digitalWrite(blinkPin[sensor], LOW);            // turn off blink pin */
	Pulse[sensor] = false;                         // reset the Pulse flag so we can do it again
	thresh[sensor] = Peak[sensor] - (Peak[sensor] - T_min[sensor]) / 3;                    // set thresh at 50% of the amplitude
	/* Peak[sensor] = (Peak[sensor] + thresh[sensor]) / 2; */
	Peak[sensor] = Peak[sensor] - (Peak[sensor] - thresh[sensor]) / 3;                            // reset these for next time
	T_min[sensor] = T_min[sensor] + (thresh[sensor] - T_min[sensor]) / 3;
	already_reset[sensor] = false;
      }
    
      if (not already_reset[sensor] && last_beat_interval > 2500){                           // if 2.5 seconds go by without a beat
	if (not sound && verbose) {
	  Serial.println("2.5 seconds since last beat for A" + String(pulsePin[sensor]) + " - reseting it.");
	  Serial.println("last_beat_time[A" + String(pulsePin[sensor]) +"] = " + String(last_beat_time[sensor]));
	  Serial.println("thres: " + String(thresh[sensor]) + " high: " + String(Peak[sensor]) + " low: " + String(T_min[sensor]));
        }
        reset_all(sensor);
      }
    }
  }
}

void handle_disabled_sensor(int sensor) {
  /* Signal = -1; */
  return;
  if (not already_reset[sensor]) {
    if (not sound && verbose) {
      Serial.println("Sensor A" + String(pulsePin[sensor]) + " is disabled - reseting it.");
      Serial.println("thres: " + String(thresh[sensor]) + " high: " + String(Peak[sensor]) + " low: " + String(T_min[sensor]));
    }
    //reset_all(sensor);
  }
}
