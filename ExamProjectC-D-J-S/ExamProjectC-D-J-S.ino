/*
   Simpelt program til at sende en frame i DCC format til et tog ved hjælp af interrupt.
   Interface:
   Anbring lokomotivnummeret i den globale variabel lokoadr. f.eks 36.
   Anbring kommandoen i variablen data f.eks. 0X64 forlængs med speed 4, 0X44 baglænge med speed 4.
   Der sendes kontinuerligt frames det er op til dig at sørge for korrekt lokoadr og data.

*/
#include "SignalsAndSwitches.h"
#include "TrainFuncs.h"
//int const maxdata = 16;
//unsigned char arraydata[2][maxdata];

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DCC_PIN, OUTPUT); // enable styrepin som output på pin 6
  assemble_dcc_msg();
  SetupTimer2();

}

void loop()
{
  

}
