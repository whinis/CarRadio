
#include <Wire.h>
#include <SPI.h>

#include <SI468X.h>
#include <EEPROM.h>

SI468x_Radio Radio=SI468x_Radio(6,7);
void setup() {
  byte *buf;
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
 Serial.println("Starting INIT FM");
  Radio.init_fm();
  delay(10);
  Radio.flash_erase_chip();
}

void loop() {
}

void fm_rsq_status(){
  byte *buf;
  buf=Radio.fm_rsq_status();
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
  buf=Radio.fm_rds_blockcount();
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
  buf=Radio.fm_rds_status();
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

