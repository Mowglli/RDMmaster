#include "RDMmaster.h"

  RDMmasterClass RDM;
  int DMX;
  int fixturemode = 0;
  int DMXAdress = 0;
void setup(){

  
  RDM.initPins();
  RDM.initRDMdata();
  RDM.setDMXaddress(100);
  delay(1000);
  RDM.setFixtureMode(1);
  delay(1000);
  fixturemode = RDM.getFixtureMode();
  Serial.print("FixtureMode: ");
  Serial.println(fixturemode);
  DMXAdress = RDM.getDMXAdress();
  Serial.print("DMXAdress: ");
  Serial.println(DMXAdress);

  //RDM.DMXtransmit(1,20);
  //RDM.DMXtransmit(2,50);
  //RDM.DMXtransmit(11,255);
}

void loop() {
}
