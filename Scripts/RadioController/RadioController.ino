
#include <Wire.h>
#include <SPI.h>

#include <SI468X.h>
#include <EEPROM.h>

SI468x_Radio Radio=SI468x_Radio(10,7);
void setup() {
  byte *buf;
  byte miniPatch[1024];
  memset(miniPatch,0,1024);
  Wire.begin();
  // put your setup code here, to run once:
  Serial.begin(250000);
  Radio.set_address(0x64);


  /* reset si46xx  */
  Serial.println("Resetting si46xx");
  Radio.resetChip();
  delay(5);
  Serial.println("Starting Powerup");
  Radio.powerup();
  delay(1);
  int size =0;
  Serial.println("Reading Mini Patch");
  size=eepromReadInt(0);
  Serial.print("Size: ");
  Serial.println(size);
  if(size>0){
    size = size;
    Serial.println("Reading from eeprom");
    for(int i=2; i<=(size); i++){ //retrieve minipatch from EEPROM
       miniPatch[i-2]=EEPROM.read(i);
    }
  }
  Radio.load_init(buf);
  Radio.write_host_load_data(SI46XX_HOST_LOAD, miniPatch, 1024,buf); //load the minipatch into memory first
  delay(4); // wait 4ms (datasheet)
  Radio.load_init(buf);
  Radio.flash_load_image(0x00004000,buf);
  Radio.load_init(buf);
  Radio.flash_load_image(0x000096A4,buf);
  Radio.boot();
  Radio.read(buf,4);
  Radio.fm_tune_freq(102500,0);
}

void loop() {
  /*
  byte *buf;
  delay(2000);
  Radio.read(buf,4);
  printResponse(buf,4);
  */
}

void fm_rsq_status(){
  byte *buf;
  Radio.fm_rsq_status(buf);
  char buffer[50];
  sprintf(buffer,"SNR: %d dB\r\n",(int8_t)buf[10]);
  Serial.println(buffer);
  sprintf(buffer,"RSSI: %d dBuV\r\n",(int8_t)buf[9]);
  Serial.println(buffer);
  sprintf(buffer,"Frequency: %dkHz\r\n",(buf[7]<<8 | buf[6])*10);
  Serial.println(buffer);
  sprintf(buffer,"FREQOFF: %d\r\n",(int8_t)buf[8]*2);
  Serial.println(buffer);
  sprintf(buffer,"READANTCAP: %d\r\n",(int8_t)(buf[12]+(buf[13]<<8)));
  Serial.println(buffer);
}
void fm_rds_blockcount(){
  byte  *buf;
  Radio.fm_rds_blockcount(buf);
  char buffer[50];
  sprintf(buffer,"Expected: %d\r\n",buf[4] | (buf[5]<<8));
  Serial.println(buffer);
  sprintf(buffer,"Received: %d\r\n",buf[6] | (buf[7]<<8));
  Serial.println(buffer);
  sprintf(buffer,"Uncorrectable: %d\r\n",buf[8] | (buf[9]<<8));
  Serial.println(buffer);
}
void fm_rds_status(){
  byte  *buf;
  fm_rds_data_t fm_rds_data;
  Radio.fm_rds_status(buf);
  fm_rds_data=Radio.get_rds_data();
  char buffer[50];
  sprintf(buffer,"RDSSYNC: %u\r\n",(buf[5]&0x02)?1:0);
  Serial.println(buffer);

  sprintf(buffer,"PI: %d  Name:%s\r\nRadiotext: %s\r\n",
                  fm_rds_data.pi,
                  fm_rds_data.ps_name,
                  fm_rds_data.radiotext);
  Serial.println(buffer);
}

void printResponse(byte *buf,int s){
  Serial.println("Response:");
  for(int i=0;i<s;i++){
      Serial.print(i);
      Serial.print(": ");
        Radio.printBits(buf[i]);

      Serial.println();
  }
}

int eepromReadInt(int address){
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}
int checkError(byte *buf){
  if(buf[0] & 0x40){
   return 0;  
  }
  printResponse(buf,4);
  return 1;
}
void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

void eepromWriteInt(int address, int value){
   EEPROM.write(address, (value >> 8) & 0xFF );
   EEPROM.write(address+1, value & 0xFF);
}

