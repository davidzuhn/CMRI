/* simple example for CMRI on standalone cpNode hardwaee
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

// set to true if logging print statements are desired
#define DEBUG true

// set to true if the sketch should pause until the serial console is active
#define DEBUG_CONSOLE_REQUIRED false

// this needs to be unique on your CMRI network -- this is your C/MRI node
// ID (this was set by dip switches on older CMRI hardware such as SUSIC or
// SMINI)
#define CMRI_UA  31


// these next assignments are purely based on what you have attached
// to your hardware.  

// This is the trivial case of a single cpNode board with every I/O line
// configured as an output

IOLine *outputs[] = {
    new Pin(A4, OUTPUT),   // 0
    new Pin(A3, OUTPUT),   // 1
    new Pin(A2, OUTPUT),   // 2
    new Pin(A1, OUTPUT),   // 3
    new Pin(A0, OUTPUT),   // 4
    new Pin(13, OUTPUT),   // 5
    new Pin(12, OUTPUT),   // 6
    new Pin(11, OUTPUT),   // 7
    new Pin(10, OUTPUT),   // 8
    new Pin( 9, OUTPUT),   // 9 
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
#if DEBUG_CONSOLE_REQUIRED
    // won't run until the console is active, remove for normal operations
    while (!Serial);
    delay(5000);
#endif

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
