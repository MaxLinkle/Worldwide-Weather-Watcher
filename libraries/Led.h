#include<Arduino.h>

void sendByte(byte b){
    // Send one bit at a time, starting with the MSB
    for (byte i=0; i<8; i++)
    {
      pinMode(6,OUTPUT);
      pinMode(5,OUTPUT);
        // If MSB is 1, write one and clock it, else write 0 and clock
        if ((b & 0x80) != 0)
            digitalWrite(6, HIGH);
        else
            digitalWrite(6, LOW);

    digitalWrite(5, LOW);
    delayMicroseconds(20);
    digitalWrite(5, HIGH);
    delayMicroseconds(20);

        // Advance to the next bit to send
        b <<= 1;
    }
}

void setColorRGB(byte red, byte green, byte blue){
    // Send data frame prefix (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);

        // Start by sending a byte with the format "1 1 /B7 /B6 /G7 /G6 /R7 /R6"
    byte prefix = 0b11000000;
    if ((blue & 0x80) == 0)     prefix|= 0b00100000;
    if ((blue & 0x40) == 0)     prefix|= 0b00010000;
    if ((green & 0x80) == 0)    prefix|= 0b00001000;
    if ((green & 0x40) == 0)    prefix|= 0b00000100;
    if ((red & 0x80) == 0)      prefix|= 0b00000010;
    if ((red & 0x40) == 0)      prefix|= 0b00000001;
    sendByte(prefix);

    // Now must send the 3 colors
    sendByte(blue);
    sendByte(green);
    sendByte(red);

    // Terminate data frame (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
}