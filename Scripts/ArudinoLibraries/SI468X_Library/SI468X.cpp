#include "SI468X.h"



#include <util/delay.h>
#include <stdlib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SPI.h>


SI468x_Radio::SI468x_Radio(int8_t RESET, int8_t INT){
    IntPin = INT;
    reset = RESET;

    //define pin modes
    pinMode(IntPin, OUTPUT);
    pinMode(reset, OUTPUT);
    Address=B1100100;
    flashLoad=true;
    amLocation=0;
    fmLocation=1;
    spiEnable=false;
}

void SI468x_Radio::set_i2c_mode(){
    spiEnable=false;
}

void SI468x_Radio::set_spi_mode(int8_t MOSI, int8_t MISO, int8_t CLK, int8_t SSB){
    spiEnable=true;
    mosi=MOSI;
    miso=MISO;
    clk=CLK;
    ssb=SSB;
    pinMode(ssb, OUTPUT);
    pinMode(mosi, OUTPUT);
    pinMode(miso, INPUT);
    pinMode(clk, OUTPUT);
    digitalWrite(ssb,HIGH);
}

void SI468x_Radio::set_address(byte a){
    Address=a;
}
void SI468x_Radio::set_flash_load(bool load){
    flashLoad=load;
}



////////////////////////////////   General Read/Write Commands ///////////////////////////////////////////////


byte *SI468x_Radio::spi_transfer(byte *data,uint16_t len){
    byte *resp;
    for(int i=0;i<len;i++){
        resp[i]=SPI.transfer(data[i]);
    }
    return resp;
}



void SI468x_Radio::read(byte *data, byte cnt)
{
    byte zero = 0;
    uint8_t timeout;

    timeout = 100; // wait for CTS
    #if !RADIO_DEBUG
    while(--timeout){
            Serial.println("Requesting Status");
            zero = 0;
            if(spiEnable==false){
                //request current status
                Wire.beginTransmission(Address);
                Wire.write(&zero,1);
                Wire.endTransmission();

                //get back status
                Wire.beginTransmission(Address);
                Wire.requestFrom(Address,cnt);
                Wire.readBytes(data,cnt);
                Wire.endTransmission();
            }else{
                digitalWrite(ssb,HIGH);
                delay(1);
                digitalWrite(ssb,LOW);
                spi_transfer(&zero,1);
                data=spi_transfer(&zero,cnt);
                digitalWrite(ssb,HIGH);
            }
            if(data[0] & 0x80)
                    break;
    }
    if(timeout==0){
       Serial.println("timeout");
    }
    Serial.print("Response:");
    for(int i=0;i<cnt;i++){
        Serial.print(data[i],BIN);
    }
    Serial.println();
#else
    data[0]=0x80;
#endif

}


uint16_t SI468x_Radio::read_dynamic(byte *data)
{
    byte zero = 0;
    byte *data2;
    uint16_t cnt;

#if !RADIO_DEBUG
    if(spiEnable==false){
        //request current status
        Wire.beginTransmission(Address);
        Wire.write(&zero,1);
        Wire.endTransmission();

        //request initial header
        Wire.beginTransmission(Address);
        Wire.requestFrom(Address,6);
        Wire.readBytes(data,6);
    }else{
        digitalWrite(ssb,HIGH);
        delay(1);
        digitalWrite(ssb,LOW);
        spi_transfer(&zero,1);
        data=spi_transfer(&zero,6);
        digitalWrite(ssb,HIGH);
    }
    cnt = ((uint16_t)data[5]<<8) | (uint16_t)data[4];
    if(cnt > 3000) cnt = 0;

    if(spiEnable==false){
        //request extra bytes
        Wire.requestFrom(Address,cnt);
        Wire.readBytes(data,cnt);
        Wire.endTransmission();
    }else{
        digitalWrite(ssb,HIGH);
        delay(1);
        digitalWrite(ssb,LOW);
        data2=spi_transfer(&zero,cnt);
        digitalWrite(ssb,HIGH);
        for(int i=0;i<sizeof(data2);i++){
            data[6+i]=data2[i];
        }
    }

    Serial.print("Received:");
    for(int i=0;i<cnt;i++){
        Serial.print(data[i],BIN);
    }
    Serial.println();
#else
    data[0]=0x80;
    cnt=0;
#endif

    return cnt + 6;
}

