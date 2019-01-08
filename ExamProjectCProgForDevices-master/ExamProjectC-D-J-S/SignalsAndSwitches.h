#ifndef SIGNALSANDSWITCHES_h
#define SIGNALSANDSWITCHES_h

extern void assembleAndSendSignalSwitchBytes (unsigned char thisLayoutAddress,  unsigned char signalTurn);
extern void computeSignalSwitchDataByteOne (unsigned char accAddress);
extern void computeSignalSwitchDataByteTwo (unsigned char fifthBit, unsigned char eigthBit);

#endif
