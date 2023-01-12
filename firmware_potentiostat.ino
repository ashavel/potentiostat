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
 * MERCHANTABILITY or FITNES FOR A PARTICULAR PURPOSE. See the GNU
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
short ReadingsForMean = 10; //Number of readings taken for averaging. 10 is recommended.
unsigned long PreviousMillis;
unsigned long CurrentMillis;
short Msdelay;
unsigned long Linesamples;
int Potref = 2047; // CE potential in mV vs GND.
int ScanSpeed;
int ScanPotential;
int ScanPotentialDigital;
int EndPotential;
int EndPotentialDigital;
byte MethodType;
boolean F1 = false;
boolean F2 = false;
short J = 0;
short VoidSamples = 0; // VoidSamples is the set number of idle samples before the data begin to be sent to the serial port
short VoidSamplesActual = 0; // VoidSamplesActual is the actual number of idle samples before the data begin to be sent to the serial port
float WorkElectrodePotential; //Output work electrode potential in mV
float CounterElectrodePotential; //Output counter electrode potential in mV
float ReferenceElectrodePotential; //Output reference electrode potential in mV
float CurrentSignal; //Output voltage drop on the shunt in mV. To get current in mA, should be divided to the shunt R value in ohm

//~ variables for incoming serial data
boolean IsTransmitting = false;
const byte NumChars = 32;
char ReceivedChars[NumChars]; // an array to store the received data
// char EndMarker = '\n';
char RecvChar;
char MessageFromPc[32] = {0};

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
  dac.setVoltage(Potref, false); //At the startup, sets the potential of the WE equal to the CE
}

void RecieveData() {
  static byte Ndx = 0;
  char ReadCaracter;
  if (Serial.available() > 0 && F1 == false) {
    //Serial.println("Recieval started");
    ReadCaracter = Serial.read();
    ReceivedChars[Ndx] = ReadCaracter;
    Ndx++;

    // '\x02' byte is ASCII code for the "start of text"
    // if (ReadCaracter == 2 && ReceivedChars[Ndx - 1] == '\x02') {
    // '\x2A' byte is ASCII code for the "*"
    if (ReadCaracter == 42 && ReceivedChars[Ndx - 1] == '\x2A') {
      // clear ReceivedChars and enable buffer filling
      memset(ReceivedChars, 0, sizeof(ReceivedChars));
      IsTransmitting = true;
      Ndx = 0; // Do not hold the '\x2A' byte
    }

    //~ if (ReadCaracter == '\n' && IsTransmitting == true) {
    // '\x62' byte is ASCII code for the ">"
    if (ReadCaracter == 62 && IsTransmitting == true) {
      IsTransmitting = false; // Transmission is off
      ReceivedChars[Ndx] = '\0'; // Terminate the string
      Ndx = 0; // Counter to zero
      Serial.println("Receival terminated");
      Serial.println(ReceivedChars);
      F1 = true;
      F2 = false;
    }

    // Protection against overfloating
    if (Ndx >= NumChars) {
      Ndx = NumChars - 1;
    }
  }
}


