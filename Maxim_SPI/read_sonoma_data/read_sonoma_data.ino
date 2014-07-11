#include <MAX78615_LMU.h>
//Have to include. MAX78615_LMU.h uses the SPI library, including SPI.h here, 
//allows the MAX7815_LMU.h library to gain access to SPI.h when it is compiled. 
#include <SPI.h>

void print_configuration_register();
void print_instantaneous_voltages();
void print_samples();

MAX78615_LMU m;

void setup()
{
  Serial.begin(9600);
  m.write_register(VFSCALE, 667);
  //Normal Operation
  m.write_register(COMMAND, 0x000030);
}

void loop()
{ 
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

  delay(1000);
}

