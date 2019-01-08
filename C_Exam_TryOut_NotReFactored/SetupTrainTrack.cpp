#include "SetupTrainTrack.h"
#include <arduino.h>
#include <stdio.h>
#include "TrainSignalSwitchFunctions.h"

int outerTrackSignals[] = {152, 142, 141, 122, 121, 131, 132, 151};
int innerTrackSignals[] = {112, 102, 101, 82, 81, 91, 92, 111};
int outerTwoTracksSwitches[] = {252, 244, 250, 242, 249, 241, 251, 243};

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
