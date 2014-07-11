#define BUTTON 5
#define GREEN 4
#define RED 3

void setLed(int led, boolean on)
{
   if (on) digitalWrite(led, LOW);
   else    digitalWrite(led, HIGH);
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
