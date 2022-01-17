/*
 * This file is part of the Potentiostat distribution
 * https://github.com/ashavel/potentiostat
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
Command string examples...
*1,10,2,1000,-1000> - scan from 1000 mV to -1000 mV at 10 mV/s, omit 2 first measurement points
*2,100,3,1000> - keep the potential of 1000 mV for 100 s, omit 3 first measurement points
*/

#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;

//~ Pins chosen for measurements. Can be changed.
short currentmeasurepin = A0;
short counterelectrodepin = A1;
short referenceelectrodepin = A2;
short workelectrodepin = A3;

//~ measurement variables
short CNTC = 10; //Number of readings taken for averaging. 10 is recommended.
unsigned long previousMillis;
unsigned long currentMillis;
short msdelay;
unsigned long linesamp;
int potref = 2047; // CE potential in mV vs GND.
int Ss;
int Sp;
int SpD;
int Ep;
int EpD;
byte Mt;
boolean F1 = false;
boolean F2 = false;
short j = 0;
short Vs = 0; // Vs is the set number of idle samples before the data begin to be sent to the serial port
short Va = 0; // Va is the actual number of idle samples before the data begin to be sent to the serial port
float v; //Output work electrode potential in mV
float p; //Output counter electrode potential in mV
float r; //Output reference electrode potential in mV
float c; //Output voltage drop on the shunt in mV. To get current in mA, should be divided to the shunt R value in ohm

//~ variables for incoming serial data
boolean is_transmitting = false;
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
// char endMarker = '\n';
char recvChar;
char messageFromPC[32] = {0};

//~ I2C
#define dac_ADDR 0x60 //For DAC devices with A0 pulled HIGH, use 0x61
//~ I2C

void setup() {
  pinMode (workelectrodepin, INPUT);
  pinMode (counterelectrodepin, INPUT);
  pinMode (currentmeasurepin, INPUT);
  analogReference(EXTERNAL);
  Serial.begin(115200);
  dac.begin(0x60);
  dac.setVoltage(potref, false); //At the startup, sets the potential of the WE equal to the CE
}

void RecieveData() {
  static byte ndx = 0;
  char rc;
  if (Serial.available() > 0 && F1 == false) {
    //Serial.println("Recieval started");
    rc = Serial.read();
    receivedChars[ndx] = rc;
    ndx++;

    // '\x02' byte is ASCII code for the "start of text"
    // if (rc == 2 && receivedChars[ndx - 1] == '\x02') {
    // '\x2A' byte is ASCII code for the "*"
    if (rc == 42 && receivedChars[ndx - 1] == '\x2A') {
      // clear receivedChars and enable buffer filling
      memset(receivedChars, 0, sizeof(receivedChars));
      is_transmitting = true;
      ndx = 0; // Do not hold the '\x2A' byte
    }

    //~ if (rc == '\n' && is_transmitting == true) {
    // '\x62' byte is ASCII code for the ">"
    if (rc == 62 && is_transmitting == true) {
      is_transmitting = false; // Transmission is off
      receivedChars[ndx] = '\0'; // Terminate the string
      ndx = 0; // Counter to zero
      Serial.println("Receival terminated");
      Serial.println(receivedChars);
      F1 = true;
      F2 = false;
    }

    // Protection against overfloating
    if (ndx >= numChars) {
      ndx = numChars - 1;
    }
  }
}


