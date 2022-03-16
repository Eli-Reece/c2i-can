    /*
        1. Receive data from I2C and store into recieveData check if CRITICAL
            - if critical send CAN msg for CRITICAL 
        2. Mask and Filter CAN
        3. Parse filtered CAN message -> determine what action to take (case statement? prolly just if else)
            - send sensor data to master
            - send master data to sensor
            - etc
        4. 
        5. Receive data from I2C and store into recieveData check if CRITICAL
        crit water level > 0x05

    */
#include <SPI.h>
#include <Wire.h>

const int SPI_CS_PIN = 9;   //13 on ATMega, 16 on 2515
const int CAN_INT_PIN = 2;  //32 on ATMega, 12 on 2515

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN);

/*I2C global */
byte recieveData;                   //i2c data to be received from Target
byte rD;
int sendData = 0;                   //i2c data sent to Target

/*CAN global */
unsigned char flagRecv = 0;         //interrupt flag
unsigned char len = 0;
unsigned char emptybuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char buf[8];

/*CAN message IDs*/
unsigned int leakDetected = 0x002;           //CAN MSG OUT: ID
//unsigned char leakSuddenReset = 0x12;     //!!!!!!data out, is this independent of 0x13, how does this work in general?
unsigned int leakStatusResp = 0x202;        //CAN MSG OUT: ID, DATA
unsigned int leakHBOut = 0x402;             //CAN MSG OUT: ID
//To Leak Sensor
unsigned int leakReset = 0x013;              //CAN MSG IN: ID
unsigned int leakStatusReq = 0x203;         //CAN MSG IN: ID
unsigned int leakHBRequest = 0x403;         //CAN MSG IN: ID, DATA?

unsigned long msgid = 0x13;                  //switch statement case

void setup()
{
    /* I2C setup */  
    Wire.begin();

    /*CAN setup*/
    attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MCP2515_ISR, FALLING);
    while (CAN_OK != CAN.begin(CAN_500KBPS)) {      //add 16 MHz?
        delay(100);
    }

    //Fix the filters
    CAN.init_Mask(0, 0, 0x7ff);      //Mask 1 for ID
    CAN.init_Filt(0, 0, 0x013);      //Allow ID 0x013, leakReset
    CAN.init_Filt(1, 0, 0x203);      //Allow ID 0x203, leakStatusReq

    CAN.init_Mask(1, 0, 0x7ff);      //Mask 2 for ID
    CAN.init_Filt(2, 0, 0x013);      //Allow ID 0x013, leakReset
    CAN.init_Filt(3, 0, 0x203);      //Allow ID 0x203, leakStatusReq
    CAN.init_Filt(4, 0, 0x403);      //Allow ID 0x403, leakHBReq
    CAN.init_Filt(5, 0, 0x403);      //Allow ID 0x403, leakHBReq

    
}
/*
1. should data be parsed on trent's end or mine
2. make requestSensor into a try catch
3. find out what kind of data trent wants (extension of part 1)
*/
void loop()
{
    //critical leak test
    recieveData = requestSensor(0x14, 1);
    if(recieveData == 1){   //if leak data was 1
            CAN.sendMsgBuf(leakDetected, 0, 1, emptybuf);
    }
    //incoming CAN message parsing
    if(flagRecv){
        flagRecv = 0;
        CAN.readMsgBuf(&len, buf);
        //parse the buf data and mask what data I want
        msgid = CAN.getCanId();
        //clarification on case format
        //add critical check on all I2C pulls
        switch (msgid) { 
            case 0x13:                                  //'0x13' reset leak sensor (Master data -> sensor)
                sendData = 20;                          //if trent took 20 as a code to reset
                Wire.beginTransmission(0x14);           //sensor I2C address 0x14
                Wire.write(sendData);
                Wire.endTransmission();
                break;
            case 0x203:                                 //'0x203' send status data (Sensor data -> Master)
                //take data from sensor
                recieveData = requestSensor(0x14, 1);
                if(recieveData == 1){                   //if leak data was 1
                    CAN.sendMsgBuf(leakDetected, 0, 1, emptybuf);
                } else {
                    //Parse receiveData into the buf


                    //ID: 0x202
                    CAN.sendMsgBuf(0x202, 0, 8, buf);
                }
                break;
            case 0x403: //'0x403' send out sensor HB data
                recieveData = requestSensor(0x14, 1);
                if(recieveData == 1){                   //if leak data was 1
                    CAN.sendMsgBuf(leakDetected, 0, 1, emptybuf);
                } else {
                    //ID: 0x402
                    CAN.sendMsgBuf(0x402, 0, 8, emptybuf);
                }
                break;
            default:
                //sendData = some sorta parsing of the buffer data or if trent already has a code
                Wire.beginTransmission(0x14);          //sensor I2C address 0x14
                Wire.write(sendData);
                Wire.endTransmission();  
        }
    
    }
  
}


/*CAN Controller Interrupt*/
void MCP2515_ISR() {
    flagRecv = 1;
}

/* I2C get data from Sensor */
//try catch to detect sensor disconnection
// recieveData = requestSensor(address, length);
byte requestSensor(int address, int length){
    Wire.requestFrom(address, length); //address, length
    if(Wire.available()){
        byte rD = Wire.read();
    }   
    return rD;
}
