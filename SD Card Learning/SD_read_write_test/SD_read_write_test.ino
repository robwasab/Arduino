#include <SdFat.h>
#include <MAX78615_LMU.h>
#include <SPI.h>

/*This file reads the root directory of an SD card. Then reconfigures the SPI module to talk to the Sonoma MAX78615_LMU board.*/

#define SD_CS 8
#define SONOMA_CS 7

SdFat sd;
SdFile myFile;
MAX78615_LMU m;

void configure_spi_sonoma() { m.configure_spi(); }

void configure_spi_sd() 
{
   SPI.setBitOrder(MSBFIRST);
   SPI.setDataMode(SPI_MODE0);
   SPI.setClockDivider(SPI_FULL_SPEED); 
}

void sonoma_readout() 
{
  m.write_register(VFSCALE, 667);
  //vf_scale is an "INT"
  uint32_t vf_scale = m.read_register(VFSCALE);
  //va_rms is a "S.23" floating point number
  uint32_t v_rms   = m.read_register(VA_RMS);
  //freq is a "S.16" number
  uint32_t freq     = m.read_register(FREQ);
  
  Serial.print("VF Scale: ");
  Serial.println(vf_scale);
  
  Serial.print("V RMS: ");
  v_rms *= (vf_scale >> 1);
  v_rms >>= 22;
  Serial.println(v_rms);
  
  Serial.print("Freq: ");
  Serial.println(freq >> 16);
}

void setup()
{
  Serial.begin(115200);
  if (!sd.begin(SD_CS, SPI_FULL_SPEED)) sd.initErrorHalt();

  // open the file for write at end like the Native SD library
  if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
  // if the file opened okay, write to it:
  Serial.print("Writing to test.txt...");
  myFile.println("testing 1, 2, 3.");

  // close the file:
  myFile.close();
  Serial.println("done.");
}

void loop()
{
  if (!myFile.open("test.txt", O_READ)) {
    sd.errorHalt("opening test.txt for read failed");
  }
  Serial.println("test.txt:");

  // read from the file until there's nothing else in it:
  int data;
  while ((data = myFile.read()) >= 0) Serial.write(data);
  // close the file:
  myFile.close();

 configure_spi_sonoma();
 for (int i = 0; i < 10; i++)
 {
    sonoma_readout();
    delay(1000);
 }
 configure_spi_sd();
}

