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

// TODO - set all pin numbers!

//  pin numbers
const int pulsePin[6] =    {5,  4,  3, 2, 1, 0};                 // Pulse Sensor purple wire connected to analog pin 0
const int pressure_sensor_pin[6] = {10,  11,  12, 13, 14, 15};                 // Pulse digital switch
/* const int blinkPin[6] =    {13, 13, 2, 3, 4, 5};                // pin to blink led at each beat */
const int fade_pins[6] =   {7, 6, 5, 4, 3, 2};                  // pin to do fancy classy fading blink at each beat

const bool use_pressure_sensors = false;	/* should sensors be ignored if their pressure_sensor_pin is 0 */

volatile boolean QS[6] = {false, false, false, false, false, false};        // becomes true when Arduoino finds a beat to signal main loop to handle leds.
volatile boolean notes_on[6] = {false, false, false, false, false, false};        // flag for sounds per sensor
volatile int fade_leds_power[6] = {0, 0, 0, 0, 0, 0};                 // used to fade LED on with PWM on fadePin
volatile unsigned long sample_time = 0;          // used to determine pulse timing

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 
static boolean verbose = false;
static boolean sound = true;


void setup(){
  for (int sensor=0; sensor<NUM_OF_SENSORS; ++sensor) {  
    /* pinMode(blinkPin[sensor], OUTPUT);         // pin that will blink to your heartbeat! */
    pinMode(fade_pins[sensor], OUTPUT);        // pin that will fade to your heartbeat!
    reset_all(sensor);			       /* reset sensor variables */
  }
  Serial.begin(115200);             // we agree to talk fast!
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
                         // BPM and IBI have been Determined
                         // Quantified Self "QS" true when arduino finds a heartbeat
          fade_leds_power[sensor] = 255;         // Makes the LED Fade Effect Happen

	  if (sound) {
            MIDI.sendNoteOn(36 + sensor,127,1);  // Send a Note (pitch 42, velo 127 on channel 1)
	  }
	  notes_on[sensor] = true;

          // Serial.println("setting to 255");
                                  // Set 'fadeRate' Variable to 255 to fade LED with pulse

          if (not sound) {
              serialOutputWhenBeatHappens(sensor);   // A Beat Happened, Output that to serial.     
          }
          QS[sensor] = false;                      // reset the Quantified Self flag for next time    
    }
    ledsFadeToBeat(sensor);                      // Makes the LED Fade Effect Happen 
  }   
  delay(7);                             //  take a break
}


void ledsFadeToBeat(int sensor){
  // if (fade_leds_power[sensor] > 0) {
      // Serial.println("before - fade_leds_power[sensor]");
      // Serial.println(fade_leds_power[sensor]);
  // }
  fade_leds_power[sensor] -= 5;                         //  set LED fade value
  fade_leds_power[sensor] = constrain(fade_leds_power[sensor],0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fade_pins[sensor],fade_leds_power[sensor]);          //  fade LED
}


void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}

