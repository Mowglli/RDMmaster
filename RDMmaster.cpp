
#include "RDMmaster.h"
#include "Arduino.h"

byte RDMdataTx[257]; //Til at gemme de nuværende RDM pakke data
byte RDMdataRx[257]; //Til at gemme de nuværende RDM pakke data
byte dmx_data[513]; //Til at gemme de nuværende DMX data slots
unsigned int dmx_data_slots = 0;  //Angiver hvor mange slots der anvendes i DMX512 framen
char print_line[100]; //Til temp print line 

RDMmasterClass RDM;

RDMmasterClass::RDMmasterClass(){}

// Init pin configurations
void RDMmasterClass::initPins()
{
  pinMode(rdmTx, OUTPUT);
  pinMode(rdmRx, INPUT_PULLUP);
  pinMode(rs485DriverEnable, OUTPUT);
  pinMode(rs485RecieverEnable, OUTPUT);
  digitalWrite(rdmTx, HIGH);  //Idle dmx er høj i mark before break (MBB)
  digitalWrite(rs485DriverEnable, HIGH);  //Active high
  digitalWrite(rs485RecieverEnable, HIGH);  //Active low 
  Serial.begin(9600);
}

// Init RDM data
void RDMmasterClass::initRDMdata()
{
  // RDM command
  RDMdataTx[0] = 0xCC;    // START code
  RDMdataTx[1] = 0x01;    // Sub START code
  RDMdataTx[2] = 0x00;    // Msg length (without checksum)
  // Destination UID
  RDMdataTx[3] = 0x4D;    // Manufacturer ID (Martin specific)
  RDMdataTx[4] = 0x50;    // Manufacturer ID (Martin specific)
  RDMdataTx[5] = 0x01;    // RDM ID (Fixture specific)
  RDMdataTx[6] = 0x08;    // RDM ID (Fixture specific)
  RDMdataTx[7] = 0x7F;    // RDM ID (Fixture specific)
  RDMdataTx[8] = 0x2D;    // RDM ID (Fixture specific)
  // Source UID
  RDMdataTx[9] =  0x4D;    // Manufacturer ID (Martin specific)
  RDMdataTx[10] = 0x50;   // Manufacturer ID (Martin specific)
  RDMdataTx[11] = 0x00;   // RDM ID (Source specific)
  RDMdataTx[12] = 0x00;   // RDM ID (Source specific)
  RDMdataTx[13] = 0x00;   // RDM ID (Source specific)
  RDMdataTx[14] = 0x00;   // RDM ID (Source specific)

  RDMdataTx[15] = 0x00;   // Transaction number
  RDMdataTx[16] = 0x01;   // Port ID / Response type
  RDMdataTx[17] = 0x00;   // Msg count
  RDMdataTx[18] = 0x00;   // Sub-device MSB
  RDMdataTx[19] = 0x00;   // Sub-device LSB
  // Parameter data
  RDMdataTx[20] = 0x00;   // Command Class (GET=0x20, SET=0x30)
  RDMdataTx[21] = 0x00;   // Parameter ID MSB
  RDMdataTx[22] = 0x00;   // Parameter ID LSB
  RDMdataTx[23] = 0x00;   // Parameter Data Length    
  RDMdataTx[24] = 0x00;   // Parameter Data MSB
  RDMdataTx[25] = 0x00;   // Parameter Data LSB
  RDMdataTx[26] = 0x00;   // Checksum MSB
  RDMdataTx[27] = 0x00;   // Checksum LSB
}

void RDMmasterClass::setFixtureMode(unsigned int fixtureMode)
{
 //Send RDM fixture mode
  RDMdataTx[2] = 0x19; //Msg length 25 (without checksum)
  RDMdataTx[20] = 0x30; //Command Class (Get/Set)
  RDMdataTx[21] = 0x00; //Parameter Identifier (PID) MSB  
  RDMdataTx[22] = 0xE0; //Parameter Identifier (PID) LSB
  RDMdataTx[23] = 0x01; //Paremeter Data Lenght (PDL)
  RDMdataTx[24] = fixtureMode; //Parameter Data
  
  RDMtransmit(27);  //27 grundet 1 parameter data
  }

