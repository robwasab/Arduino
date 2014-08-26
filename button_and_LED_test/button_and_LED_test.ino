#define BUTTON 7
#define GREEN 6
#define RED 14

void setLed(int led, boolean on) 
{
  if (led == RED)
  {
    digitalWrite(RED, on);
    digitalWrite(GREEN, !on);
  }
  if (led == GREEN)
  {
    digitalWrite(GREEN, on);
    digitalWrite(RED, !on);
  }
}

void flash_error_code(int blips)
{
  while(blips)
  {
    setLed(RED, true);
    delay(500);
    setLed(RED, false);
    delay(500);
    blips--;
  } 
}

void setup()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUTTON, INPUT);
}

void loop()
{
   setLed(GREEN, digitalRead(BUTTON));
   setLed(RED,  !digitalRead(BUTTON)); 
}
