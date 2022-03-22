#ifndef RDMmaster_h
#define RDMmaster_h

#include <Arduino.h>

#define rdmTx 18
#define rdmRx 19
#define rs485DriverEnable 3
#define rs485RecieverEnable 2

class RDMmasterClass
{

  public:
    RDMmasterClass();

    void initPins();
    void initRDMdata();
    void setFixtureMode(unsigned int fixtureMode);
    void setDMXaddress(uint16_t DMXaddress);
    int getFixtureMode();
    void RDMtransmit(unsigned int rdm_data_slots);
    int getDMXAdress();
    boolean calculate_rdm_checksum(boolean tx_flag, boolean rx_flag, boolean disc_rx_flag);
    boolean RDMrecieve(unsigned int rdm_data_slots);
    void DMXtransmit(unsigned int channel_number, unsigned int channel_value);
};
//extern RDMmasterClass RDMmaster;

#endif