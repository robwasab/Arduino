#include <MAX78615_PROGRAMMER.h>
#include <SdFat.h>
#include <SPI.h>

/*This file reads the root directory of an SD card. Then reconfigures the SPI module to talk to the Sonoma MAX78615_LMU board.*/

#define SD_CS 8
#define SONOMA_CS 7

//Size of a Flash page in words. It actually depends on which page you are referring to.
//The pages 64 to 75 (both inclusive) are 3 byte wide words. All other pages are composed
//of 2 byte words
#define PAGE 64

//Objects required to simply read from a file stored on the SD card. 
SdFat sd;
SdFile myFile;

//Object that handles communicating with the MAX78615 chip in FPI mode.
MAX78615_PROGRAMMER m;

//Prototype. This function converts the 4 character byte instructions mentioned above
//into actual hexadecimal numbers.
u16 convert_to_16_bit(char * line);

//Reconfigure the SPI to talk to the MAX78615/Sonoma board
void configure_spi_sonoma() { m.configure_spi(); }

//Reconfigure the SPI to talk to the SD card. 
void configure_spi_sd() { SPI.setBitOrder(MSBFIRST); SPI.setDataMode(SPI_MODE0); SPI.setClockDivider(SPI_FULL_SPEED); }

void setup()
{
  Serial.begin(115200);
  
  //This is how you start the SD card. Simply copied from SD card examples (Not Arduino
  //Library but the imported SdFatLib 
  if (!sd.begin(SD_CS, SPI_FULL_SPEED)) sd.initErrorHalt();
  
  //List Root Directory
  sd.ls();
  
  //Using the SdFile myFile object, open the file...
  if (!myFile.open("IMU.DAT", O_READ)) { sd.errorHalt("opening IMU.DAT for read failed"); }
  
  //Now its time to switch over to the sonoma board
  //initialize its FPI interface... 
  configure_spi_sonoma();
  
  //Stop the computation engine...
  m.stop_ce();
}

void loop()
{
   //Buffer for reading from the SD card, hence 'file'
   static u16 firmware_file[64];
   
   static boolean file_finish = false, spi_finish = false;
   
   //Buffer for reading from the Sonoma board
   static u16 firmware_dump[64];
   
   static int spi_address = 0;
   
   //Read the number of words inside a PAGE...
   int words_to_read = PAGE;
   
   //Reconfigure for SD card reading
   configure_spi_sd();
   
   //I defined read page below... it is a utility function
   int words_read_file = read_page(myFile, firmware_file, words_to_read);
   
   if (words_read_file < words_to_read) file_finish = true;
   
   configure_spi_sonoma();
   
   //You must cast the firmware_dump buffer as an 8-bit sized buffer.
   int words_read_spi = m.read_registers(spi_address, (u8 *) firmware_dump, words_to_read);
   
   //reset the watchdog
   m.reset_wdt();
   
   spi_address += 64;
   
   if (spi_address >= 0xC00) spi_finish = true;
   
   //Dump both pages you just read.
   for (int i = 0; i < 64; i++)
   {
      u16 file_read = firmware_file[i];
      u16 spi_read  = firmware_dump[i];
     
      //The two words should equal each other...
      //If they don't, then print **** to mark an 
      //error
      if (file_read != spi_read)
      {
        Serial.print("   ****");
      }
      Serial.print("File: "); Serial.print(file_read, HEX); Serial.print(" | SPI: "); Serial.println(spi_read, HEX);
   }
   
   if (file_finish) Serial.println("Done Reading File...");
   if (spi_finish) Serial.println("Done Read SPI...");
     
   if (file_finish || spi_finish) while(true);  
}

u16 convert_to_16_bit(char * line)
{
  //OP_CODE_CHAR_SIZE the character width of the MAX chip's byte instructions stored on an
  //SD card. 
  //For example: 
  //"FFFF"
  //"FEDC"
  //These are 4 characters wide (neglecting the null string) 

   const int OP_CODE_CHAR_SIZE = 4;
  
   char c, temp;
   
   u16 res = 0;
   
   u16 multiplier = 1;   
   
   int i = OP_CODE_CHAR_SIZE - 1;
   do
   {
      c = line[i];
      
      c = (c >= 'A') ? (c - 0x41) + 10 : c - 0x30;
      
      res += c * multiplier;
   
      multiplier *= 16;
   
      --i;
   }while(i >= 0);   
   
   return res;
}

int read_page(SdFile& file, u16 * ret_page, int words_to_read)
{
   int i, result;
   
   static char line[6];
   
   char delim = '\n';
      
   for (i = 0; i < 64; i++)
   {
     result = file.fgets(line, 6, &delim);
   
     if (result == EOF || result <= 0) return i;
   
     ret_page[i] = convert_to_16_bit(line);
   }
   return i;
}