void ParseAndSet() {

  // Split the data into parts

  if (F1 == true) {
    char * strtokIndx; // This is used by strtok() as an index

    strtokIndx = strtok(receivedChars, ","); // Get the first part - the string
    strcpy(messageFromPC, strtokIndx);     // Copy it to messageFromPC
    Mt = atoi(messageFromPC);             // Convert this part to the integer method code

    strtokIndx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(messageFromPC, strtokIndx); // Copy it to messageFromPC
    Ss = atoi(messageFromPC);         // Convert this part to the integer scanning speed OR scanning time

    strtokIndx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(messageFromPC, strtokIndx); // Copy it to messageFromPC
    Vs = atoi(messageFromPC);         // Convert this part to an integer number of void samples taken before the measurement starts


    strtokIndx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(messageFromPC, strtokIndx); // Copy it to messageFromPC
    Sp = atoi(messageFromPC);         // Convert this part to the integer starting potential
    SpD = Sp + potref; // Calculate starting potential in DAC digital units

    strtokIndx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(messageFromPC, strtokIndx); // Copy it to messageFromPC
    Ep = atoi(strtokIndx);            // Convert this part to the integer end potential
    EpD = Ep + potref; // Calculate the end potential in DAC digital units


    // Serial.println("Method code");
    // Serial.print(Mt);
    // Serial.println();
    // Serial.println("LSV/CV scanning speed mv/s OR Line scanning time s");
    // Serial.print(Ss);
    // Serial.println();
    // Serial.println("Number of idle measurement steps before data delivery begins");
    // Serial.print(Vs);
    // Serial.println();
    // Serial.println("Starting potential mv OR Line scanning potential");
    // Serial.print(Sp);
    // Serial.println();
    // Serial.println("End potential mv");
    // Serial.print(Ep);
    // Serial.println();
    // Serial.println();


    F1 = false;
    F2 = true;

    //Check if the parameters are in reasonable ranges
    if (Mt > 2 || Mt < 1) {Serial.print("Wrong method code"); F2 = false;}
    if (Vs > 1000 || Vs < 0) {Serial.print("Wrong idle samples number"); F2 = false;}
    // if (Sp > 2000 || Sp < -2000) {Serial.print("Wrong low potential"); F2 = false;}
    // if (Ep > 2000 || Ep < -2000) {Serial.print("Wrong high potential"); F2 = false;}
    // if (Mt == 1) {if (Ss > 150 || Ss < 1) {Serial.print("Wrong scanning speed"); F2 = false;}}
    // if (Mt == 2) {if (Ss > 36000 || Ss < 1) {Serial.print("Wrong line scanning time"); F2 = false;}}

    Va = 0;

    // External stop...
    if (Mt == 0) {
      disable();
    }

    if (Mt == 1 && F2 == true) {
      msdelay = (1000 / Ss);
      Serial.println("START");
      Serial.println();
    }

    if (Mt == 2) {
      msdelay = 100;
      linesamp = Ss * (1000 / msdelay);
      Serial.println("START");
      Serial.println();
    }

  }
}

void disable() {
  Serial.print("STOP");
  // Serial.print(",");
  // Serial.print(";");
  Serial.println();
  F1 = false;
  F2 = false;
  Mt = 0;
  Va = 0;
  Vs = 0;
}

void LSV () {

  if (Mt == 1) {
    Va++;
    measure();
    dac.setVoltage(SpD, false);
    if (Va > Vs && SpD < EpD) {(SpD++);};
    if (Va > Vs && SpD > EpD) {(SpD--);};
    if (Va > Vs && SpD == EpD) {measure(); disable();};
  }
}

void Line() {
  if (Mt == 2 && linesamp > 0) {
    Va++;
    measure();
    if (Va > Vs) {
      (linesamp--);
    };
  }

  if (Mt == 2 && linesamp == 0) {
    measure();
    disable();
  }
}


void measure () {
  dac.setVoltage(SpD, false);
  v = 0;
  c = 0;
  p = 0;
  r = 0;
  float cnr_delta;
  if (Va > Vs) {
    for (j = 0; j < CNTC; j++) {
      v = v + analogRead(workelectrodepin);
      p = p + analogRead(counterelectrodepin);
      c = c + analogRead(currentmeasurepin);
      r = r + analogRead(referenceelectrodepin);
    }
    v = (v - r) * 4 / CNTC;
    c = (p - c) * 4 / CNTC;
    cnr_delta = potref - p * 4 / CNTC;

    if (cnr_delta > 500) {
      disable();
      Serial.println("EMERGENCY!!");
      Serial.print("Delta Ref is ");
      Serial.print(cnr_delta);
      Serial.println(" mV");

    }
    Serial.print(currentMillis);
    Serial.print(",");
    Serial.print(-1);
    Serial.print(",");
    Serial.print(cnr_delta);
    Serial.print(",");
    Serial.print(v);
    Serial.print(",");
    Serial.print(c);
    Serial.print(",");
    //Serial.print(0,4);
    //Serial.print(",");
    //Serial.print(0,4);
    //Serial.print(",");
    Serial.print(";");
    Serial.println();
  }
}

void loop() {
  RecieveData();
  ParseAndSet();
  currentMillis = millis();
  if (currentMillis - previousMillis >= msdelay) {
    previousMillis = currentMillis;
    // Serial.println("Millis passed");
    LSV();
    Line();
  }
}
