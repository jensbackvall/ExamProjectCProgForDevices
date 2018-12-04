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
#define TIMER_SHORT 0x8D               // 58usec pulse length 141 255-141=114
#define TIMER_LONG  0x1B               // 116usec pulse length 27 255-27 =228
#define PREAMBLE  0                    // definitions for state machine
#define SEPERATOR 1                    // definitions for state machine
#define SENDBYTE  2                    // definitions for state machine
#define MAXMSG  2
#define trigPin 12
#define echoPin 13

int analogin = A0;
int sensorValue = 0;
int outputValue = 0;
int speedValue = 0X60;


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
unsigned char accAddress = 0;
unsigned char signalSwitchDataByteOne = 0;
unsigned char signalSwitchDataByteTwo = 0;
int regAddress;
unsigned char currentSettingSignalOrSwitch = 0;
unsigned char lastOrder = 0x80;
int output = 3;
int starttal = 3;

long randNumber;
int a[3];

int outerTrackSignals[] = {152, 142, 141, 122, 121, 131, 132, 151};
int innerTrackSignals[] = {112, 102, 101, 82, 81, 91, 92, 111};
int outerTwoTracksSwitches[] = {252, 244, 250, 242, 249, 241, 251, 243};


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

ISR(TIMER2_OVF_vect) //Timer2 overflow interrupt vector handler
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
  delay(750);
}

/* void assembleAndSendOrder(unsigned char trainFunction) {

  unsigned char newOrder;

  if (trainFunction == 7) {
    newOrder = lastOrder ^ 16; // changes the fifth bit
  } else if (trainFunction == 8) {
    newOrder = lastOrder ^ 4; // changes the third bit
  } else if (trainFunction == 9) {
    newOrder = lastOrder ^ 1; // changes the first bit
  } else {
    return;
  }

  data = newOrder;
  
  assemble_dcc_msg(lokoadr);

  lastOrder = data;

  delay(750);
} */





// Function for controlling the boolean value of a switch or a signal
void assembleAndSendSignalSwitchBytes (unsigned char switchOrSignalAddress, int greenRedStraightTurnBoolean) {

  // Set the layoutaddress to the address of the signal or switch to be controlled
  layoutAddress = switchOrSignalAddress;
  Serial.println("LAYOUTADDRESS");
  Serial.println(layoutAddress);

  accAddress = ((layoutAddress / 4) + 1) & 63;

  regAddress = ((layoutAddress % 4) - 1);
  Serial.println("REGADDRESS:");
  Serial.println(regAddress);

  if (regAddress < 0) {
    regAddress = 3;
    Serial.println("regAddress lige sat til 3");
    Serial.println(regAddress);
    accAddress = accAddress - 1;
  }

  signalSwitchDataByteOne = accAddress + 128;
  Serial.println("regAddress should still be 3");
  Serial.println(regAddress);
  //assemble_dcc_msg(signalSwitchDataByteOne);

  delay(30);

  computeSignalSwitchDataByteTwo(1, greenRedStraightTurnBoolean);

  Serial.println("regAddress should STILL be 3");
  Serial.println(regAddress);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg(signalSwitchDataByteOne);
  Serial.println("First assembly");
  Serial.println(accAddress);
  Serial.println(regAddress);

  delay(30);

  computeSignalSwitchDataByteTwo(0, greenRedStraightTurnBoolean);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg(signalSwitchDataByteOne);
  Serial.println("Second assembly");
  Serial.println(accAddress);
  Serial.println(regAddress);


}

//void computeSignalSwitchDataByteOne (unsigned char accAddress) {
//  signalSwitchDataByteOne = accAddress + 128;
//}

// TODO: ændring af regaddr virker ikke helt, så når vi eksempelvis vil skifte signal nr 152, fungerer det ikke.
// Dette skal ordnes!
void computeSignalSwitchDataByteTwo (unsigned char fifthBit, unsigned char eigthBit) {
  signalSwitchDataByteTwo = 240;
  if (fifthBit == 1) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 8;
  } else {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 0;
  }

  //regAddress = (layoutAddress % 4) - 1;

  //if(accAddress % 4 != 0) {
  //  regAddress = (layoutAddress % 4) - 1;
  //} else {
  //  regAddress = 3;
  //}


  //if (regAddress < 0) {
    //regAddress = 3;
    //accAddress = accAddress - 1;
  //}

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
  
  for (int signalAddress: outerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
  }
  for (int signalAddress: innerTrackSignals) {
      assembleAndSendSignalSwitchBytes(signalAddress, 1);
  }
    for (int switchAddress: outerTwoTracksSwitches) {
      assembleAndSendSignalSwitchBytes(switchAddress, 1);
  }

}

void rideTwoTrainsIntoTheHorizon(int loko1, int loko2){
  Serial.println("Random værdier for loko1");
  randomSpeed(loko1);
  Serial.println("Random værdier for loko2");
  randomSpeed(loko2);
}



void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT); // enable styrepin som output på pin 6
  assemble_dcc_msg(36);
  SetupTimer2();

}

void loop()
{
  //assembleAndSendSpeed(0X6F,40);
  //assembleAndSendSignalSwitchBytes (151, 0);
  int l = distance(trigPin,echoPin);
  if (startUpSignalsAndSwitches == false) {
    startUpSignalsAndSwitchesFunction();
    startUpSignalsAndSwitches = true;
  }

  rideTwoTrainsIntoTheHorizon(40, 8);
  
}



void assemble_dcc_msg(unsigned char lokoAddr)
{
  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[1].data[0] = lokoAddr;
  msg[1].data[1] = data;
  msg[1].data[2] = lokoAddr ^ data;
  //Serial.print(lokoadr);
  //Serial.print("  ");
  //Serial.print(data);
  //Serial.println();

  interrupts();
}
