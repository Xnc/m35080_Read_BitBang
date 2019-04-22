//doesnt contain any transfer16, to avoid more clocks//
#include <SPI.h>

int Chip_Select = SS;
int slaveSelectPin = SS;
int DATAIN = MISO;
int Sclk = SCK;
int DATAOUT = MOSI;
int HWPROTECT = 9;//0-protected, 1-unprotected

//Instruction Set//M35080, M95080, M95080-D
byte WREN = 0b00000110; //Set Write Enable Latch
byte WRDI = 0b00000100; //Reset Write Enable Latch
byte RDSR = 0b00000101; //Read Status Register
byte WRSR = 0b00000001; //Write Status Register
byte READ = 0b00000011; //Read Data from Memory Array
byte WRITE = 0b00000010; //Write Data to Memory Array
byte WRINC = 0b00000111; //Write Data to Secure Array //----------
byte ERASE = 0b01100000; //ERASE//0x60
//"1.Instruction available only for the M95080-D device."
byte RDID = 0b10000011; //Read Identification Page//1.//addr 000000000000 A3 A2 A1 A0
byte WRID = 0b10000011; //Write Identification Page//1.//addr 000000000000 A3 A2 A1 A0
byte RDLS = 0b00000110; //Reads the Identification Page lock status//1.//addr 000001000000000
byte LID = 0b10000010; //Locks the Identification page in read-only mode//1.//addr 000001000000000

byte WIP = 0x01;//Write in progres

int i = 0;
unsigned int starting_address;
int h, l;
int fourhigh;
int readings[9];
int retrieve_readings[9];
int input;
byte statusregister;  


