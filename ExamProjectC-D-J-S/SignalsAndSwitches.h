
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