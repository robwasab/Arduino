
// communicates using SPI, so include the library:
#include <SPI.h>
// CS pin
const int chipSelectPin = 7;

//Global variables
int loop_count;
unsigned long tempval;
signed long regval;
signed long regval_d;
unsigned int ldelay;
unsigned long pass;
unsigned long time;

void setup() {
  loop_count = 0;
  ldelay = 100;
  pass=0;
  time = millis();
  regval = 0;

  // put your setup code here, to run once:

  // start the serial library
  Serial.begin(230400);

  // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE3);

  // initalize the chip select pin:
  pinMode(chipSelectPin, OUTPUT);
}

void loop() 
{ 
  regval_d = regval;
  tempval = readRegister(0x2);  
  if(tempval & 0x00800000)
  {
    tempval |= 0xFF000000;
  }
  regval = tempval;
    Serial.println(regval-regval_d);
  
  if(regval != regval_d)
  {  
//    Serial.println(regval_d- regval);
    //Serial.println("HI");
  }  else
  {
//    Serial.println("1");
  }
}
unsigned long readRegister(int address) {

  byte outByte0 = (byte)((((address >> 6) & 0x03) << 2) + 0x01); // 1 Access + (page << 2) + !FPI + Type
  byte outByte1 = (byte)(address << 2); // !RW + !Type
  byte outByte2 = 0x00;           // outgoing byte to the SPI
  byte outByte3 = 0x00;           // outgoing byte to the SPI
  byte outByte4 = 0x00;           // outgoing byte to the SPI

  byte inByte0 = 0;           // incoming byte from the SPI
  byte inByte1 = 0;           // incoming byte from the SPI
  byte inByte2 = 0;           // incoming byte from the SPI
  byte inByte3 = 0;           // incoming byte from the SPI
  byte inByte4 = 0;           // incoming byte from the SPI

  digitalWrite(chipSelectPin, LOW);
  inByte0 = SPI.transfer(outByte0);
  inByte1 = SPI.transfer(outByte1);
  inByte2 = SPI.transfer(outByte2);
  inByte3 = SPI.transfer(outByte3);
  inByte4 = SPI.transfer(outByte4);
  digitalWrite(chipSelectPin, HIGH);

  // Serial.println("TX");
  // Serial.println(outByte0);
  // Serial.println(outByte1);

  // Serial.println("RX");
  // Serial.println(inByte2);
  // Serial.println(inByte3);
  // Serial.println(inByte4);

  return((((long)inByte2)<<16)|(((long)inByte3)<<8)|(((long)inByte4)<<0));
}
