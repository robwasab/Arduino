#include <MAX78615_PROGRAMMER.h>
#include <SPI.h>

MAX78615_PROGRAMMER m;

void setup()
{
  Serial.begin(115200);
  m.stop_ce();   
  m.reset_wdt();
}

void loop()
{
   static int address = 0;
   static u32 data[64] = {0};
   
   m.read_registers(address, data, 64);
   
   for (u8 i = 0; i < 64; i++) 
   {
     if     (0x10   > data[i]) Serial.print("000");
     else if(0x100  > data[i]) Serial.print("00");
     else if(0x1000 > data[i]) Serial.print("0"); 
     Serial.println(data[i], HEX);
   }   
   address += 64;
   
   if (address >= 0xC00) while(true);
      
   if (address % 64 == 0) m.reset_wdt();
}


