
#include <Wire.h>
#include <SPI.h>

#include <SI468X.h>
#include <EEPROM.h>

SI468x_Radio Radio=SI468x_Radio(6,7);
void setup() {
  byte *buf;
  byte *fullPatch;
  int bytes = 4096;
  
  pinMode(2, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(2,LOW);
  digitalWrite(5,LOW);
  Wire.begin();
  // put your setup code here, to run once:
  Serial.begin(57600);
  Radio.set_address(0x64);
  Radio.resetChip();
  Radio.read(buf,4);
  Serial.println("Starting firmware load");
  Radio.init_firmware(0,1);
  delay(10);
  Serial.println("-requesting_patch");
  while( bytes == 4096){
    bytes = stream.readBytes(fullPatch, 4096);
    //Radio.write_host_load_data(SI46XX_HOST_LOAD, buf, 1024); //load the minipatch into memory first
    if(bytes == 4096){  
      Serial.print("a");
    }
    else
    {
      Serial.print("d");
    }
  }
  Serial.println("-requesting_patch");
  uint32_t address = 0x00002000;
  while( bytes == 4096){
    bytes = stream.readBytes(fullPatch, 4096);
    //Radio.flash_write_block(address,bytes,fullPatch,bytes);
    address = address + bytes;
    if(bytes == 4096){  
      Serial.print("a");
    }
    else
    {
      Serial.print("d");
    }
  }

}

void loop() {
  
}