// Set DMX address to requested value
void RDMmasterClass::setDMXaddress(uint16_t DMXaddress)
{ 
  RDMdataTx[2] = 0x1A;    // Msg length (without checksum)
  RDMdataTx[20] = 0x30;   // Command Class (GET=0x20, SET=0x30)
  RDMdataTx[21] = 0x00;   // Parameter ID MSB
  RDMdataTx[22] = 0xF0;   // Parameter ID LSB
  RDMdataTx[23] = 0x02;   // Parameter Data Length    
  RDMdataTx[24] = DMXaddress >> 8;      // Parameter Data MSB
  RDMdataTx[25] = DMXaddress & 0x00FF;  // Parameter Data LSB

  RDMtransmit(28);
}

//Send RDM get fixture mode
int RDMmasterClass::getFixtureMode()
{
  RDMdataTx[2] = 0x18; //Msg length 24 (without checksum)
  RDMdataTx[20] = 0x20; //Command Class (Get/Set)
  RDMdataTx[21] = 0x00; //Parameter Identifier (PID) MSB  
  RDMdataTx[22] = 0xE0; //Parameter Identifier (PID) LSB
  RDMdataTx[23] = 0x00; //Paremeter Data Lenght (PDL)
  
  RDMtransmit(26);  //26 grundet 0 parameter data
  if (RDMrecieve(28) == true)  //Hvis der ikke er fejl i modtagelse af svar (28 grundet 2 parameter data)
  {
    //Sender BT return pakke
    //sprintf(print_line,"Fixturemode: %u",RDMdataRx[24]);
    //Serial.println(print_line);
    return RDMdataRx[24];
  }
  else  //Hvis der er fejl i modtagelse af svar
  {
    //Sender BT return pakke
    //sprintf(print_line,"Fixturemode: error");
    //Serial.println(print_line);
    return 0;
  }
  }

// RDM transmit
 void RDMmasterClass::RDMtransmit(unsigned int rdm_data_slots)
{  
  int index = 0;
  
  //RS-485 driver enable and receiver disable
  digitalWrite(rs485DriverEnable, HIGH);  //Active high
  digitalWrite(rs485RecieverEnable, HIGH);  //Active low 
    
  calculate_rdm_checksum(true, false);  //Checksum udregnes og indsættes i globalt rdm_data_tx array  
  
  //Break i minimum 176 us
  digitalWrite(rdmTx, LOW);
  delayMicroseconds(176);  //Min break time
    
  //Mark after break i minimum 12 us
  digitalWrite(rdmTx, HIGH);
  delayMicroseconds(12);  //Min mark after break
  
  Serial1.begin(250000, SERIAL_8N2);   //UART 1 - RDM TX med 1 startbit, 8 databit og 2 stopbit
  
  //Det samlede rdm_data_tx array sendes
  while (index < rdm_data_slots)
  {
    Serial1.write(RDMdataTx[index]);  //Sender dmx data slot
    index++;
  }
  
  Serial1.end();  //Deaktiver UART 1
  pinMode(rdmTx,OUTPUT); //Skift til digital output
  digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
}

int RDMmasterClass::getDMXAdress()
{
  unsigned int temp = 0;
  
  //Send RDM get dmx address
  RDMdataTx[2] = 0x18; //Msg length 24 (without checksum)
  RDMdataTx[20] = 0x20; //Command Class (Get/Set)
  RDMdataTx[21] = 0x00; //Parameter Identifier (PID) MSB  
  RDMdataTx[22] = 0xF0; //Parameter Identifier (PID) LSB
  RDMdataTx[23] = 0x00; //Paremeter Data Lenght (PDL)
  
  RDMtransmit(26);  //26 grundet 0 parameter data
  
  if (RDMrecieve(28) == true)  //Hvis der ikke er fejl i modtagelse af svar (28 grundet 2 parameter data)
  { 
    //Sender BT return pakke
    temp = RDMdataRx[24]; //MSB
    temp = temp << 8;
    temp = temp + RDMdataRx[25]; //LSB
    //sprintf(print_line,"DMX Adress: %u",temp);
    //Serial.println(print_line);
    return temp;
  }
  else  //Hvis der er fejl i modtagelse af svar
  {
    //Sender BT return pakke
    //sprintf(print_line,"DMX Adress: error");
    //Serial.println(print_line);
    return 0;
  }
  
}