void SI468x_Radio::write(byte cmd, byte *data,uint16_t len){

    uint8_t timeout;
    byte buf[4];


    timeout = 100; // wait for CTS
    while(--timeout){
        this->read(buf,4);
        if(buf[0] & 0x80)
                break;
    }
    Serial.print("Sending ");

#if !RADIO_DEBUG
    if(spiEnable==false){

        Wire.beginTransmission(Address);
        Wire.write(&cmd,1);
        Wire.write(data,len);
        Wire.endTransmission();
    }else{
        digitalWrite(ssb,HIGH);
        delay(1);
        digitalWrite(ssb,LOW);
        spi_transfer(&cmd,1);
        spi_transfer(data,len);
        digitalWrite(ssb,HIGH);
    }
    Serial.print(cmd,HEX);
    for(int i=0;i<len;i++){
        Serial.print(data[i],HEX);
    }
    Serial.println();

#else
/*    Serial.print(cmd,HEX);
    for(int i=0;i<len;i++){
        Serial.print(data[i],HEX);
    }
    Serial.println();*/
#endif
}
void SI468x_Radio::write_data(byte cmd, byte *data,uint16_t len){
    this->write(cmd,data,len);
}



///////////////////////////System Control////////////////////////////////////////////////

void SI468x_Radio::set_property(uint16_t property_id, uint16_t value){
    byte data[5];
    byte buf[4];

    printf("si46xx_set_property(0x%02X,0x%02X)\r\n",property_id,value);

    data[0] = 0;
    data[1] = property_id & 0xFF;
    data[2] = (property_id >> 8) & 0xFF;
    data[3] = value & 0xFF;
    data[4] = (value >> 8) & 0xFF;
    this->write(SI46XX_SET_PROPERTY,data,5);
    this->read(buf,4); //read the response
}

byte *SI468x_Radio::get_sys_state()
{
    byte zero = 0;
    byte buf[6];

    this->write(SI46XX_GET_SYS_STATE,&zero,1);
    this->read(buf,6);
    mode = buf[4];
    return buf;
    /*
    switch(mode)
    {
            case 0: printf("Bootloader is active\n"); break;
            case 1: printf("FMHD is active\n"); break;
            case 2: printf("DAB is active\n"); break;
            case 3: printf("TDMB or data only DAB image is active\n"); break;
            case 4: printf("FMHD is active\n"); break;
            case 5: printf("AMHD is active\n"); break;
            case 6: printf("AMHD Demod is active\n"); break;
            default: break;
    }*/
}

byte *SI468x_Radio::get_part_info()
{
    uint8_t zero = 0;
    byte buf[22];

    this->write(SI46XX_GET_PART_INFO,&zero,1);
    this->read(buf,22);
    chipInfo=*buf;
    return buf;
}



////////////////////////////////System Initilization///////////////////////////////////////////////


void SI468x_Radio::load_init()
{
    uint8_t data = 0;
    this->write(SI46XX_LOAD_INIT,&data,1);
    delay(4); // wait 4ms (datasheet)
}

void SI468x_Radio::store_image(const byte *data, uint32_t len, uint8_t wait_for_int)
{
    uint32_t remaining_bytes = len;
    uint32_t count_to;
    byte buf[4];

    this->load_init();
    while(remaining_bytes){
            if(remaining_bytes >= 2048){
                    count_to = 2048;
            }else{
                    count_to = remaining_bytes;
            }

            this->write_host_load_data(SI46XX_HOST_LOAD, data+(len-remaining_bytes), count_to);
            remaining_bytes -= count_to;
            delay(1);
    }
    delay(4); // wait 4ms (datasheet)
    this->read(buf,4);
    delay(4); // wait 4ms (datasheet)
}




void SI468x_Radio::write_host_load_data(byte cmd,
                const byte *data,
                uint16_t len)
{

    uint8_t zero_data[3];

    zero_data[0] = 0;
    zero_data[1] = 0;
    zero_data[2] = 0;
#if !RADIO_DEBUG
    if(spiEnable==false){
        Wire.beginTransmission(Address);
        Wire.write(&cmd,1);
        Wire.write(zero_data,3);
        Wire.write((byte*)data,len);
        Wire.endTransmission();
    }else{
        digitalWrite(ssb,HIGH);
        delay(1);
        digitalWrite(ssb,LOW);
        spi_transfer(&cmd,1);
        spi_transfer(zero_data,3);
        spi_transfer((byte*)data,len);
        digitalWrite(ssb,HIGH);
    }

#else
    Serial.write(&cmd,1);
    Serial.write(zero_data,3);
    Serial.write((byte*)data,len);
#endif
}

