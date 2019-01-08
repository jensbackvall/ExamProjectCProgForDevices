#ifndef TRAIN_SIGNAL_SWITCH_FUNCTIONS
#define TRAIN_SIGNAL_SWITCH_FUNCTIONS

void assembleAndSendSpeed(unsigned char newSpeed, unsigned char lokoAddr);
void assembleAndSendSignalSwitchBytes (unsigned char switchOrSignalAddress, int greenRedStraightTurnBoolean);
void computeSignalSwitchDataByteTwo (unsigned char fifthBit, unsigned char eigthBit);
void randomSpeed(unsigned char lokoAddr);
void rideTwoTrainsIntoTheHorizon(int loko1, int loko2);
void assemble_dcc_msg(unsigned char lokoAddr);



#endif
