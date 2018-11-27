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
unsigned char lokoadr = 40;            // global variabel adresse
unsigned char data = 96;               // global variabel kommando
unsigned char layoutAddress = 102;      // global variabel layoutAdresse
unsigned char accAddress = 0;
unsigned char signalSwitchDataByteOne = 0;
unsigned char signalSwitchDataByteTwo = 0;
unsigned char regAddress;
unsigned char currentDirection102 = 0;
unsigned char currentDirection101 = 1;
unsigned char lastOrder = 0x80;
unsigned char oldLokoAdr;
int output = 3;
int starttal = 3;

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

//int getspeed()
//{
//  sensorValue = analogRead(analogin);
//  outputValue = map(sensorValue, 0, 1023, 0, 32);
//  if (outputValue < 16)
//  {
//    speedValue = 0X40 + (15 - outputValue);
//  }
//  else
//  {
//    speedValue = 0X60 + (outputValue - 16);
//  }
//  //Serial.println(speedValue,HEX);
//  if ((speedValue != 0x41) && (speedValue != 0X61) && (speedValue != 0x70)) {
//    Serial.println(speedValue, HEX);
//    return speedValue;
//  }
//
//}


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

void numpadFunctionality() {

  if (output >= 6) {
    output = 3;
    starttal = 3;
  }

  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(output, LOW);
  if (digitalRead(11) == LOW) {
    if (starttal == 1) {
      assembleAndSendSpeed(0x62);
      Serial.println("1 pressed");

    } else if (starttal == 2) {
      assembleAndSendSpeed(0x69);
      Serial.println("2 pressed");

    } else if (starttal == 3) {
      assembleAndSendSpeed(0x6F);
      Serial.println("3 pressed");
    }
  } else if (digitalRead(10) == LOW) {
    if (starttal + 3 == 4) {
      assembleAndSendSpeed(0x42);
      Serial.println("4 pressed");

    } else if (starttal + 3 == 5) {
      assembleAndSendSpeed(0x49);
      Serial.println("5 pressed");

    } else if (starttal + 3 == 6) {
      assembleAndSendSpeed(0x4F);
      Serial.println("6 pressed");
    }
  } else if (digitalRead(9) == LOW) {
    if (starttal + 6 == 7) {
      assembleAndSendOrder(7);
      Serial.println("7 pressed");

    } else if (starttal + 6 == 8) {
      assembleAndSendOrder(8);
      Serial.println("8 pressed");

    } else if (starttal + 6 == 9) {
      assembleAndSendOrder(9);
      Serial.println("9 pressed");
    }
  } else if (digitalRead(8) == LOW) {
    if (output == 5) {
      Serial.println("* pressed");
      assembleAndSendSignalSwitchBytes(102);
    } else if (output == 4) {
      assembleAndSendSpeed(0x61);
      Serial.println("0 pressed");
    } else if (output == 3) {
      Serial.println("# pressed");
      assembleAndSendSignalSwitchBytes(101);
    }
  }
  output++;
  starttal--;

}

void assembleAndSendSpeed(unsigned char newSpeed) {
  data = newSpeed;
  assemble_dcc_msg();
  delay(750);
}

void assembleAndSendOrder(unsigned char trainFunction) {

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
  
  assemble_dcc_msg();

  lastOrder = data;

  delay(750);
}


void assembleAndSendSignalSwitchBytes (unsigned char thisLayoutAddress) {

  unsigned char signalTurn = 0;

  if (thisLayoutAddress == 102) {

    if (currentDirection102 == 0) {
      currentDirection102 = 1;
    } else {
      currentDirection102 = 0;
    }

    signalTurn = currentDirection102;
  }


  if (thisLayoutAddress == 101) {

    if (currentDirection101 == 0) {
      currentDirection101 = 1;
    } else {
      currentDirection101 = 0;
    }

    signalTurn = currentDirection101;
  }

  layoutAddress = thisLayoutAddress;

  accAddress = ((layoutAddress / 4) + 1) & 63;

  computeSignalSwitchDataByteTwo(1, signalTurn);

  data = signalSwitchDataByteTwo;

  computeSignalSwitchDataByteOne(accAddress);

  oldLokoAdr = lokoadr;

  lokoadr = signalSwitchDataByteOne;

  assemble_dcc_msg();

  delay(30);

  computeSignalSwitchDataByteTwo(0, signalTurn);

  data = signalSwitchDataByteTwo;

  assemble_dcc_msg();

  //delay(750);

  lokoadr = oldLokoAdr;
}

void computeSignalSwitchDataByteOne (unsigned char accAddress) {
  signalSwitchDataByteOne = accAddress + 128;
}

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


void twoTrains () {
  if (distance < 675) {
    assembleAndSendSignalSwitchBytes(102);
    delay(1000);
    data = 0x61;
    assemble_dcc_msg();
    delay(30);
    if (lokoadr == 9){
      lokoadr = 40;
      data = 0x6F;
    assemble_dcc_msg();
    } else {
      lokoadr = 9;
      data = 0x6C;
      assemble_dcc_msg();
    }
  }
}
















void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT); // enable styrepin som output på pin 6
  assemble_dcc_msg();
  SetupTimer2();

  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);

pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
pinMode(echoPin, INPUT); // Sets the echoPin as an Input
}

void loop()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  distance = pulseIn(echoPin, HIGH);
  Serial.println(distance);

  twoTrains();

  numpadFunctionality();

  Serial.println(lokoadr);

}



void assemble_dcc_msg()
{
  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[1].data[0] = lokoadr;
  msg[1].data[1] = data;
  msg[1].data[2] = lokoadr ^ data;
  //Serial.print(lokoadr);
  //Serial.print("  ");
  //Serial.print(data);
  //Serial.println();

  interrupts();
}