void SI468x_Radio::powerup()
{
    uint8_t data[15];
    byte buf[4];

    data[0] = 0x80; // ARG1
    data[1] = (1<<4) | (7<<0); // ARG2 CLK_MODE=0x1 TR_SIZE=0x7
    //data[2] = 0x28; // ARG3 IBIAS=0x28
    data[2] = 0x48; // ARG3 IBIAS=0x48
    data[3] = 0x00; // ARG4 XTAL
    data[4] = 0xF9; // ARG5 XTAL // F8
    data[5] = 0x24; // ARG6 XTAL
    data[6] = 0x01; // ARG7 XTAL 19.2MHz
    data[7] = 0x1F; // ARG8 CTUN
    data[8] = 0x00 | (1<<4); // ARG9
    data[9] = 0x00; // ARG10
    data[10] = 0x00; // ARG11
    data[11] = 0x00; // ARG12
    data[12] = 0x00; // ARG13 IBIAS_RUN
    data[13] = 0x00; // ARG14
    data[14] = 0x00; // ARG15

    this->write_data(SI46XX_POWER_UP,data,15);
    delay(1); // wait 20us after powerup (datasheet)
}

void SI468x_Radio::boot()
{
    uint8_t data = 0;
    byte buf[4];

    this->write_data(SI46XX_BOOT,&data,1);
    delay(300); // 63ms at analog fm, 198ms at DAB
    this->read(buf,4);
}

void SI468x_Radio::flash_load_image(uint32_t address){
    byte data[11];
    byte cmd=SI46XX_FLASH_LOAD;


    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = (address >> 24)& 0xff;
    data[4] = (address >> 16)& 0xff;
    data[5] = (address >> 8)& 0xff;
    data[6] = (address)& 0xff;
    data[7] = 0;
    data[8] = 0;
    data[9] = 0;
    data[10] = 0;

    this->load_init();

    this->write(cmd,data,11);
}
void SI468x_Radio::resetChip(){
    Serial.println("Resetting si46xx");
    digitalWrite(reset,LOW);
    delay(10);
    digitalWrite(reset,HIGH);
    delay(10);
    if(spiEnable==true){
        digitalWrite(ssb,HIGH);
    }
}

void SI468x_Radio::init_firmware(uint8_t location,uint8_t patching)
{
    byte miniPatch[1024];
    byte buf[4];

    Serial.println("Reading Mini Patch");
    int size=eepromReadInt(0);
    if(size>0){
      int c=0;
      Serial.println("Reading from eeprom");
      for(int i=c; i<=size; i++){ //retrieve minipatch from EEPROM
        miniPatch[i]=EEPROM.read(2+i);
        c++;
      }
      for(int q=c; q<=1024; q++){ //Pad end of patch
        miniPatch[q]=0;
      }
    }


    //printf("si46xx_init_mode_fm()\r\n");
    /* reset si46xx  */
    Serial.println("Resetting si46xx");
    digitalWrite(reset,LOW);
    delay(10);
    digitalWrite(reset,HIGH);
    delay(10);
    Serial.println("Starting Powerup");
    this->powerup();


    Serial.println("Applying Minipatch");
    //apply minipatch
    this->load_init();
    this->write_host_load_data(SI46XX_HOST_LOAD, miniPatch, 1024); //load the minipatch into memory first
    delay(4); // wait 4ms (datasheet)
    this->load_init();

    if(patching == 1)   // we are attempting to update the flash rom, return here
        return;

    Serial.println("Loading Fullpatch");
    this->flash_load_image(0x00002000+(0x00002000*location));
    this->load_init();
    Serial.println("Loading Firmware");
    this->flash_load_image(0x00008000+(0x00086000*location));


    this->boot();
    this->get_sys_state();
    this->get_part_info();
}

void SI468x_Radio::init_fm()
{
    if(flashLoad){
        this->init_firmware(fmLocation);
    }else{
        //in future will make function to load from serial or something
    }
}
void SI468x_Radio::init_am()
{
    if(flashLoad){
        this->init_firmware(amLocation);
    }else{
        //in future will make function to load from serial or something
    }
}



////////////////////////////////FM FUNCTIONS//////////////////////////

