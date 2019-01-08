/*
   Simpelt program til at sende en frame i DCC format til et tog ved hjælp af interrupt.
   Interface:
   Anbring lokomotivnummeret i den globale variabel lokoadr. f.eks 36.
   Anbring kommandoen i variablen data f.eks. 0X64 forlængs med speed 4, 0X44 baglænge med speed 4.
   Der sendes kontinuerligt frames det er op til dig at sørge for korrekt lokoadr og data.

*/
#include "Ultralyd.h"
#include "SetupTrainTrack.h"
#include "TrainSignalSwitchFunctions.h"


#define DCC_PIN  6                     // DCC out
#define trigPin 9
#define echoPin 10
#define MAXMSG  2
#define TIMER_SHORT 0x8D               // 58usec pulse length 141 255-141=114
#define TIMER_LONG  0x1B               // 116usec pulse length 27 255-27 =228

#define PREAMBLE  0                    // definitions for state machine
#define SEPERATOR 1                    // definitions for state machine
#define SENDBYTE  2                    // definitions for state machine

int analogin = A0;
int sensorValue = 0;
int outputValue = 0;
int speedValue = 0X60;
unsigned char currentSettingSignalOrSwitch = 0;
unsigned char lastOrder = 0x80;
unsigned char last_timer = TIMER_SHORT; // store last timer value

unsigned char flag = 0;                // used for short or long pulse
unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;
unsigned char preample1;               // global variabel for preample part 1
unsigned char preample2;               // global variabel for preample part 2

int output = 3;
int starttal = 3;
bool startUpSignalsAndSwitches = false;




struct Message msg[MAXMSG] =
{
  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3}    // message
};


struct Message                         // buffer for command
{
  unsigned char data[7];
  unsigned char len;
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






void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // enable styrepin som output på pin 6
  SetupTimer2();
  
  randomSeed(analogRead(0));

}

void loop()
{
  //rideTwoTrainsIntoTheHorizon(40, 8);
  assembleAndSendSpeed(0x61, 40);
  assembleAndSendSpeed(0x61, 8);
  //randomSpeed(40);
  //randomSpeed(8);
  int l = distance(trigPin,echoPin);
  Serial.println(l);
  if (l < 5) {
        Serial.println("TRAIN ON OUTER TRACK");
        delay(500);
  } else if (l > 9 && l < 14) {
        Serial.println("TRAIN ON INNER TRACK");
        delay(500);
  } else {
        Serial.println("NO TRAIN RIGHT NOW");
  }
  /*if (startUpSignalsAndSwitches == false) {
    startUpSignalsAndSwitchesFunction();
    startUpSignalsAndSwitches = true;
  }*/

  
  
}
