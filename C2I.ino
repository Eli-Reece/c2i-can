#include <SPI.h>
#include <Wire.h>
#include "mcp2515_can.h"

/*  Consts
    - Figure out what to do with the actual SS/CS pin since using the digital pin
    - Read that the actual cs pin needs to be set to high or something idk
*/
const int SPI_CS_PIN = 9;   //DIGITAL PIN 9 LETSGOOOO
const int CAN_INT_PIN = 2;  //DIGITAL PIN 2
mcp2515_can CAN(SPI_CS_PIN);

/*  I2C global
    - Figure out with trent what kinda of data he wants when getting a reset request
*/
byte recieveData;                   //i2c data to be received from Target
byte rD;
int sendData = 0;                   //i2c data sent to Target

/*  CAN global 
    - set interrupt flag
    - len, buf
    - initialized an empty buf just to save time 
*/
unsigned char flagRecv = 0;         //interrupt flag
unsigned char len = 0;
unsigned char emptybuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char buf[8];

/*  CAN message IDs (Leak Detection Specific)  
    IDS COMING IN:
    leakReset (0x13) -> tells sensor to reset
    leakStatusReq (0x203) -> requests current sensor data
    leakHBRequest (0x403) -> requests current connection status (if data is pulled, send 1 else 0)
    IDS GOING OUT and DATA (if it has data):
    leakDetected (0x002) -> no data
    leakStatusResp (0x202) -> current sensor data
    leakHBOut (0x402) -> 1 or 0 determining connection status or if data is still being pulled
    - leaksuddenreset kinda corresponds to the HB out
    - still need to figure out specifics with Trent and Tristan
*/
unsigned int leakDetected = 0x002;           //CAN MSG OUT: ID
//unsigned char leakSuddenReset = 0x12;     
unsigned int leakStatusResp = 0x202;        //CAN MSG OUT: ID, DATA
unsigned int leakHBOut = 0x402;             //CAN MSG OUT: ID
//To Leak Sensor
unsigned int leakReset = 0x013;              //CAN MSG IN: ID
unsigned int leakStatusReq = 0x203;         //CAN MSG IN: ID
unsigned int leakHBRequest = 0x403;         //CAN MSG IN: ID, DATA?

unsigned long msgid = 0x13;                  //switch statement case

void setup()
{
    /*  I2C setup    */  
    Wire.begin();

    /*  CAN setup   
        - shouldn't have to add the 16MHz but it might be good to make sure
    */
    attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MCP2515_ISR, FALLING);
    while (CAN_OK != CAN.begin(CAN_500KBPS)) {      //add 16 MHz?
        delay(100);
    }

    /*  CAN ID FILTERING
        MASK 0 -> set to ID
        Filters 0-1 correspond to MASK 0
        MASK 1 -> set to ID
        Filters 2-5 correspond to MASK 0
        - Read that all masks/filters need to be set but haven't tested
        - thats why there is repeat filtering
    */
    CAN.init_Mask(0, 0, 0x7ff);      //Mask 1 for ID
    CAN.init_Filt(0, 0, 0x013);      //Allow ID 0x013, leakReset
    CAN.init_Filt(1, 0, 0x203);      //Allow ID 0x203, leakStatusReq
    CAN.init_Mask(1, 0, 0x7ff);      //Mask 2 for ID
    CAN.init_Filt(2, 0, 0x013);      //Allow ID 0x013, leakReset
    CAN.init_Filt(3, 0, 0x203);      //Allow ID 0x203, leakStatusReq
    CAN.init_Filt(4, 0, 0x403);      //Allow ID 0x403, leakHBReq
    CAN.init_Filt(5, 0, 0x403);      //Allow ID 0x403, leakHBReq    
}

void loop()
{
    //critical leak test
    recieveData = requestSensor(0x14, 1);
    if(recieveData == 1){   //if leak data was 1
            CAN.sendMsgBuf(leakDetected, 0, 1, emptybuf);
    }

    if(flagRecv){
        flagRecv = 0;
        CAN.readMsgBuf(&len, buf);
        //parse the buf data and mask what data I want
        
        msgid = CAN.getCanId();

        //probably need to fix case format
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