byte *SI468x_Radio::fm_rsq_status()
{
        byte data = 0;
        byte buf[20];

        this->write(SI46XX_FM_RSQ_STATUS,&data,1);
        this->read(buf,20);
        return buf;
}

byte *SI468x_Radio::fm_rds_blockcount()
{
        //uint8_t data = 1; // clears block counts if set
        byte data = 0; // clears block counts if set
        byte  buf[10];

        this->write_data(SI46XX_FM_RDS_BLOCKCOUNT,&data,1);
        this->read(buf,10);
        return buf;
}
fm_rds_data_t SI468x_Radio::get_rds_data(){
    return fm_rds_data;
}

uint8_t SI468x_Radio::rds_parse(uint16_t *block)
{
        byte addr;
        fm_rds_data.pi = block[0];
        if((block[1] & 0xF800) == 0x00){ // group 0A
                addr = block[1] & 0x03;
                fm_rds_data.ps_name[addr*2] = (block[3] & 0xFF00)>>8;
                fm_rds_data.ps_name[addr*2+1] = block[3] & 0xFF;
                fm_rds_data.group_0a_flags |= (1<<addr);
        }else if((block[1] & 0xF800)>>11 == 0x04){ // group 2A
                addr = block[1] & 0x0F;
                if((block[1] & 0x10) == 0x00){ // parse only string A
                        fm_rds_data.radiotext[addr*4] = (block[2] & 0xFF00)>>8;
                        fm_rds_data.radiotext[addr*4+1] = (block[2] & 0xFF);
                        fm_rds_data.radiotext[addr*4+2] = (block[3] & 0xFF00)>>8;
                        fm_rds_data.radiotext[addr*4+3] = (block[3] & 0xFF);

                        if(fm_rds_data.radiotext[addr*4] == '\r'){
                                fm_rds_data.radiotext[addr*4] = 0;
                                fm_rds_data.group_2a_flags = 0xFFFF;
                        }
                        if(fm_rds_data.radiotext[addr*4+1] == '\r'){
                                fm_rds_data.radiotext[addr*4+1] = 0;
                                fm_rds_data.group_2a_flags = 0xFFFF;
                        }
                        if(fm_rds_data.radiotext[addr*4+2] == '\r'){
                                fm_rds_data.radiotext[addr*4+2] = 0;
                                fm_rds_data.group_2a_flags = 0xFFFF;
                        }
                        if(fm_rds_data.radiotext[addr*4+3] == '\r'){
                                fm_rds_data.radiotext[addr*4+3] = 0;
                                fm_rds_data.group_2a_flags = 0xFFFF;
                        }
                        fm_rds_data.group_2a_flags |= (1<<addr);
                }
        }
        if(fm_rds_data.group_0a_flags == 0x0F &&
                        fm_rds_data.group_2a_flags == 0xFFFF){
                fm_rds_data.ps_name[8] = 0;
                fm_rds_data.radiotext[128] = 0;
                return 1;
        }
        return 0;
}


byte *SI468x_Radio::fm_rds_status()
{
        byte data = 0;
        byte buf[20];
        uint16_t timeout;
        uint16_t blocks[4];

        timeout = 5000; // work on 1000 rds blocks max
        while(--timeout){
                data = 1;
                this->write(SI46XX_FM_RDS_STATUS,&data,1);
                this->read(buf,20);
                blocks[0] = buf[12] + (buf[13]<<8);
                blocks[1] = buf[14] + (buf[15]<<8);
                blocks[2] = buf[16] + (buf[17]<<8);
                blocks[3] = buf[18] + (buf[19]<<8);
                fm_rds_data.sync = (buf[5] & 0x02)?1:0;
                if(!fm_rds_data.sync)
                        break;
                if(this->rds_parse(blocks))
                        break;
                if(fm_rds_data.group_0a_flags == 0x0F) // stop at ps_name complete
                        break;
        }
        if(!timeout)
                printf("Timeout\r\n");
        return buf;
}

byte *SI468x_Radio::fm_tune_freq(uint32_t khz, uint16_t antcap)
{
        byte data[5];
        byte buf[4];


        //data[0] = (1<<4) | (1<<0); // force_wb, low side injection
        //data[0] = (1<<4)| (1<<3); // force_wb, tune_mode=2
        data[0] = 0;
        data[1] = ((khz/10) & 0xFF);
        data[2] = ((khz/10) >> 8) & 0xFF;
        data[3] = antcap & 0xFF;
        data[4] = 0;
        this->write_data(SI46XX_FM_TUNE_FREQ,data,5);

        this->read(buf,4);
        return buf;
}

