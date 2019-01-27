/*
   Simpelt program til at sende en frame i DCC format til et tog ved hjælp af interrupt.
   Interface:
   Anbring lokomotivnummeret i den globale variabel lokoadr. f.eks 36.
   Anbring kommandoen i variablen data f.eks. 0X64 forlængs med speed 4, 0X44 baglænge med speed 4.
   Der sendes kontinuerligt frames det er op til dig at sørge for korrekt lokoadr og data.
*/

#include "Ultralyd.h"

#define DCC_PIN  6                     // DCC out
#define soundPin 2
#define hornPin 7
// 58 mikrosekunder med høj efterfulgt af 58 mikrosekunder med lav giver en bit på 1
#define TIMER_SHORT 0x8D               // 58usec pulse length 141 255-141=114
//116 mikrosekunder med høj efterfulgt af 116 mikrosekunder med lav giver en bit på 0
#define TIMER_LONG  0x1B               // 116usec pulse length 27 255-27 =228
#define PREAMBLE  0                    // definitions for state machine
#define SEPERATOR 1                    // definitions for state machine
#define SENDBYTE  2                    // definitions for state machine
#define MAXMSG  2
#define trigPin 9
#define echoPin 10

int analogin = A0;
int sensorValue = 0;
int outputValue = 0;
int speedValue = 0X60;
int lastL = 0;
long timeMillis = 0;


bool second_isr = false;               // pulse up or down
unsigned char last_timer = TIMER_SHORT; // store last timer value
unsigned char flag = 0;                // used for short or long pulse
unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;
unsigned char preample1;               // global variabel for preample part 1
unsigned char preample2;               // global variabel for preample part 2
            // global variabel adresse
unsigned char data = 96;               // global variabel kommando
unsigned char layoutAddress = 0;      // global variabel layoutAdresse
int accAddress = 0;
unsigned char signalSwitchDataByteOne = 0;
unsigned char signalSwitchDataByteTwo = 0;
int regAddress;
unsigned char currentSettingSignalOrSwitch = 0;
unsigned char lastOrder = 0x80;
int output = 3;
int starttal = 3;

long randNumber;
int a[3] = {0x64, 0x69, 0x6F};

// Assign a pointer to a position for an array of signaladdresses with malloc
int* allSignals = (int*)malloc(28 * sizeof(int)); // This can also be written as: (int*)malloc(28 * sizeof allSignals);
// sizeof is NOT a function, so the paranthesis are redundant and sizeof(int) repeats the type we started with, so sizeof allSignals is perhaps better...
int outerTrackSignals[] = {152, 142, 141, 122, 121, 131, 132, 151};
int innerTrackSignals[] = {112, 102, 101, 82, 81, 91, 92, 111};
int outerTwoTracksSwitches[] = {252, 244, 250, 242, 249, 241, 251, 243};
int innerTracksSwitches[] = {244,242,241,243};
int outerTracksSwitches[] = {252,250,249,251};

bool startUpSignalsAndSwitches = false;

int const maxdata = 16;
unsigned char arraydata[2][maxdata];
int msgIndex = 0;
int byteIndex = 0;


struct Message                         // buffer for command
{
  unsigned char data[7];
  unsigned char len;
};

struct Message msg[MAXMSG] =
{
  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3}    // message
};


void SetupTimer2()
{
  TCCR2A = 0; //page 203 - 206 ATmega328/P
  TCCR2B = 2; //Page 206
  TIMSK2 = 1 << TOIE2; //Timer2 Overflow Interrupt Enable - page 211 ATmega328/P
  TCNT2 = TIMER_SHORT; //load the timer for its first cycle
}

