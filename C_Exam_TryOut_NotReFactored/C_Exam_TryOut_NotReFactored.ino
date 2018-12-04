/*
   Simpelt program til at sende en frame i DCC format til et tog ved hjælp af interrupt.
   Interface:
   Anbring lokomotivnummeret i den globale variabel lokoadr. f.eks 36.
   Anbring kommandoen i variablen data f.eks. 0X64 forlængs med speed 4, 0X44 baglænge med speed 4.
   Der sendes kontinuerligt frames det er op til dig at sørge for korrekt lokoadr og data.

*/

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
unsigned char regAddress;
unsigned char currentSettingSignalOrSwitch = 0;
unsigned char lastOrder = 0x80;
int output = 3;
int starttal = 3;

long randNumber;
int a[3];


int const maxdata = 16;
unsigned char arraydata[2][maxdata];
int msgIndex = 0;
int byteIndex = 0;
long distance;


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

void randomSpeed(unsigned char lokoAddr) {
  randNumber = random(0, 3);
  assembleAndSendSpeed(a[randNumber], lokoAddr);
  Serial.println(a[randNumber]);   
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
  
  accAddress = ((layoutAddress / 4) + 1) & 63;

  computeSignalSwitchDataByteTwo(1, greenRedStraightTurnBoolean);

  data = signalSwitchDataByteTwo;

  computeSignalSwitchDataByteOne(accAddress);

  assemble_dcc_msg(signalSwitchDataByteOne);

  delay(30);

  computeSignalSwitchDataByteTwo(0, greenRedStraightTurnBoolean);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg(signalSwitchDataByteOne);


}

void computeSignalSwitchDataByteOne (unsigned char accAddress) {
  signalSwitchDataByteOne = accAddress + 128;
}

// TODO: ændring af regaddr virker ikke helt, så når vi eksempelvis vil skifte signal nr 152, fungerer det ikke.
// Dette skal ordnes!
void computeSignalSwitchDataByteTwo (unsigned char fifthBit, unsigned char eigthBit) {
  signalSwitchDataByteTwo = 240;
  if (fifthBit == 1) {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 8;
  } else {
    signalSwitchDataByteTwo = signalSwitchDataByteTwo + 0;
  }

  regAddress = (layoutAddress % 4) - 1;


  if (regAddress < 0) {
    regAddress = 3;
    accAddress = accAddress - 1;
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



void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT); // enable styrepin som output på pin 6
  assemble_dcc_msg(36);
  SetupTimer2();
  
  randomSeed(analogRead(0));
  
  a[0] = 0x62;
  a[1] = 0x69;
  a[2] = 0x6F;

  

}

void loop()
{

  assembleAndSendSpeed(0x60, 07);
  
  /*randomSpeed(07);
  delay(5000);
  assembleAndSendSignalSwitchBytes(101, 1);
  delay(5000);
  assembleAndSendSignalSwitchBytes(101, 0);
  delay(5000);
  assembleAndSendSignalSwitchBytes(102, 1);
  delay(5000);
  assembleAndSendSignalSwitchBytes(102, 0);
  delay(5000);*/
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
