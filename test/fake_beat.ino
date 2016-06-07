#include <TimerOne.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();


const int NUM_OF_SENSORS = 6;
const int pressure_sensor_pin[6] = {10,  11,  12, 13, 14, 15};                 // Pulse digital switch
const int PRESSURE_SENSOR_ZERO[6] = {200,  0,  0, 0, 0, 0};                 // Pulse digital switch
const int fade_pins[6] =   {2, 3, 4, 5, 6, 7};                  // pin to do fancy classy fading blink at each beat
volatile boolean notes_on[6] = {false, false, false, false, false, false};        // flag for sounds per sensor
volatile int fade_leds_power[6] = {0, 0, 0, 0, 0, 0};                 // used to fade LED on with PWM on fadePin
volatile unsigned long sample_time = 0;          // used to determine pulse timing


static const boolean verbose = false;
static const boolean sound = true;

// Idle animation configs ("fake pulse")
static const int START_ANIMATION_AFTER_SECONDS = 3;
static const int SWITCH_ANIMATION_AFTER_SECONDS = 20;
static const float ANIMATION_PROBABILITY = 60;
static boolean animating_beat[6] = {false, false, false, false, false, false;}
static unsigned long fake_pulse_interval_ms[6] = {100000l, 100000l, 100000l, 100000l, 100000l, 100000l};
static unsigned long last_real_beat = 0;
static unsigned long last_fake_beat[6] = {0, 0, 0, 0, 0, 0};
static boolean fake_qs[6] = {false, false, false, false, false, false};
static unsigned long cur_animation_started;


void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  if (not sound) Serial.println("setup...");
  for (int sensor=0; sensor<NUM_OF_SENSORS; ++sensor) {  
    pinMode(fade_pins[sensor], OUTPUT);        // pin that will fade to your heartbeat!
  }

  randomSeed(100);		    /* make randomness repeatable */
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
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
    
    if (animating_beat[sensor] and fake_qs[sensor]) {
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
}

void ledsFadeToBeat(int sensor){
  fade_leds_power[sensor] -= FADE_SPEED;                         //  set LED fade value
  fade_leds_power[sensor] = constrain(fade_leds_power[sensor],0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fade_pins[sensor],fade_leds_power[sensor]);          //  fade LED
}


void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}

void update_fake_beats() {
  long now = millis();
  
  if (sensor_enabled[sensor]) {
    if (not animating_beat[sensor]) {
      // if this is a "new" press on the sensor, then randomize a beat frequency
      if (now - last_real_beat >= START_ANIMATION_AFTER_SECONDS * 1000l) {
        /* start animating */
        animating_beat[sensor] = true;
        randomize_animation(now);
      }
    // animation is in progress
    } else {
      // create fake heartbeats where needed
      for (int sensor = 0; sensor < NUM_OF_SENSORS; sensor++) {
        if (now - last_fake_beat[sensor] >= fake_pulse_interval_ms[sensor]) {
          fake_qs[sensor] = true;
          last_fake_beat[sensor] = now;
        }
      }
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

	

void interruptSetup(){    
  Timer1.initialize(2000);                      // a timer of 2000 microseconds
  Timer1.attachInterrupt(get_sensor_readings);  // get readings from sensors every 2 ms   
} 


void get_sensor_readings(void) {
  cli();                                      // disable interrupts while we do this
  sample_time = millis();                     // keep track of the time in ms with this variable
  for (int sensor=0; sensor < NUM_OF_SENSORS; ++sensor) {
      sensor_enabled[[sensor] = (analogRead(pressure_sensor_pin[sensor]) > PRESSURE_SENSOR_ZERO[sensor]+80);
  }
  sei();                                   // enable interrupts when youre done!
}