ISR(TIMER2_OVF_vect) //Timer2 overflow interrupt vector handler. ISR: Interrupt Service Routine
{
  unsigned char latency;

  if (second_isr)
  { // for every second interupt just toggle signal
    digitalWrite(DCC_PIN, 0);
    second_isr = false;
    latency = TCNT2;  // set timer to last value
    TCNT2 = latency + last_timer;
  }
  else
  { // != every second interrupt, advance bit or state
    digitalWrite(DCC_PIN, 1);
    second_isr = true;
    switch (state)
    {
      case PREAMBLE:
        flag = 1; // short pulse
        preamble_count--;
        if (preamble_count == 0)
        {
          state = SEPERATOR; // advance to next state
          msgIndex++; // get next message
          if (msgIndex >= MAXMSG)
          {
            msgIndex = 0;
          }
          byteIndex = 0; //start msg with byte 0
        }
        break;
      case SEPERATOR:
        flag = 0; // long pulse and then advance to next state
        state = SENDBYTE; // goto next byte ...
        outbyte = msg[msgIndex].data[byteIndex];
        cbit = 0x80;  // send this bit next time first
        break;
      case SENDBYTE:
        if ((outbyte & cbit) != 0)
        {
          flag = 1;  // send short pulse
        }
        else
        {
          flag = 0;  // send long pulse
        }
        cbit = cbit >> 1;
        if (cbit == 0)
        { // last bit sent
          byteIndex++;
          if (byteIndex >= msg[msgIndex].len) // is there a next byte?
          { // this was already the XOR byte then advance to preamble
            state = PREAMBLE;
            preamble_count = 16;
          }
          else
          { // send separtor and advance to next byte
            state = SEPERATOR ;
          }
        }
        break;
    }

    if (flag)
    { // data = 1 short pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_SHORT;
      last_timer = TIMER_SHORT;
    }
    else
    { // data = 0 long pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_LONG;
      last_timer = TIMER_LONG;
    }
  }
}



// Function for setting the speed newSpeed of a specific train with address lokoAddr
void assembleAndSendSpeed(unsigned char newSpeed, unsigned char lokoAddr) {
  data = newSpeed;
  assemble_dcc_msg(lokoAddr);
  delay(50);
}

// Function for controlling the boolean value of a switch or a signal
void assembleAndSendSignalSwitchBytes (int switchOrSignalAddress, int greenRedStraightTurnBoolean) {
  // Set the layoutaddress to the address of the signal or switch to be controlled
  layoutAddress = switchOrSignalAddress;
 
  accAddress = layoutAddress / 4 + 1;
  regAddress = ((layoutAddress % 4) - 1);

  if (regAddress < 0) {
    regAddress = 3;
    accAddress = accAddress - 1;
  }
  
  accAddress = accAddress & 63;

  signalSwitchDataByteOne = accAddress + 128;
  
  delay(30);

  computeSignalSwitchDataByteTwo(1, greenRedStraightTurnBoolean);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg(signalSwitchDataByteOne);

  delay(30);

  computeSignalSwitchDataByteTwo(0, greenRedStraightTurnBoolean);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg(signalSwitchDataByteOne);

}

void computeSignalSwitchDataByteTwo (unsigned char fifthBit, unsigned char eigthBit) {
  signalSwitchDataByteTwo = 240;
  if (fifthBit == 1) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 8;
  } else {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 0;
  }

  if (regAddress == 3) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo ^ 6;
  } else if (regAddress == 2) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo ^ 4;
  } else if (regAddress == 1) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo ^ 2;

  }

  if (eigthBit == 1) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo ^ 1;
  } else {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo ^ 0;
  }
}

void startUpSignalsAndSwitchesFunction() {
  for (int switchAddress: outerTwoTracksSwitches) {
      assembleAndSendSignalSwitchBytes(switchAddress, 0);
  }
  for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
  }
  for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
  }

  for(int i = 0; i < 8; i++) {
      assembleAndSendSignalSwitchBytes(outerTrackSignals[i], 1);
      assembleAndSendSignalSwitchBytes(innerTrackSignals[i], 1);
      delay(500);
      if (i > 0) {
        assembleAndSendSignalSwitchBytes(outerTrackSignals[i - 1], 0);
        assembleAndSendSignalSwitchBytes(innerTrackSignals[i - 1], 0);
      }
  } 
  
  for (int j = 0; j < 28; j++) {
      assembleAndSendSignalSwitchBytes(allSignals[j], 1);
  }
  free(allSignals);
  for (int switchAddress: outerTwoTracksSwitches) {
      assembleAndSendSignalSwitchBytes(switchAddress, 1);
  }

}

