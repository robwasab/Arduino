#define SD_CS 9
#define SO  12
#define SI  11
#define CLK   13
#define GREEN 6
#define RED 14
long time = 0;
boolean clk = false;
int pin = 11;

void setLed(int led, boolean on) 
{
  if (led == RED)
  {
    digitalWrite(RED, on);
    digitalWrite(GREEN, LOW);
  }
  if (led == GREEN)
  {
    digitalWrite(GREEN, on);
    digitalWrite(RED, LOW);
  }
}
void setup()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(SO, OUTPUT);
  pinMode(SI, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  setLed(GREEN, false);
  setLed(RED, true);
  delay(2000);
  time = millis();
}

void port(unsigned char b)
{
  if (b & 0x01) digitalWrite(SD_CS, HIGH);
  else digitalWrite(SD_CS, LOW);
  if (b & 0x02) digitalWrite(SI, HIGH);
  else digitalWrite(SI, LOW);
  if (b & 0x04) digitalWrite(SO, HIGH);
  else digitalWrite(SO, LOW);
  if (b & 0x08) digitalWrite(CLK, HIGH);
  else digitalWrite(CLK, LOW);
}

void loop()
{
  static boolean out = false;  
  static unsigned char b = 1;
  port(b  = (b > 0x08) ? 0x01 : b + 1); 
  setLed(GREEN, out);
  delay(100);
  out ^= true;
}
