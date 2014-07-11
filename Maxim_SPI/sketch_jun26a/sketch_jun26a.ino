#include <MAX78615_LMU.h>
#include <SPI.h>

MAX78615_LMU m;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  static uint32_t reg_content, test = 1;
  
  u8 addr = SCRATCH1;
  Serial.print("Register Addr: ");
  Serial.println(addr, HEX);
  Serial.print("Send: 0x");
  Serial.println(test, HEX);
  
  m.write_register(addr, test);
  
  test = (test & 0x800000) ? 1 : test << 1;
  
  reg_content = m.read_register(addr);
  Serial.print("Read: 0x");
  Serial.println(reg_content, HEX);
  delay(1000);
}