void setup() {
  Serial.println("Boot");
  SPI.begin();
  Serial.begin(9600);
  Serial.flush();
  pinMode(SS, OUTPUT);
  pinMode(Chip_Select, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(Sclk, OUTPUT);
  pinMode(DATAOUT, OUTPUT);
  pinMode(HWPROTECT, OUTPUT);
  digitalWrite(SS, HIGH);  
  Serial.println();
  Serial.flush();
  Serial.println("Done setting up...");
}

void loop() {
  while (Serial.available() <= 0) {
  } 
  byte address = 0x0000;
    int received;
    int cnt = 0x00;
    int index;
    int valToSend;
  
    digitalWrite(SS, LOW);
    send_8(WREN);
    digitalWrite(SS, HIGH);
    digitalWrite(SS, LOW);
    send_8(RDSR);
    status();//in case of status incorrect some areas/half/all chip may be impossible to write so need WRSR to correct it - use it with attention
    digitalWrite(SS, HIGH);
    
  byte aux;   
    digitalWrite(SS, LOW);
    digitalWrite(HWPROTECT, HIGH);//unprotected
    digitalWrite(SS, HIGH);
    
 //---------------------VIN----------------------------------------------------
  //----replace with your VIN address, use any car test program to find KOMBI VIN then search on dump for----//
  int adr = 0x007A;
  char content[] = {0xFF,0xFF,0xFF,0xFF,0xFF};
  for (int i = 0; i < 5 ; i++) {
  //---uncomment for serial write---//
  //write_8(adr, content[i]);
  adr = adr + 0x1;
  }
 //------------------------Errors clear-----------------------------------------------
  adr = 0x0020;//----replace with your errors address, not easy to find it
  for (int i = 0; i < 16 ; i++) {
  //---uncomment for clear errors---//  
  //write_8(adr, 0x00]);
  adr = adr + 0x0001;
  }
 //------------------------Counters clear (usualy service/oil intervals)-----------------------------------------------
  adr = 0x0030;//----replace with your counters address, not easy to find it
  for (int i = 0; i < 16 ; i++) {
  //---uncomment for clear counters---//  
  //write_8(adr, 0x00);
  adr = adr + 0x0001;
 }
 //-----------------Erase non incremental area 0x20 to 0x3FF---------------------
  adr = 0x0020;
  for (int i = 0x20; i < 0x400 ; i++) {
  //---uncomment for clear non inc registers, return area to factory state except incrementals ---//
  //write_8(adr, 0xFF);
  adr = adr + 0x0001;
  }
  status();//check status if not erase 
  //-----------------A try to clear incremental register (not work)
for (int i = 0x00; i < 0x20; i++) 
{
  //---uncomment for clear inc registers, return area to factory state incrementals only---//
  //write_secure(adr, 0x00, 0x00);
  adr = adr + 0x0002;
}
  status();//check status if not erase   
//.................Read and list incremental registers, this is how must be done!!!........
  Serial.print("\n");
  cnt = -1 ;
  byte auxHigh;
  byte auxLow;
  int aux16;
  for (int index = 0x00; index < 0x20; index = index + 0x2) {
    cnt++;
    if (cnt % 16 == 0) {
      Serial.print("\n");//--uncomment next if you need adresses also, or just copy/paste into HxD editor
      /*      if (index <= 0xF) Serial.print("  ");
      else if (index <= 0xFF) Serial.print(" ");
      Serial.print(index, HEX);      
      Serial.print(": ");      */
    }      
    if (index == 0x10) {Serial.print("\n");}
    aux16 = read_16(index);
    auxHigh = (aux16 >> 8) & 0xFF;
    auxLow = aux16 & 0xFF; 
    if (auxHigh <= 0xF) Serial.print("0");
    Serial.print(auxHigh , HEX);
    Serial.print(" "); 
    if (auxLow <= 0xF) Serial.print("0");
    Serial.print(auxLow , HEX);
    Serial.print(" ");  
  }
  //..........Read and list non incremental area............................
  cnt = -1 ;
  for (int index = 0x20; index <= 0x3FF; index = index + 0x1) {
    cnt++;
    if (cnt % 16 == 0) {
      Serial.print("\n");//--uncomment next if you need adresses also, or just copy/paste into HxD editor
      /*      if (index <= 0xF) Serial.print("  ");
      else if (index <= 0xFF) Serial.print(" ");
      Serial.print(index, HEX);      
      Serial.print(": ");      */
    }
    aux = read_8(index);
    if (aux <= 0xF) Serial.print("0");
    Serial.print(aux , HEX);
    Serial.print(" ");    
  }
  SPI.endTransaction();
  while (1) {
  }
}

/*****************************************************/
/*functions*/
void chip_select_low(void) {
  digitalWrite(Chip_Select, LOW);
  delay(5);
}
void chip_select_high(void) {
  digitalWrite(Chip_Select, HIGH);
  delay(5);
}
void sclk() {//THIS MUST BE CORRECTED FOR ONLY ONE TRANSITION, BETTER 2 CALLS FOR FINE TUNING, 
             //EG MODE0,1,2,3 OR SPEED - MAY BE TE REASON FOR NOT WORKING ERASE COMMAND,IF SUPPORTED
  digitalWrite(Sclk, HIGH);
  digitalWrite(Sclk, LOW);
}
//************************************************/
int read_buff() {//ADDED, used in status (xnc)//THE PAIR send_8() and read_buff() is perfect for reading status in midle of writing/erasing chip as no switch up/down chip select
  int value = 0;
    value = SPI.transfer(0);
  return value;
}
//************************************************/
int read_8(int address) {
  int value = 0;
  chip_select_low();
  send_8(READ);
  send_address(address);
    value = SPI.transfer(0);
  chip_select_high();
    delay(3);
  return value;
}
/************************************************/
int read_16(int address) {
  int value = 0;
  chip_select_low();
  chip_select_high();
    delay(3);
  chip_select_low();
    send_8(READ);
    send_address(address);
    value = SPI.transfer(0);
    value = value << 8;
    value += SPI.transfer(0);
  chip_select_high();
    delay(3);
  return value;
}
//************************************************/
void write_8(int address, char dat) {
  chip_select_low();
  send_8(WREN);
  chip_select_high();
  delay(10);
  chip_select_low();
    send_8(WRITE);
    send_address(address);
    send_8(dat);
  chip_select_high();
  delay(100);
  send_8(WRDI);
}
/***********************************************/
/***********************************************/
/***********************************************/
/***********************************************/
void write_secure(int address, char dat1, char dat2) {//CORRECTED !!!  (xnc)
  int value = 0;  
  chip_select_low();
  send_8(WREN);
  chip_select_high();
  delay(5);  
  chip_select_low();
    send_8(WRINC);
    send_address(address);
    send_8(dat1);
    send_8(dat2);
  chip_select_high();
  delay(500);
}
/***********************************************/
/***********************************************/
/***********************************************/
/***********************************************/
void send_8(char dat) {//THE PAIR send_8() and read_buff() is perfect for reading status in midle of writing/erasing chip as no switch up/down chip select, though is done on reading status (xnc)
    SPI.transfer(dat & 0xFF);  
}
/******************************************************************/
void send_address(int address) {
  byte addrHigh;
  byte addrLow;
  addrHigh = (address >> 8) & 0xFF;
  addrLow = address & 0xFF;
  SPI.transfer(addrHigh);
  SPI.transfer(addrLow);
}
/************************************************************/
void status(){
    digitalWrite(SS, LOW);
    SPI.transfer(RDSR);  
    statusregister = SPI.transfer(0);
    Serial.println(statusregister, BIN);
    digitalWrite(SS, HIGH);
    delay(3);
}
/************************ END ... OR NOT YET!!!  **************/
