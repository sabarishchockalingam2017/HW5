#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
// I2C Master utilities, 100 kHz, using polling rather than interrupts
// The functions must be callled in the correct order as per the I2C protocol
// Change I2C1 to the I2C channel you are using
// I2C pins need pull-up resistors, 2k-10k
//#define SL_WR 0b01000000
//#define SL_RD 0b01000001
#include "i2c_master_noint.h"

void i2c_master_setup(void) {
  I2C2BRG = 233;            // I2CBRG = [1/(2*Fsck) - PGD]*Pblck - 2 
                                    // look up PGD for your PIC32
                                    //  PGD Typically value is 104 ns for PIC32MX250F128B
  I2C2CONbits.ON = 1;               // turn on the I2C2 module
}

// Start a transmission on the I2C bus
void i2c_master_start(void) {
    I2C2CONbits.SEN = 1;            // send the start bit
    while(I2C2CONbits.SEN) { ; }    // wait for the start bit to be sent,
    //hardware turns SEN to 0 automatically after start bit is sent
}

void i2c_master_restart(void) {     
    I2C2CONbits.RSEN = 1;           // send a restart 
    while(I2C2CONbits.RSEN) { ; }   // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
  I2C2TRN = byte;                   // if an address, bit 0 = 0 for write, 1 for read
  while(I2C2STATbits.TRSTAT) { ; }  // wait for the transmission to finish
  if(I2C2STATbits.ACKSTAT) {        // if this is high, slave has not acknowledged
    // ("I2C2 Master: failed to receive ACK\r\n");
  }
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C2CONbits.RCEN = 1;             // start receiving data
    while(!I2C2STATbits.RBF) { ; }    // wait to receive the data
    return I2C2RCV;                   // read and return the data
}

void i2c_master_ack(int val) {        // sends ACK = 0 (slave should send another byte)
                                      // or NACK = 1 (no more bytes requested from slave)
    I2C2CONbits.ACKDT = val;          // store ACK/NACK in ACKDT
    I2C2CONbits.ACKEN = 1;            // send ACKDT
    while(I2C2CONbits.ACKEN) { ; }    // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) {          // send a STOP:
  I2C2CONbits.PEN = 1;                // comm is complete and master relinquishes bus
  while(I2C2CONbits.PEN) { ; }        // wait for STOP to complete
}

void initExpander(){
   ANSELBbits.ANSB2=0; // Turning off default ANSEL function for B2 pin
   ANSELBbits.ANSB3=0; // Turning off default ANSEL function for B3 pin
   I2C2BRG = 233;            // I2CBRG = [1/(2*Fsck) - PGD]*Pblck - 2 
                                    // look up PGD for your PIC32
                                    //  PGD Typically value is 104 ns for PIC32MX250F128B
  I2C2CONbits.ON = 1;               // turn on the I2C2 module
  
    i2c_master_start(); //sending start bit
    i2c_master_send(SL_WR); //sending Write Control bit, last bit 0 as this is a write
    i2c_master_send(0x00); //sending address of IODIR register
    i2c_master_send(0b11111110); //sending data to make GP7 an input and GP0 an out, all others are input
    i2c_master_stop(); // sending stop bit, releases control of bus
}

void setExpander (char addr, char data){
    i2c_master_start(); //sending start bit
    i2c_master_send(SL_WR); //sending Write Control bit, last bit 0 as this is a write
    i2c_master_send(addr); //sending address of register
    i2c_master_send(data); //sending data to write to register
    i2c_master_stop(); // sending stop bit, releases control of bus
}

char getExpander(char pin){
    char rec;
    char out;
    i2c_master_start(); //sending start bit, this time to write register to read
    i2c_master_send(SL_WR); //sending Write Control bit, last bit 0 as this is a write
    i2c_master_send(0x09); //sending address of PORT register
    i2c_master_restart(); //restarting transfer this time to read
    i2c_master_send(SL_RD); //sending Read Control bit, last bit 1 as this is a read
    rec=i2c_master_recv(); //receiving data from register
    i2c_master_ack(1); // sending NACK to signal end of requests
    i2c_master_stop(); // sending stop bit, releases control of bus
    out=(rec>>pin)&(0b1);
    return out;
}