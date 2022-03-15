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
unsigned char buf[8];

/*CAN message IDs*/
unsigned int leakDetected = 0x02;           //CAN MSG OUT: ID
//unsigned char leakSuddenReset = 0x12;     //!!!!!!data out, is this independent of 0x13, how does this work in general?
unsigned int leakStatusResp = 0x202;        //CAN MSG OUT: ID, DATA
unsigned int leakHBOut = 0x402;             //CAN MSG OUT: ID
//To Leak Sensor
unsigned int leakReset = 0x13;              //CAN MSG IN: ID
unsigned int leakStatusReq = 0x203;         //CAN MSG IN: ID
unsigned int leakHBRequest = 0x403;         //CAN MSG IN: ID, DATA?

unsigned int msgid = 0x13;                  //switch statement case

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
    CAN.init_Mask(0, 0, 0x7ff);     //Mask 1 for ID                         
    CAN.init_Mask(1, 0, 0x7ff);     //Mask 2 for ID
    CAN.init_Filt(0, 0, 0x13);      //Allow ID 0x04
    CAN.init_Filt(1, 0, 0x05);      //Allow ID 0x05
    CAN.init_Filt(2, 0, 0x06);      //Allow ID 0x06
    CAN.init_Filt(3, 0, 0x07);      //Allow ID 0x07
    CAN.init_Filt(4, 0, 0x08);      //Allow ID 0x08
    CAN.init_Filt(5, 0, 0x09);      //Allow ID 0x09
}

void loop()
{
    recieveData = requestSensor(0x14, 1);
    if(recieveData >= 0x05){
        //send leakDetected CAN BUS MSG
        /*
            CAN.sendMsgBuf(INT8U id, INT8U ext, INT8U len, data_buf);
            CAN.sendMsgBuf(leakDetected, 0, 1, 0);
            OR
            CAN.sendMsgBuf(0x00, 0, 1, leakDetected);
        */
    }

    if(flagRecv){
        flagRecv = 0;
        CAN.readMsgBuf(&len, buf);
        //parse the buf data and mask what data I want
        
        //clarification on case format
        //add critical check on all I2C pulls
        switch (msgid) { 
            case 1:  //'0x13' reset leak sensor (Master data -> sensor)
                //sendData = some sorta parsing of the buffer data or if trent already has a code
                Wire.beginTransmission(0x14);           //sensor I2C address 0x14
                Wire.write(sendData);
                Wire.endTransmission();
                break;
            case 2: //'0x203' send status data (Sensor data -> Master)
                //take data from sensor
                recieveData = requestSensor(0x14, 1);
                //send data out
                //ID: 0x202
                CAN.sendMsgBuf(0x402, 0, INT8U len, data_buf);
                break;
            case 3: //'0x403' recieve HB data and send out sensor HB data
                //sendData = HeartBeat data from CAN MSG
                Wire.beginTransmission(0x14);           //sensor I2C address 0x14
                Wire.write(sendData);
                Wire.endTransmission();
                recieveData = requestSensor(0x14, 1);   //recieve data from heartbeat
                if(recieveData < 0x04){ //sensor data (idk if it'll be 0x04)
                    //ID: 0x402
                    //CAN.sendMsgBuf(0x402, 0, INT8U len, data_buf); healthy code
                } else {
                    //CAN.sendMsgBuf(0x402, 0, INT8U len, data_buf); not healthy code
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
byte requestSensor(int address, int length){
    Wire.requestFrom(address, length); //address, length
    if(Wire.available()){
        byte rD = Wire.read();
    }   
    return rD;
}
// recieveData = requestSensor(address, length);