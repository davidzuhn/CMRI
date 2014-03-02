/* Computer Model Railroad Interface
 *
 * This package implements CMRInet, allowing a device
 * to be a participant on the CMRI RS-485 network.
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

#ifndef CMRI_H
#define CMRI_H

#include "Arduino.h"

class CMRI {
  public:
    CMRI(Stream & stream, uint8_t nodeId);

    void check();

    void setInitHandler(bool(*initHandler) (uint8_t * data, int dataLen));
    void setInputHandler(uint16_t numLines, bool(*inputHandler) (uint16_t line));
    void setOutputHandler(uint16_t numLines,
                          void (*perLineOutputHandler) (uint16_t line, bool isOn),
                          void (*overallOutputHandler) (uint16_t numOutputs, bool outputs[]));

    static const int MAX_MESG_LEN = 72;

    void addDebugStream(Stream * s);


    void printSummary();



  private:
    enum cmriStreamState { START, ATTN_NEXT, STX_NEXT, ADDR_NEXT,
        TYPE_NEXT, MAYBE_DATA_NEXT, DATA_NEXT
    };

    void sendMessage();

    void nextChar(uint8_t b);

     bool(*initHandler) (uint8_t * data, int dataLen);

    uint16_t numInputs;
    bool *inputs;
     bool(*inputHandler) (uint16_t line);

    uint16_t numOutputs;
    bool *outputs;
    void (*perLineOutputHandler) (uint16_t line, bool isOn);
    void (*overallOutputHandler) (uint16_t numOutputs, bool outputs[]);


    bool isForMe();

    void resetMessage();

    // true if successful, false if any error (buffer overflow, mainly)
    bool addCharToMessage(uint8_t b);

    void processMessage();

    void processInit();
    void pollInputs();
    void processOutputs();

    void processOtherMessages();

    void printCurrentMessage(const char *tag);


    char *printState(cmriStreamState);
    cmriStreamState currentState;


     Stream & stream;
    uint8_t nodeId;

    int messageDest;
    uint8_t messageType;
    uint8_t buf[MAX_MESG_LEN];
    int messageLength;

    Stream *debug;

    unsigned long int tickCount;
    unsigned long int charCount;
    unsigned long int messagesSeen;
    unsigned long int messagesProcessed;

    void changeState(cmriStreamState newstate, uint8_t inputChar);

    unsigned int errorCount;
    cmriStreamState error();

    uint16_t bytesForLines(uint16_t lines);

    void setOutput(uint16_t line, bool isOn);
};

#endif
