#include <MAX78615_PROGRAMMER.h>
#include <SdFat.h>
#include <SPI.h>
#include <FreeMemory.h>

#define SD_CS 8

#define SONOMA_CS 7

/*Size of a Flash page in words. It actually depends on which page you are referring to.
 *The pages 64 to 75 (both inclusive) are 3 byte wide words. All other pages are composed
 *of 2 byte words
 */
#define PAGE_SIZE 64

#define LAST_PAGE 64

#define MAX_TRYS 10

#define BUTTON 5
#define GREEN 4
#define RED 3


#define FOREVER true

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ERROR CODES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define SD_INIT_ERROR           1
#define SD_OPEN_FILE_ERROR      2
#define VERIFY_FIRMWARE_ERROR   3
#define MASS_ERASE_FAILIURE     4
#define SD_PAGE_LOAD_ERROR      5
#define WRITE_PAGE_ERROR        6

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PROTOTYPES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*Prototype. This function converts the 4 character byte instructions mentioned above
 *into actual hexadecimal numbers.
 */
u16 convert_to_16_bit(char line[]);
boolean verify_firmware_image(SdFile& file);
boolean check(char c);
void pass_fail(const char * str, boolean res);
boolean erase_flash();
void print_trys(int trys);
boolean sd_get_next_page(u16 buffer[]);
boolean sd_get_page(u16 buffer[], u8 page_num);
boolean write_page(const u16 source[], int page_num);
void finish();
void flash(int blips);

/*
 *returns the number of trys it took to verify the page
 *returns -1 if it ran out of trys
 */
int repeatively_write_page(const u16 * source, int page_num);

int last_verify();

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~GLOBAL VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//Objects required to simply read from a file stored on the SD card. 
SdFat sd;
SdFile myFile;

u16 _page[PAGE_SIZE];
u16 _error_code;
u8 _spi_speed = SPI_FULL_SPEED;


//Object that handles communicating with the MAX78615 chip in FPI mode.
MAX78615_PROGRAMMER m;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SIMPLE FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//Reconfigure the SPI to talk to the MAX78615/Sonoma board
void configure_spi_sonoma() { m.configure_spi(); }

//Reconfigure the SPI to talk to the SD card. 
void configure_spi_sd() { SPI.setBitOrder(MSBFIRST); SPI.setDataMode(SPI_MODE0); SPI.setClockDivider(_spi_speed); }

