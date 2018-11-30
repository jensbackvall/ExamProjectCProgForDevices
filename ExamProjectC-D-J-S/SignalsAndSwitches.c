#include "DataFuncs.h"
#include "SignalsAndSwitches.h"

unsigned char accAddress = 0;
unsigned char signalSwitchDataByteOne = 0;
unsigned char signalSwitchDataByteTwo = 0;
unsigned char regAddress;
unsigned char oldLokoAdr;

unsigned char layoutAddress = 102;      // global variabel layoutAdresse


void assembleAndSendSignalSwitchBytes (unsigned char thisLayoutAddress, unsigned char signalTurn) {

  layoutAddress = thisLayoutAddress;

  accAddress = ((layoutAddress / 4) + 1) & 63;

  computeSignalSwitchDataByteTwo(1, signalTurn);

  //data = signalSwitchDataByteTwo;
  setData(signalSwitchDataByteTwo);

  computeSignalSwitchDataByteOne(accAddress);

1  oldLokoAdr = lokoadr;

  lokoadr = signalSwitchDataByteOne;

  assemble_dcc_msg();

  delay(30);

  computeSignalSwitchDataByteTwo(0, signalTurn);

  //data = signalSwitchDataByteTwo;
  setData(signalSwitchDataByteTwo);

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
