
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

// TODO - set all pin numbers!

//  pin numbers
int pulsePin[6] = {0, 1, 2, 3, 4, 5};                 // Pulse Sensor purple wire connected to analog pin 0
int pulseSwitch[6] = {4, 4, 2, 3, 4, 5};                 // Pulse digital switch
int blinkPin[6] = {13, 13, 2, 3, 4, 5};                // pin to blink led at each beat
int fade_pins[6] = {6, 6, 2, 3, 4, 5};                  // pin to do fancy classy fading blink at each beat


volatile boolean QS[6] = {false, false, false, false, false, false};        // becomes true when Arduoino finds a beat to signal main loop to handle leds.
volatile unsigned long BPM[6];                   // int that holds raw Analog in 0. updated every 2mS
volatile unsigned long IBI[6] = {600, 0, 0, 0, 0, 0};             // int that holds the time interval between beats! Must be seeded! 
volatile int fade_leds_power[6] = {0, 0, 0, 0, 0, 0};                 // used to fade LED on with PWM on fadePin
volatile int NUM_OF_SENSORS = 2;

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = true;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 
static boolean verbose_log = true;


void setup(){
  for (int sensor=0; sensor<NUM_OF_SENSORS; ++sensor) {  
    pinMode(blinkPin[sensor],OUTPUT);         // pin that will blink to your heartbeat!
    pinMode(fade_pins[sensor], OUTPUT);          // pin that will fade to your heartbeat!
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
    if (QS[sensor]){     // A Heartbeat Was Found
                         // BPM and IBI have been Determined
                         // Quantified Self "QS" true when arduino finds a heartbeat
          fade_leds_power[sensor] = 15;         // Makes the LED Fade Effect Happen
          Serial.println("setting to 50");
                                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
          serialOutputWhenBeatHappens(sensor);   // A Beat Happened, Output that to serial.     
          QS[sensor] = false;                      // reset the Quantified Self flag for next time    
    }
    ledsFadeToBeat(sensor);                      // Makes the LED Fade Effect Happen 
  }   
  delay(7);                             //  take a break
}





void ledsFadeToBeat(int sensor){
  if (fade_leds_power[sensor] > 0) {
      Serial.println("before - fade_leds_power[sensor]");
      Serial.println(fade_leds_power[sensor]);
  }
  fade_leds_power[sensor] -= 5;                         //  set LED fade value
  fade_leds_power[sensor] = constrain(fade_leds_power[sensor],0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fade_pins[sensor],fade_leds_power[sensor]);          //  fade LED
}