boolean RDMmasterClass::calculate_rdm_checksum(boolean tx_flag, boolean rx_flag)
{
  int index = 0;
  unsigned int checksum = 0;
  
  if (tx_flag == true)
  {
    while (index < (RDMdataTx[2]))
    {
      checksum = checksum + RDMdataTx[index];
      index++;
    }
    RDMdataTx[RDMdataTx[2]] = checksum >> 8; //MSB
    checksum = checksum << 8;
    RDMdataTx[RDMdataTx[2] + 1] = checksum >> 8;  //LSB
  }

  if (rx_flag == true)
  {
    while (index < (RDMdataRx[2]))
    {
      checksum = checksum + RDMdataRx[index];
      index++; 
    }
    
    if (RDMdataRx[RDMdataRx[2]] != (checksum >> 8)) //MSB
    {
      return false;
    }
    
    checksum = checksum << 8;
    if (RDMdataRx[RDMdataRx[2] + 1] != (checksum >> 8)) //LSB
    {
      return false;
    }
    
    return true;
  }
}

boolean RDMmasterClass::RDMrecieve(unsigned int rdm_data_slots)
{
  int index = 0;
  byte temp = 0;
  unsigned int frame_size = rdm_data_slots; //Sættes indledningsvist til maskimal tilladt størrelse
  unsigned long timestamp = millis();
  
  //RS-485 driver disable and receiver enable
  digitalWrite(rs485DriverEnable, LOW);  //Active high
  digitalWrite(rs485RecieverEnable, LOW);  //Active low
  
  Serial1.begin(250000, SERIAL_8N2);
  
  while (Serial1.available() == 0)  //Vent på ny karakter modtages i bufferen
  {
    if (millis() > (timestamp + 1000))  //Timeout hvis der ventes længere end 1 sek
    {
      Serial1.end();  //Deaktiver UART 1
      pinMode(rdmTx,OUTPUT); //Skift til digital output
      digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
      
      //RS-485 driver enable and receiver disable
      digitalWrite(rs485DriverEnable, HIGH);  //Active high
      digitalWrite(rs485RecieverEnable, HIGH);  //Active low
      return false;
    }
  }

  delay(100);  //Wait for a complete RDM frame to be received

  temp = Serial1.read();
  timestamp = millis(); //Nulstiller timer
  
  while (temp != 0xCC)  //Vent til CC start kode læses fra buffer
  {
    if (millis() > (timestamp + 1000) || (index >= rdm_data_slots))  //Timeout hvis der ventes længere end 1 sek eller max frame størrelse overskrides
    {
      while (Serial1.available() > 0) //Flush RX buffer in serial 1
      {
        Serial1.read();
      }
      
      Serial1.end();  //Deaktiver UART 1
      pinMode(rdmTx,OUTPUT); //Skift til digital output
      digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
      
      //RS-485 driver enable and receiver disable
      digitalWrite(rs485DriverEnable, HIGH);  //Active high
      digitalWrite(rs485RecieverEnable, HIGH);  //Active low
      return false;
    }
    temp = Serial1.read();
    index++;
  }
  RDMdataRx[0] = temp;  //Ligger 0xCC over i den første plads

  index = 1;
  while (index < frame_size) //Get chars from uart buffer
  {
    RDMdataRx[index] = Serial1.read();
    
    if (index >= rdm_data_slots)  //Hvis max frame størrelse overskrides
    {
      while (Serial1.available() > 0) //Flush RX buffer in serial 1
      {
        Serial1.read();
      }
      Serial1.end();  //Deaktiver UART 1
      pinMode(rdmTx,OUTPUT); //Skift til digital output
      digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
      
      //RS-485 driver enable and receiver disable
      digitalWrite(rs485DriverEnable, HIGH);  //Active high
      digitalWrite(rs485RecieverEnable, HIGH);  //Active low
      
      return false;
    }
    if (index == 2) //Opdater message length når pakke længden er modtaget
    {
      frame_size = RDMdataRx[2] + 2; //Plus to da checksum ikke er med i message længden
    }
    index++;
  }

  while (Serial1.available() > 0) //Flush RX buffer in serial 1
  {
    Serial1.read();
  }
  
  Serial1.end();  //Deaktiver UART 1
  pinMode(rdmTx,OUTPUT); //Skift til digital output
  digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
  
  //RS-485 driver enable and receiver disable
  digitalWrite(rs485DriverEnable, HIGH);  //Active high
  digitalWrite(rs485RecieverEnable, HIGH);  //Active low
  
  index = 0;
  while (index < 6) //Hvis pakke destination UID ikke er til controllerens source UID
  {
    if (RDMdataRx[index + 3] != RDMdataTx[index + 9])
    {
      return false;
    }
    index++;
  }
  
  if (RDMdataRx[16] != 0) //Hvis RDM response type ikke er ACK
  {
    return false;
  }
  
  if (calculate_rdm_checksum(false, true) == false)  //Hvis checksum ikke er korrekt
  {
    return false;
  }
  return true;
}