void randomSpeed(unsigned char lokoAddr) {
  randNumber = random(0, 3);
  assembleAndSendSpeed(a[randNumber], lokoAddr);
  Serial.println(a[randNumber]);
}

void rideTwoTrainsIntoTheHorizon(int loko1, int loko2){
  if (timeMillis+10000<millis()){
    Serial.println("Random værdier for loko1");
    randomSpeed(loko1);
    Serial.println("Random værdier for loko2");
    randomSpeed(loko2);
    timeMillis = millis();
  }
}



void setup()
{
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

 // Fill our memory allocated array with values for signals. The temporary array will automatically be discarded when setup function ends
  int tempAllSignalsArray[] = {152, 142, 141, 122, 121, 131, 132, 151, 112, 102, 101, 82, 81, 91, 92, 111, 31, 42, 41, 32, 11, 12, 52, 61, 21, 22, 51, 62};
  if (allSignals != NULL) {
    for(int i = 0; i < 28; i++) {
    allSignals[i] = tempAllSignalsArray[i];
    }
  }
  
  
  SetupTimer2();
  startUpSignalsAndSwitchesFunction();
  assembleAndSendSignalSwitchBytes(231, 1);
  assembleAndSendSignalSwitchBytes(232, 0);
  assembleAndSendSignalSwitchBytes(233, 0);
  assembleAndSendSignalSwitchBytes(234, 1);
  delay(5000);
  
  randomSeed(analogRead(0));

}

void loop()
{
  rideTwoTrainsIntoTheHorizon(40, 8);
  int l = distance(trigPin,echoPin);
  Serial.println(l);
  if (l < 5 && lastL < 5) {
        Serial.println("TRAIN ON OUTER TRACK");
        trainPassed(false);
  } else if (l > 9 && l < 16 && lastL > 9 && lastL < 16) {
        Serial.println("TRAIN ON INNER TRACK");
        trainPassed(true);
  } else {
        Serial.println("NO TRAIN RIGHT NOW");
  }
  lastL = l;
  
}

void trainPassed(bool innerTrain){
  for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
  }
  for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
  }
  if (innerTrain){
    for (int signalAddress: innerTracksSwitches) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
    }
    delay(3000);
    for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
    }
  }else{
    for (int switchAddress: outerTracksSwitches) {
      assembleAndSendSignalSwitchBytes(switchAddress, 0);
    }
    for (int signalAddress: innerTracksSwitches) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
    }
    delay(3000);
    for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
    }
  } 
  Serial.println("Delay starter");
  delay(10000);
  Serial.println("Delay slutter");
  bool passed = false;
  lastL = 0;
  while (passed == false){
    rideTwoTrainsIntoTheHorizon(40, 8);
    int l = distance(trigPin,echoPin);
    Serial.println(l);
    if (l < 5 && lastL <5) {
        Serial.println("TRAIN ON OUTER TRACK");
        passed = true;
    } else if (l > 9 && l < 16 && lastL > 9 && lastL < 16) {
          Serial.println("TRAIN ON INNER TRACK");
          passed = true;
    } else {
          Serial.println("NO TRAIN RIGHT NOW");
    }
    lastL = l;
  }
  if (innerTrain){
    for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
    }
    delay(3000);
    for (int signalAddress: innerTracksSwitches) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
    }
  }else{
    for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 0);
    }
    delay(3000);
    for (int switchAddress: outerTwoTracksSwitches) {
      assembleAndSendSignalSwitchBytes(switchAddress, 1);
    }
  }
  for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
  }
  for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
  }
  delay(2000);
}



void assemble_dcc_msg(unsigned char lokoAddr)
{
  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[1].data[0] = lokoAddr;
  msg[1].data[1] = data;
  msg[1].data[2] = lokoAddr ^ data;
  interrupts();
}
