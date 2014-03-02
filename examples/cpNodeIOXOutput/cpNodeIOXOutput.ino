/* simple example for CMRI on cpNode & IOX hardwaee
 *
 * Copyright 2013, 2014 by david d zuhn <zoo@whitepineroute.org>
 *
 * This work is licensed under the Creative Commons Attribution-ShareAlike
 * 4.0 International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-sa/4.0/deed.en_US.
 *
 * You may use this work for any purposes, provided that you make your
 * version available to anyone else.  
 */


#include "Wire.h"
#include "MCP23017.h"
#include "IOLine.h"
#include "CMRI.h"
#include "Metro.h"


#define DEBUG 1                 // set to 1 if logging print statements are desired

// this needs to be unique on your CMRI network -- this is your C/MRI node
// ID (this was set by dip switches on older CMRI hardware such as SUSIC or
// SMINI)
#define CMRI_UA  30


// these next assignments are purely based on what you have attached
// to your hardware.  

// This is a cpNode with a single IOX32 board attached.  The IOX32
// has no address jumpers, so it uses I2C addresses 0x20 & 0x21.

IOLine *outputs[] = {
    new Pin(A5, OUTPUT),         // 0
    new Pin(A4, OUTPUT),         // 1
    new Pin(A3, OUTPUT),         // 2
    new Pin(A2, OUTPUT),         // 3
    new Pin(A1, OUTPUT),         // 4
    new Pin(A0, OUTPUT),         // 5
    new Pin(13, OUTPUT),         // 6
    new Pin(12, OUTPUT),         // 7
    new Pin(11, OUTPUT),         // 8
    new Pin(10, OUTPUT),         // 9 

    new Pin( 9, OUTPUT),         // 10
    new Pin( 8, OUTPUT),         // 11
    new Pin( 7, OUTPUT),         // 12
    new Pin( 6, OUTPUT),         // 13
    new Pin( 5, OUTPUT),         // 14
    new Pin( 4, OUTPUT),         // 15
    new IOX(0x20, 0, 7, OUTPUT), // 16   -- IOX node address 0x20, port 0, bit 7
    new IOX(0x20, 0, 6, OUTPUT), // 17
    new IOX(0x20, 0, 5, OUTPUT), // 18
    new IOX(0x20, 0, 4, OUTPUT), // 19

    new IOX(0x20, 0, 3, OUTPUT), // 20
    new IOX(0x20, 0, 2, OUTPUT), // 21
    new IOX(0x20, 0, 1, OUTPUT), // 22
    new IOX(0x20, 0, 0, OUTPUT), // 23
    new IOX(0x20, 1, 7, OUTPUT), // 24
    new IOX(0x20, 1, 6, OUTPUT), // 25
    new IOX(0x20, 1, 5, OUTPUT), // 26
    new IOX(0x20, 1, 4, OUTPUT), // 27
    new IOX(0x20, 1, 3, OUTPUT), // 28
    new IOX(0x20, 1, 2, OUTPUT), // 29

    // bits 1 & 0 of port 1 are not used in this configuration

    new IOX(0x21, 0, 7, OUTPUT), // 30   -- IOX node address 0x21, port 0, bit 7
    new IOX(0x21, 0, 6, OUTPUT), // 31
    new IOX(0x21, 0, 5, OUTPUT), // 32
    new IOX(0x21, 0, 4, OUTPUT), // 33
    new IOX(0x21, 0, 3, OUTPUT), // 34
    new IOX(0x21, 0, 2, OUTPUT), // 35
    new IOX(0x21, 0, 1, OUTPUT), // 36
    new IOX(0x21, 0, 0, OUTPUT), // 37
    new IOX(0x21, 1, 7, OUTPUT), // 38
    new IOX(0x21, 1, 6, OUTPUT), // 39

    // bits 5-0 of port 1 are not used in this configuration
};

#define outputCount NELEMENTS(outputs)


// this is a setting that depends on your Arduino hardware -- what are the
// pins for the CMRI serial line 
//   these are correct for the BBLeo based cpNode board
#define rxPin 0
#define txPin 1


CMRI cmri(Serial1, CMRI_UA);         // I/O device, my Node Number (UA in CMRI terms)


// this function will be called every time an output line changes value based on
// a CMRI 'T' transmit message

void cmriPerLineOutputHandler(uint16_t outputLine, bool isOn)
{
#if DEBUG
    Serial.print("changing output line ");
    Serial.print(outputLine);
    Serial.print(" to ");
    Serial.println(isOn ? "ON" : "OFF");
#endif

    // because this is based on IOLine, we can add any subclass of IOLine
    // to the outputs array without changing this function
    outputs[outputLine]->digitalWrite(isOn ? HIGH : LOW);
}



////////////////////////////////////////////////////////////////

void setup()
{
#ifdef rxPin
    pinMode(rxPin, INPUT);
#endif
#ifdef txPin
    pinMode(txPin, OUTPUT);
#endif

    // debug console
    Serial.begin(9600);

    // this is the CMRI port -- change the baud rate to whatever you use on your network
    // typical values will be 9600, 19200, 38400 or 57600
    Serial1.begin(57600, SERIAL_8N1);

#if DEBUG
    // won't run until the console is active, remove for normal operations
    while (!Serial);

    delay(5000);
    Serial.println("starting CMRI setup");
    cmri.addDebugStream(&Serial);
#endif

    cmri.setOutputHandler(outputCount, cmriPerLineOutputHandler, NULL);

    // initialize all the CMRI outputs
    for (int i = 0; i < outputCount; i++) {
	outputs[i]->init();
	outputs[i]->digitalWrite(LOW);
    }

    Serial.println("Done with setup");
}



////////////////////////////////////////////////////////////////


void loop()
{
    // put your main code here, to run repeatedly:
    cmri.check();
}