void RDMmasterClass::DMXtransmit(unsigned int channel_number, unsigned int channel_value)
{ 
  dmx_data[channel_number] = channel_value;
  int index = 0;
  int dmx_data_slots_diff = 0;
  static unsigned long timestamp = micros(); //Sættes kun første gang
  
  
  if (micros() < timestamp) //Hvis der er sket overflow nulstilles timestamp og returnerer
  {
    timestamp = micros();
    return;
  }

  if (micros() < (timestamp + 22728)) //Hvis minimum periodetid på 44 Hz ikke er gået imellem frames skal den returnere
  {
    return;
  }
  else  //Hvis minimum periodetid er gået sendes en frame og timestamp nulstilles
  { 
    timestamp = micros(); //Timestamp nulstilles
    
    //RS-485 driver enable and receiver disable
    digitalWrite(rs485DriverEnable, HIGH);  //Active high
    digitalWrite(rs485RecieverEnable, HIGH);  //Active low 
    
    //Break i minimum 92 us
    digitalWrite(rdmTx, LOW);
    delayMicroseconds(92);  //Min break time
    
    //Mark after break i minimum 12 us + min frame lenght
    digitalWrite(rdmTx, HIGH);
    if (dmx_data_slots >= 24) //Hvis minimum frame tid kan overholdes (start slot + 24 data slots + min break + min mark after break = 1204 us)
    {
      delayMicroseconds(12);  //Min mark after break
    }
    else //Hvis minimum frame tid ikke kan overholdes
    {
      dmx_data_slots_diff = 24 - dmx_data_slots; 
      delayMicroseconds(12);  //Min mark after break
      delayMicroseconds(dmx_data_slots_diff*44);  //Ekstra break tid for at overholdes 1204 us (44 us svarer til en dmx data slot tid)
    }
  
    Serial1.begin(250000, SERIAL_8N2);   //UART 1 - DMX TX med 1 startbit, 8 databit og 2 stopbit
    
    dmx_data[0] = 0x00;  //Start code is null
    
    while (index <= dmx_data_slots)
    {
      Serial1.write(dmx_data[index]);  //Sender dmx data slot
      index++;
    }
    Serial1.end();  //Deaktiver UART 1
    pinMode(rdmTx,OUTPUT); //Skift til digital output
    digitalWrite(rdmTx, HIGH);  //Idle er høj i mark before break (MBB)
  }
}
