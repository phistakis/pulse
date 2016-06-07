#include <TimerOne.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();


/*  Pulse Sensor Amped 1.4    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

    ----------------------  Notes ----------------------  ---------------------- 
    This code:
    1) Blinks an LED to User's Live Heartbeat   PIN 13
    2) Fades an LED to User's Live HeartBeat
    3) Determines BPM
    4) Prints All of the Above to Serial

    Read Me:
    https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md   
    ----------------------       ----------------------  ----------------------
*/

const int NUM_OF_SENSORS = 6;

//  pin numbers
const int pulsePin[6] =    {0, 1, 2, 3, 5, 6};                 // Pulse Sensor purple wire connected to analog pin 0 - NOTE! - pin 4 is N/A
const int pressure_sensor_pin[6] = {10,  11,  12, 13, 14, 15};                 // Pulse digital switch
const int PRESSURE_SENSOR_ZERO[6] = {200,  0,  0, 0, 0, 0};                 // Pulse digital switch
const int fade_pins[6] =   {2, 3, 4, 5, 6, 7};                  // pin to do fancy classy fading blink at each beat

volatile boolean QS[6] = {false, false, false, false, false, false};        // becomes true when Arduoino finds a beat to signal main loop to handle leds.
volatile boolean notes_on[6] = {false, false, false, false, false, false};        // flag for sounds per sensor
volatile int fade_leds_power[6] = {0, 0, 0, 0, 0, 0};                 // used to fade LED on with PWM on fadePin
volatile unsigned long sample_time = 0;          // used to determine pulse timing

// Configuration  -- Set This Up to your needs
static const boolean use_pressure_sensors = false;	/* should sensors be ignored if their pressure_sensor_pin is 0 */
static const boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 
static const boolean verbose = false;
static const boolean sound = true;
static const boolean animate_in_idle = true;  /* use fake heartbeats for animation when no one is pressing the sensors */

// Idle animation configs ("fake pulse")
static const int START_ANIMATION_AFTER_SECONDS = 3;
static const int SWITCH_ANIMATION_AFTER_SECONDS = 20;
static const float ANIMATION_PROBABILITY = 60;
static boolean animate = false;
static unsigned long fake_pulse_interval_ms[6] = {100000l, 100000l, 100000l, 100000l, 100000l, 100000l};
static unsigned long last_real_beat = 0;
static unsigned long last_fake_beat[6] = {0, 0, 0, 0, 0, 0};
static boolean fake_qs[6] = {false, false, false, false, false, false};
static unsigned long cur_animation_started;

void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  if (not sound) Serial.println("setup...");
  for (int sensor=0; sensor<NUM_OF_SENSORS; ++sensor) {  
    /* pinMode(blinkPin[sensor], OUTPUT);         // pin that will blink to your heartbeat! */
    pinMode(fade_pins[sensor], OUTPUT);        // pin that will fade to your heartbeat!
    reset_all(sensor);			       /* reset sensor variables */
  }

  randomSeed(100);		    /* make randomness repeatable */
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE, 
  // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
  //   analogReference(EXTERNAL);   
}


//  Where the Magic Happens
void loop(){
  for (int sensor=0; sensor<NUM_OF_SENSORS; ++sensor) { 
    if (notes_on[sensor]) {
      if (sound) {
	MIDI.sendNoteOff(36 + sensor,0,1);
      }
      notes_on[sensor] = false;
    }
    
    if (QS[sensor]){     // A Heartbeat Was Found
                         // Quantified Self "QS" true when arduino finds a heartbeat
      animate = false;
      pulse_found(sensor);
      QS[sensor] = false;                      // reset the Quantified Self flag for next time
      last_real_beat = millis();
    } else if (animate and fake_qs[sensor]) {
      pulse_found(sensor);
      fake_qs[sensor] = false;
    }
    ledsFadeToBeat(sensor);                      // Makes the LED Fade Effect Happen

    update_fake_beats();

    delay(7);                             //  take a break
  }
}

void pulse_found(int sensor) {
  fade_leds_power[sensor] = 255;         // Makes the LED Fade Effect Happen

  if (sound) {
    MIDI.sendNoteOn(36 + sensor,127,1);  // Send a Note (pitch 42, velo 127 on channel 1)
  }
  notes_on[sensor] = true;

  if (not sound) {
    serialOutputWhenBeatHappens(sensor);   // A Beat Happened, Output that to serial.
  }
}

void ledsFadeToBeat(int sensor){
  // if (fade_leds_power[sensor] > 0) {
  // Serial.println("before - fade_leds_power[sensor]");
  // Serial.println(fade_leds_power[sensor]);
  // }
  fade_leds_power[sensor] -= 40;                         //  set LED fade value
  fade_leds_power[sensor] = constrain(fade_leds_power[sensor],0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fade_pins[sensor],fade_leds_power[sensor]);          //  fade LED
}


void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}

void update_fake_beats() {
  if (not animate_in_idle) {
    return;
  }
  long now = millis();
  
  if (not animate) {
    // if a long time without a person touching the sensor has passed - animate the sensor sometimes
    if (now - last_real_beat >= START_ANIMATION_AFTER_SECONDS * 1000l) {
      /* start animating */
      animate = true;
      randomize_animation(now);
    }
  // animation is in progress
  } else if (now - cur_animation_started > SWITCH_ANIMATION_AFTER_SECONDS * 1000l) {
      /* swithch animation after some time */
      randomize_animation(now);
  } else if (animate) {
  // create fake heartbeats where needed
  for (int sensor = 0; sensor < NUM_OF_SENSORS; sensor++) {
    if (now - last_fake_beat[sensor] >= fake_pulse_interval_ms[sensor]) {
      fake_qs[sensor] = true;
      last_fake_beat[sensor] = now;
    }
  }
}

void randomize_animation(long now) {
  if (not sound)
    Serial.println("randomize_animation");
  
  cur_animation_started = now;
  for (int sensor = 0; sensor < NUM_OF_SENSORS; sensor++) {
    int rand = random(100);
    if (rand < ANIMATION_PROBABILITY) {
	long bpm = random(50, 120);
	fake_pulse_interval_ms[sensor] = (60 * 1000l) / bpm;  /* bpm -> ms: 60 -> 1000, 120 -> 500, 30 -> 2000, 45 -> 1333, 95 -> 631 */
	fake_qs[sensor] = true;
	if (not sound)
	  Serial.println("sensor " + String(sensor) + " will be animated with bpm " + String(bpm));
    } else { // don't animate this sensor in this animation
      if (not sound)
	  Serial.println("sensor " + String(sensor) + " will NOT be animated");
      fake_pulse_interval_ms[sensor] = 10000l;
      fake_qs[sensor] = false;
    }
  } 
}

	