byte *SI468x_Radio::fm_seek_start(uint8_t up, uint8_t wrap)
{
        byte data[5];
        byte buf[4];

        data[0] = 0;
        data[1] = (up&0x01)<<1 | (wrap&0x01);
        data[2] = 0;
        data[3] = 0;
        data[4] = 0;
        this->write(SI46XX_FM_SEEK_START,data,5);

        this->read(buf,4);
        return buf;
}



//////////////////////////FLASH COMMANDS//////////////////////////////////////
byte *SI468x_Radio::flash_erase_sector(uint32_t address)
{
        byte data[5];
        byte buf[4];

        data[0] = 0xFE;
        data[1] = 0xC0;
        data[2] = 0xDE;
        data[3] = (address >> 24)& 0xff;
        data[4] = (address >> 16)& 0xff;
        data[5] = (address >> 8)& 0xff;
        data[6] = (address)& 0xff;
        this->write(SI46XX_FLASH_LOAD,data,6);

        this->read(buf,4);
        return buf;
}
byte *SI468x_Radio::flash_erase_chip()
{
        byte data[5];
        byte buf[4];

        data[0] = 0xFF;
        data[1] = 0xDE;
        data[2] = 0xC0;
        this->write(SI46XX_FLASH_LOAD,data,6);

        this->read(buf,4);
        return buf;
}
byte *SI468x_Radio::flash_write_block(uint32_t address, uint32_t size, const byte *data, uint32_t len)
{
        byte arguments[15];
        byte buf[4];
        byte cmd=SI46XX_FLASH_LOAD;

        arguments[0] = 0xF0;
        arguments[1] = 0x0C;
        arguments[2] = 0xED;
        arguments[3] = 0;
        arguments[4] = 0;
        arguments[5] = 0;
        arguments[6] = 0;
        arguments[7] = (address >> 24)& 0xff;
        arguments[8] = (address >> 16)& 0xff;
        arguments[9] = (address >> 8)& 0xff;
        arguments[10] = (address)& 0xff;
        arguments[11] = (size >> 24)& 0xff;
        arguments[12] = (size >> 16)& 0xff;
        arguments[13] = (size >> 8)& 0xff;
        arguments[14] = (size)& 0xff;

        if(spiEnable==false){
            Wire.beginTransmission(Address);
            Wire.write(&cmd,1);
            Wire.write(arguments,15);
            Wire.write((byte*)data,len);
            Wire.endTransmission();
        }else{
            spi_transfer(&cmd,1);
            spi_transfer(arguments,15);
            spi_transfer((byte*)data,len);
        }

        this->read(buf,4);
        return buf;
}
byte *SI468x_Radio::flash_set_properties(uint16_t write, uint16_t read, uint16_t hs_read, uint16_t erase_sector, uint16_t erase_chip)
{
        byte arguments[15];
        byte buf[4];

        arguments[0] = 0x10;
        arguments[1] = 0x00;
        arguments[2] = 0x00;
        arguments[3] = 0x02;
        arguments[4] = 0x01;
        arguments[5] = (write >> 8)& 0xff;
        arguments[6] = (write)& 0xff;
        arguments[7] = 0x01;
        arguments[8] = 0x01;
        arguments[9] = (read >> 8)& 0xff;
        arguments[10] = (read)& 0xff;
        arguments[11] = 0x01;
        arguments[12] = 0x02;
        arguments[13] = (hs_read >> 8)& 0xff;
        arguments[14] = (hs_read)& 0xff;
        arguments[15] = 0x02;
        arguments[16] = 0x02;
        arguments[17] = (erase_sector >> 8)& 0xff;
        arguments[18] = (erase_sector)& 0xff;
        arguments[19] = 0x02;
        arguments[20] = 0x04;
        arguments[21] = (erase_chip >> 8)& 0xff;
        arguments[22] = (erase_chip)& 0xff;

        this->write(SI46XX_FLASH_LOAD,arguments,23);

        this->read(buf,4);
        return buf;
}


////OTHER THINGS/////
int SI468x_Radio::eepromReadInt(int address){
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}

void SI468x_Radio::eepromWriteInt(int address, int value){
   EEPROM.write(address, (value >> 8) & 0xFF );
   EEPROM.write(address+1, value & 0xFF);
}


