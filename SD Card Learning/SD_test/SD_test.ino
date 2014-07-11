#include <SdFat.h>
#include <MAX78615_LMU.h>
#include <SPI.h>

/*This file reads the root directory of an SD card. Then reconfigures the SPI module to talk to the Sonoma MAX78615_LMU board.*/

#define SD_CS 8
#define SONOMA_CS 7

Sd2Card card;
SdVolume volume;
SdFile root;

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
  
  CARD_CHECK:
  
  boolean success = card.init(SPI_FULL_SPEED, SD_CS);
  
  if (success == false) { Serial.println("Sd initialization failed"); return; }
  
  uint32_t size = card.cardSize();
  
  SIZE_CHECK:
  
  if (size == 0) { Serial.println("Can't determine the card size.\n"); return; } 
  
  uint32_t sizeMB = 0.000512 * size + 0.5;

  Serial.print("Card size: ");
  Serial.println(sizeMB);
  
  FAT_PARTITION_CHECK:
  
  success = volume.init(&card);
  
  if (success == false) { Serial.println("Can't read the card.\n"); return; }
  
  Serial.print("Volume is FAT"); Serial.println(int(volume.fatType()));
  
  ROOT_DIRECTORY_CHECK:
  
  root.close();
  
  success = root.openRoot(&volume);
  
  if (success == false) { Serial.println("Can't open root directory.\n"); return; }
  
}

void loop()
{
 root.ls(LS_R | LS_DATE | LS_SIZE);
 configure_spi_sonoma();
 for (int i = 0; i < 10; i++)
 {
    sonoma_readout();
    delay(1000);
 }
 configure_spi_sd();
}