void ParseAndSet() {

  // Splits the data into parts

  if (F1 == true) {
    char * strtokINdx; // This is used by strtok() as an index

    strtokINdx = strtok(ReceivedChars, ","); // Get the first part - the string
    strcpy(MessageFromPc, strtokINdx);     // Copy it to MessageFromPc
    MethodType = atoi(MessageFromPc);             // Convert this part to the integer method code

    strtokINdx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(MessageFromPc, strtokINdx); // Copy it to MessageFromPc
    ScanSpeed = atoi(MessageFromPc);         // Convert this part to the integer scanning speed OR scanning time

    strtokINdx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(MessageFromPc, strtokINdx); // Copy it to MessageFromPc
    VoidSamples = atoi(MessageFromPc);         // Convert this part to an integer number of void samples taken before the measurement starts


    strtokINdx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(MessageFromPc, strtokINdx); // Copy it to MessageFromPc
    ScanPotential = atoi(MessageFromPc);         // Convert this part to the integer starting potential
    ScanPotentialDigital = ScanPotential + Potref; // Calculate starting potential in DAC digital units

    strtokINdx = strtok(NULL, ",");     // This continues where the previous call left off
    strcpy(MessageFromPc, strtokINdx); // Copy it to MessageFromPc
    EndPotential = atoi(strtokINdx);            // Convert this part to the integer end potential
    EndPotentialDigital = EndPotential + Potref; // Calculate the end potential in DAC digital units


    // Serial.println("Method code");
    // Serial.print(MethodType);
    // Serial.println();
    // Serial.println("LSV/CV scanning speed mv/s OR Line scanning time s");
    // Serial.print(ScanSpeed);
    // Serial.println();
    // Serial.println("Number of idle measurement steps before data delivery begins");
    // Serial.print(VoidSamples);
    // Serial.println();
    // Serial.println("Starting potential mv OR Line scanning potential");
    // Serial.print(ScanPotential);
    // Serial.println();
    // Serial.println("End potential mv");
    // Serial.print(EndPotential);
    // Serial.println();
    // Serial.println();


    F1 = false;
    F2 = true;

    //Check if the parameters are in reasonable ranges
    if (MethodType > 2 || MethodType < 1) {Serial.print("Wrong method code"); F2 = false;}
    if (VoidSamples > 1000 || VoidSamples < 0) {Serial.print("Wrong idle samples number"); F2 = false;}
    // if (ScanPotential > 2000 || ScanPotential < -2000) {Serial.print("Wrong low potential"); F2 = false;}
    // if (EndPotential > 2000 || EndPotential < -2000) {Serial.print("Wrong high potential"); F2 = false;}
    // if (MethodType == 1) {if (ScanSpeed > 150 || ScanSpeed < 1) {Serial.print("Wrong scanning speed"); F2 = false;}}
    // if (MethodType == 2) {if (ScanSpeed > 36000 || ScanSpeed < 1) {Serial.print("Wrong line scanning time"); F2 = false;}}

    VoidSamplesActual = 0;

    // External stop...
    if (MethodType == 0) {
      disable();
    }

    if (MethodType == 1 && F2 == true) {
      Msdelay = (1000 / ScanSpeed);
      Serial.println("START");
      Serial.println();
    }

    if (MethodType == 2) {
      Msdelay = 100;
      Linesamples = ScanSpeed * (1000 / Msdelay);
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
  MethodType = 0;
  VoidSamplesActual = 0;
  VoidSamples = 0;
}

void LSV () {

  if (MethodType == 1) {
    VoidSamplesActual++;
    measure();
    dac.setVoltage(ScanPotentialDigital, false);
    if (VoidSamplesActual > VoidSamples && ScanPotentialDigital < EndPotentialDigital) {(ScanPotentialDigital++);};
    if (VoidSamplesActual > VoidSamples && ScanPotentialDigital > EndPotentialDigital) {(ScanPotentialDigital--);};
    if (VoidSamplesActual > VoidSamples && ScanPotentialDigital == EndPotentialDigital) {measure(); disable();};
  }
}

void Line() {
  if (MethodType == 2 && Linesamples > 0) {
    VoidSamplesActual++;
    measure();
    if (VoidSamplesActual > VoidSamples) {
      (Linesamples--);
    };
  }

  if (MethodType == 2 && Linesamples == 0) {
    measure();
    disable();
  }
}


void measure () {
  dac.setVoltage(ScanPotentialDigital, false);
  WorkElectrodePotential = 0;
  CurrentSignal = 0;
 CounterElectrodePotential = 0;
  ReferenceElectrodePotential = 0;
  float Cnr_delta;
  if (VoidSamplesActual > VoidSamples) {
    for (J = 0; J < ReadingsForMean; J++) {
      WorkElectrodePotential = WorkElectrodePotential + analogRead(workelectrodepin);
      CounterElectrodePotential = CounterElectrodePotential + analogRead(counterelectrodepin);
      CurrentSignal = CurrentSignal + analogRead(currentmeasurepin);
      ReferenceElectrodePotential = ReferenceElectrodePotential + analogRead(referenceelectrodepin);
    }
    WorkElectrodePotential = (WorkElectrodePotential - ReferenceElectrodePotential) * 4 / ReadingsForMean;
    CurrentSignal = (CounterElectrodePotential - CurrentSignal) * 4 / ReadingsForMean;
    Cnr_delta = Potref - CounterElectrodePotential * 4 / ReadingsForMean;

    if (Cnr_delta > 500) {
      disable();
      Serial.println("EMERGENCY!!");
      Serial.print("Delta Ref is ");
      Serial.print(Cnr_delta);
      Serial.println(" mV");

    }
    Serial.print(CurrentMillis);
    Serial.print(",");
    Serial.print(-1);
    Serial.print(",");
    Serial.print(Cnr_delta);
    Serial.print(",");
    Serial.print(WorkElectrodePotential);
    Serial.print(",");
    Serial.print(CurrentSignal);
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
  CurrentMillis = millis();
  if (CurrentMillis - PreviousMillis >= Msdelay) {
    PreviousMillis = CurrentMillis;
    // Serial.println("Millis passed");
    LSV();
    Line();
  }
}
