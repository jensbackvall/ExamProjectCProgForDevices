#include "TrainSignalSwitchFunctions.h"
#include <arduino.h>
#include <stdio.h>


// Function for setting the speed newSpeed of a specific train with address lokoAddr
void assembleAndSendSpeed(unsigned char newSpeed, unsigned char lokoAddr, unsigned char data) {
  data = newSpeed;
  assemble_dcc_msg(lokoAddr);
  delay(10);
}


// Function for controlling the boolean value of a switch or a signal
void assembleAndSendSignalSwitchBytes (unsigned char switchOrSignalAddress, int greenRedStraightTurnBoolean, unsigned char data) {

  Serial.println("LAYOUTADDRESS");
  Serial.println(switchOrSignalAddress);

  int accAddress = ((switchOrSignalAddress / 4) + 1) & 63;

  regAddress = ((switchOrSignalAddress % 4) - 1);
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



// TODO: ændring af regaddr virker ikke helt, så når vi eksempelvis vil skifte signal nr 152, fungerer det ikke.
// Dette skal ordnes!
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



void randomSpeed(unsigned char lokoAddr) {
  randNumber = random(0, 3);
  assembleAndSendSpeed(a[randNumber], lokoAddr);
  delay(2000);
  Serial.println(a[randNumber]);
}

void rideTwoTrainsIntoTheHorizon(int loko1, int loko2){
  Serial.println("Random værdier for loko1");
  randomSpeed(loko1);
  Serial.println("Random værdier for loko2");
  randomSpeed(loko2);
}

void assemble_dcc_msg(unsigned char lokoAddr, unsigned char data, struct Message msg[MAXMSG])
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