void setLed(int led, boolean on) { if (on) digitalWrite(led, LOW); else digitalWrite(led, HIGH); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~S E T U P~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void setup()
{
  Serial.begin(115200);
  
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUTTON, INPUT);

  setLed(GREEN, false);
  setLed(RED, false);
  
  Serial.print(F("Free Memory: ")); Serial.println(freeMemory());
    
  Serial.println(F("Initializing SD Card"));
    
  boolean passed;
  
  int trys = 0;
  
  _spi_speed = SPI_FULL_SPEED;
  
SD_INIT:
  
    configure_spi_sd();
  
    passed = sd.begin(SD_CS, _spi_speed);
   
    trys++;
    
    //if you didn't get it to work the first time
    //slow down the clock speed until it you're running
    //at the slowest speed
    if (!passed) 
    {
      if (_spi_speed < SPI_SIXTEENTH_SPEED){ _spi_speed <<= 1; Serial.println("Slowing down SD SPI CLK..."); }
      
      //goto error handler
      else { _error_code = SD_INIT_ERROR; goto INIT_FAILIURE; }
     
      goto SD_INIT;
    }
  
  print_trys(trys);
  
  Serial.println(F("Opening Image..."));
  
  trys = 0;
  
SD_OPEN_FILE:
  
  passed = myFile.open("IMAGE.DAT", O_READ);
  
  trys++;
    
  if (!passed)
  {
    if (trys >= 3) { _error_code = SD_OPEN_FILE_ERROR;  goto INIT_FAILIURE;  }
      
    goto SD_OPEN_FILE;
  }
  
  print_trys(trys);
  
  Serial.println(F("Verifying Firmware..."));
  
  trys = 0;
  
VERIFY_FIRMWARE:
  
  myFile.rewind();
  
  passed = verify_firmware_image(myFile);
  
  trys++;
    
  if (!passed)
  {
    if (trys >= 3) { _error_code = VERIFY_FIRMWARE_ERROR;  goto INIT_FAILIURE;  }
      
    goto VERIFY_FIRMWARE;
  }
  print_trys(trys);
  
  return;
  
INIT_FAILIURE:

   switch(_error_code)
   {
     case SD_INIT_ERROR:
     
     break;
     case SD_OPEN_FILE_ERROR:
     
     break;
     case VERIFY_FIRMWARE_ERROR:
     
     break; 
   }
   Serial.println(F("Unable to fix failiure... Power cycle programmer"));

   while(FOREVER)
   {
      flash(_error_code);
      delay(2000);
   }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~L O O P~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void loop()
{
  static unsigned long time_begin; 
  static boolean passed;
  static int programming_trys, trys, write_trys, page_num;
  float seconds;
    
WAIT_FOR_BUTTON:
   
  programming_trys = 0;

  setLed(GREEN, true);
  
  Serial.println("Ready!");
  
  while(!digitalRead(BUTTON));
  
  setLed(GREEN, false);
    
  time_begin = millis();

/*Label not used for a try/looping scheme
 *Used to restart the programming sequence
 *without having to initialize the SD Card
 */
START_PROGRAM:

  programming_trys++;

  configure_spi_sd();

  myFile.rewind();
  
  configure_spi_sonoma();
  
  m.reset_wdt();
  
  m.turn_off_SPI_time_out();
  
  //Stop the computation engine...
  m.stop_ce();

  Serial.println(F("Mass Erase..."));

  trys = 0;
  
ERASE:

  trys++;
    
  passed = erase_flash();
  
  if (!passed) 
  {
    if (trys >= MAX_TRYS)
    {
      _error_code = MASS_ERASE_FAILIURE;   goto FAILIURE; 
    }
    goto ERASE; 
  }    
  print_trys(trys);  
         
  Serial.println(F("Starting Write Cycle..."));
  
  for (page_num = 0; page_num < LAST_PAGE; page_num++)
  {
    //Serial.print(F("Writing Page...")); Serial.println(page_num);
    
    trys = 0;
    //First, try to read a page with the efficient file iterator
    passed = sd_get_next_page(_page);
    
    trys++;
    
    if (!passed)
    {
      Serial.println(F("Retrying Page Load..."));
      
      /*The iterator failed at reading a page
       *Try to read again for however many trys
       *We allow. However, we will have to use
       *The more inefficient sd_get_page function 
       */
      RETRY_SD_PAGE_READ:
        
        passed = sd_get_page(_page, page_num);  
        
        trys++;
        
        if (!passed)
        {
          if (trys > MAX_TRYS)
          {
            Serial.print(F("SD page load # ")); Serial.print(page_num); Serial.println(F(" has failed..."));
            
            _error_code = SD_PAGE_LOAD_ERROR;   goto FAILIURE;
          }
          goto RETRY_SD_PAGE_READ;
        }
    }
    print_trys(trys);
     
    trys = 0;
    /*function repeatively_write_page is different...
     *it will not follow the Label: goto; scheme we have been
     *using to repeatively retry a step in our program...
     *repeatively_write_page already does this inside it...
     *it is simply a helper function.
     */
    trys = repeatively_write_page(_page, page_num);
     
    if (trys == -1)
    {
      Serial.print(F("Failiure... Couldn't write page # ")); Serial.println(page_num); 
      
      _error_code = WRITE_PAGE_ERROR;   goto FAILIURE;
    }
    print_trys(trys);
  }  
  
  Serial.println(F("Final Verification..."));
  
  int page_fail;
  
  trys = 0;
  
  configure_spi_sd();
  
  myFile = SdFile();
  
  passed = myFile.open("IMAGE.DAT", O_READ);

  if (!passed)
  {
    _error_code = SD_PAGE_LOAD_ERROR;
    goto FAILIURE;
  }
  
  LAST_VERIFY:
    
    //returns the page it failed at... duh!
    //returns -1 for OK
    page_fail = last_verify();
     
    trys++;
    
    //if page_fail is -2 SD error occurred..
    if (page_fail == -2) 
    {
      _error_code = SD_PAGE_LOAD_ERROR;   goto FAILIURE;
    }
  
    if (page_fail >= 0) 
    {
      Serial.print(F("Attempting to re-write page ")); Serial.println(page_fail);
        
      passed = sd_get_page(_page, page_fail);
 
      if(!passed) 
      {
        if (trys >= 3)
        {
          Serial.println(F("While trying to re-write a page during verification..."));
          Serial.print  (F("Couldn't re-read SD card page # ")); Serial.println(page_fail);
          
          _error_code = SD_PAGE_LOAD_ERROR;   goto FAILIURE;
        }
      }
      write_trys = repeatively_write_page(_page, page_fail);
    
      if (write_trys == -1 || trys >= MAX_TRYS) 
      {
        Serial.print(F("Couldn't re-write page # ")); Serial.println(page_fail);
        
        _error_code = WRITE_PAGE_ERROR;   goto FAILIURE;
      }
      print_trys(write_trys);
      
      Serial.println(F("Attempting re-verification..."));
    
      goto LAST_VERIFY;    
    }
    
  Serial.println(F("Finished Programming!"));
  
  seconds = (float)(millis() - time_begin)/1000.0;
  
  Serial.print(F("Took ")); Serial.print(seconds); Serial.println(" secs");
  
  Serial.println(F("Resetting..."));
  
  m.lock();  
  
  goto WAIT_FOR_BUTTON;

FAILIURE:
  
  switch (_error_code)
  {
    case MASS_ERASE_FAILIURE:

      Serial.println(F("Mass Erase Failiure..."));
      
      if (programming_trys <= 3)
      {
        Serial.print  (F("Starting programming sequence, try # ")); 

        Serial.println(programming_trys + 1);

        goto START_PROGRAM;
      }
      break;
        
    case SD_PAGE_LOAD_ERROR:
    
      Serial.println(F("SD read page error..."));
       
      if (programming_trys <= 3)
      {
        Serial.println(F("Attempting reinitialization..."));
       
        setup();

        Serial.print  (F("Starting programming sequence, try # ")); 

        Serial.println(programming_trys + 1);

        goto START_PROGRAM;
      }
      break;

    case WRITE_PAGE_ERROR:
    
      Serial.println(F("Write page error..."));

      if (programming_trys <= 3)
      {
        Serial.print(F("Starting programming sequence to re-erase flash... try # "));
       
        Serial.println(programming_trys + 1);
        
        goto START_PROGRAM;
      }
      break;
  }
  
  Serial.println(F("Unable to fix failiure... Power cycle programmer"));
  Serial.println(programming_trys);
  
  while(FOREVER)
  {
    flash(_error_code);
    delay(2000); 
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void print_trys(int trys) { if (trys > 1) { Serial.print(F("Took ")); Serial.print(trys); Serial.println(F(" Trys")); } }

/*Returns -2 if it encounters a
 *SD card read error.
 *
 *Returns -1 for no errors
 *
 *Returns positive numbers 0-47 
 *inclusive for pages it encounters 
 *erros in.
 */

int last_verify()
{
  int page_num, trys;
  
  boolean sd_read_success, page_pass;  

  configure_spi_sd();
  
  myFile.rewind();
  
  for (page_num = 0; page_num < LAST_PAGE; page_num++)
  { 
    
    sd_read_success = sd_get_next_page(_page);
    
    if (!sd_read_success) 
    { 
       return -2;
    }
    configure_spi_sonoma();
    
    trys = 0;
    
    VERIFY:
    
      trys++;
   
      m.reset_wdt();
      
      page_pass = m.verify_page(page_num, _page);

      if (trys >= MAX_TRYS) return page_num;
      
      if (!page_pass) { goto VERIFY; }
   
    if (trys > 1)
    {
      Serial.print(F("Verification of Page # ")); Serial.print(page_num);
      Serial.print(F(" took ")); Serial.print(trys); Serial.println(" trys...");
    }  
  }
  return -1;
}

boolean erase_flash()
{
  configure_spi_sonoma();
  
  m.reset_wdt();
  
  m.mass_erase();
   
  int page, word_addr;
  u32 foo;
  boolean no_unlock = false;
  //verify
  
  for (page = 0; page < LAST_PAGE; page++)
  {
    m.reset_wdt();
    
    m.read_registers(page << 6, (u8*)_page, PAGE_SIZE);
    
    for (word_addr = 0; word_addr < 64; word_addr++)
    {
      if (_page[word_addr] != 0xFFFF)
      {
        foo = m.read_register((page << 6) + word_addr, no_unlock);
        if (foo == 0xFFFF) continue;
        
        foo = m.read_register((page << 6) + word_addr, no_unlock);
        if (foo == 0xFFFF) continue;
        
        Serial.print(F("Erasing Flash Failed at page ")); Serial.print(page); Serial.print(F(" address: ")); Serial.println(word_addr);
        Serial.print(F("SPI: ")); Serial.print(_page[word_addr], HEX); Serial.print(" != "); Serial.println(0xFFFF, HEX);
        return false; 
      }
    } 
  }
  return true;
}

/* Acts as kind of an iterator for getting data out of the SD card
 * Each call consumes words_to_read out of the SD card
 * You cannot specify which line in the file to go to.
 */
boolean sd_get_next_page(u16 buffer[])
{
  //Reconfigure for SD card reading
  configure_spi_sd();
   
  //I defined read page below... it is a utility function
  int words_read = read_page(myFile, buffer, PAGE_SIZE);  
  
  return words_read == PAGE_SIZE;
}

/* Allows you to read a specific page from the image stored on the 
 * SD card. However, it is inefficient...
 * Order N behavior
 */
boolean sd_get_page(u16 buffer[], u8 page_num)
{
  myFile.rewind();
  
  page_num++;
  
  boolean success = true;
  
  do
  {
    //consume a page...
    success &= sd_get_next_page(buffer);
  
    page_num--;
    
  }while(page_num);
  
  return success;
}

int repeatively_write_page(const u16 source[], int page_num)
{
  int trys = 0;
  boolean write_success;
  
  WRITE:
     
    trys++;

    if (trys >= 3 ) return -1;
        
    write_success = write_page(source, page_num);
        
    if(!write_success) { goto WRITE; }
 
  return trys;
}

boolean write_page(const u16 source[], int page_num)
{
  boolean no_unlock = false;
  configure_spi_sonoma();
  
  m.reset_wdt();
  m.unlock();
  for (int word_addr = 0; word_addr < PAGE_SIZE; word_addr++)
  {
   m.write_register((page_num << 6) + word_addr, source[word_addr], no_unlock);
  }
  
  int verify_trys = 3;  
  //try to verify at least two times
  do 
  {   
    m.reset_wdt();
    
    if ( m.verify_page(page_num, source) ) return true;
  }
  while(--verify_trys);
    
  return false;
}

void pass_fail(const char str[], boolean res)
{
   if (res) { Serial.print(F("Passed: ")); Serial.println(str); }
   else     { Serial.print(F("Failed... ")); Serial.println(str); while(true); } 
}


/* Verify the firmware image. 
 * Make sure each line has 4 hex characters.
 * Make sure the image is 4k words long
 */
boolean verify_firmware_image(SdFile& file)
{
   int i, result, page = 0;
   float progress;
   
   static char instr[6];
   
   configure_spi_sd();
      
   for (i = 0; i <= FLASH_PROGRAM_END; i++)
   {
     result = file.fgets(instr, 6);
   
     if (result == EOF || result <= 0) { Serial.println(F("file.fgets error...")); return false; }
   
     boolean is_valid = check(instr[0]) && check(instr[1]) && check(instr[2]) && check(instr[3]);
    
     if (is_valid == false) 
     {
        Serial.println(F("Character Check Failed..."));
        Serial.print(  F("On line ")); Serial.println(i);
        Serial.println((int)instr); 
        return false;
      }      
   }   
   return true;    
}

boolean check(char c) { return ('A' <= c && c <= 'F') || ('0' <= c && c <= '9'); }

u16 convert_to_16_bit(char line[])
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

int read_page(SdFile& file, u16 ret_page[], int words_to_read)
{
   int i, result;
   
   char line[6];
   
   //Serial.println("READING SD CARD PAGE_SIZE");
      
   for (i = 0; i < PAGE_SIZE; i++)
   {
     result = file.fgets(line, sizeof(line));
   
     if (result == EOF || result <= 0) return i;
   
     ret_page[i] = convert_to_16_bit(line);
     
     //Serial.print("   "); Serial.println(ret_page[i], HEX);
   }
   return i;
}

void flash(int blips)
{
  setLed(GREEN, false);
  
  while(blips)
  {
    setLed(RED, true);
    delay(250);
    setLed(RED, false);
    delay(250);
    blips--;
  } 
}
