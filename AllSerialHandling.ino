
//////////
/////////  All Serial Handling Code, 
/////////  It's Changeable with the 'serialVisual' variable
/////////  Set it to 'true' or 'false' when it's declared at start of code.  
/////////

void serialOutput(int Signal){   // Decide How To Output Serial. 
 if (serialVisual){  
     arduinoSerialMonitorVisual('-', Signal);   // goes to function that makes Serial Monitor Visualizer
 } else{
      sendDataToSerial('S', Signal);     // goes to sendDataToSerial function
 }        
}


//  Decides How To OutPut BPM and IBI Data
void serialOutputWhenBeatHappens(int sensor){
 if (not sounds && serialVisual){            //  Code to Make the Serial Monitor Visualizer Work
    Serial.print("*** Heart-Beat Happened *** A");  //ASCII Art Madness
    Serial.println(pulsePin[sensor]);
    Serial.print(" ");
    Serial.println(sample_time);
    // Serial.print("BPM: ");
    // Serial.print(" - ");
    // Serial.println(BPM[i]);
 } else {
   if (not sound) {
     Serial.println("A0 A1 A2 A3 A4 A5");
     for (byte i=0; i < pulsePin[sensor]; i++) {
       Serial.print("   ");
     }
     Serial.println(" x ");
   }
 }   
}



//  Sends Data to Pulse Sensor Processing App, Native Mac App, or Third-party Serial Readers. 
void sendDataToSerial(char symbol, int data ){
  if (sound)
    return;
  
  Serial.print(symbol);
  Serial.println(data);                
}


//  Code to Make the Serial Monitor Visualizer Work
void arduinoSerialMonitorVisual(char symbol, int sensorReading ){
  if (sound)
    return;
  
  const int sensorMin = 0;      // sensor minimum, discovered through experiment
  const int sensorMax = 1024;    // sensor maximum, discovered through experiment

  // map the sensor range to a range of 12 options:
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);

  // do something different depending on the 
  // range value:
  switch (range) {
  case 0:     
    Serial.println("");     /////ASCII Art Madness
    break;
  case 1:   
    Serial.println("---");
    break;
  case 2:    
    Serial.println("------");
    break;
  case 3:    
    Serial.println("---------");
    break;
  case 4:   
    Serial.println("------------");
    break;
  case 5:   
    Serial.println("--------------|-");
    break;
  case 6:   
    Serial.println("--------------|---");
    break;
  case 7:   
    Serial.println("--------------|-------");
    break;
  case 8:  
    Serial.println("--------------|----------");
    break;
  case 9:    
    Serial.println("--------------|----------------");
    break;
  case 10:   
    Serial.println("--------------|-------------------");
    break;
  case 11:   
    Serial.println("--------------|-----------------------");
    break;
  
  } 
}


