#include "DataFuncs.h"
#include "TrainFuncs.h"

unsigned char lastOrder = 0x80;


void assembleAndSendSpeed(unsigned char newSpeed) {
  //data = newSpeed;
  setData(newSpeed);
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

  //data = newOrder;
  setData(newOrder);

  assemble_dcc_msg();

  lastOrder = newOrder;

  delay(750);
}
