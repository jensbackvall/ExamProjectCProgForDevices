//#include arduino.h

#define DCC_PIN 6                      // DCC out
#define TIMER_SHORT 0x8D               // 58usec pulse length 141 255-141=114
#define TIMER_LONG  0x1B               // 116usec pulse length 27 255-27 =228
#define PREAMBLE  0                    // definitions for state machine
#define SEPERATOR 1                    // definitions for state machine
#define SENDBYTE  2                    // definitions for state machine
#define MAXMSG  2

int msgIndex = 0;
int byteIndex = 0;
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

// TCCR2A, TCCR2B, TIMSK2, TCNT2 as well as TIMER2_OVF_vect i ISR are all defined in the arduino,
// and thus are not defined in this scope, but are accessed via arduino.h
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

void assemble_dcc_msg()
{
  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[1].data[0] = lokoadr;
  msg[1].data[1] = data;
  msg[1].data[2] = lokoadr ^ data;

  interrupts();
}
