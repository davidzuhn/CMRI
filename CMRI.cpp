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


#include "CMRI.h"

#define ATTN 0xFF
#define STX  0x02
#define ETX  0x03
#define DLE  0x10
#define ACK  0x06
#define NAK  0x15

/*
 * define this if the serial stream parser should be completely 100%
 * picky about the protocol (as defined in Build Your Own Universal
 * Computer Interface, 2d Edition, by Bruce Chubb).   
 *
 * I don't see a reason why this must be defined, but maybe you have a
 * convincing argument
 */
#define STRICT_PROTOCOL_CHECKING false

/******************************************************************************
 *
 * These are the PUBLIC methods of the CMRI object 
 *
 ******************************************************************************
 */

/*
 * Create a CMRI protocol parser object.  The required arguments are a Stream object,
 * typically a serial port such as Serial or Serial1, and the node identifier (the
 * value one would set by the address DIP switches on the SUSIC or SMINI). 
 *
 * The nodeId value should range between 0 and 127.
 *
 */

CMRI::CMRI(Stream & s, uint8_t n):
nodeId(n + 65), stream(s)
{
    messageLength = 0;
    currentState = START;
    debug = NULL;
    tickCount = 0;
    charCount = 0;
    messagesSeen = 0;
    messagesProcessed = 0;
    errorCount = 0;
    inputHandler = NULL;
    perLineOutputHandler = NULL;
    overallOutputHandler = NULL;
    inputs = NULL;
    outputs = NULL;
}


/*
 * Perform the next bit of processing on a CMRI protocol stream.  This
 * should be called frequently; ideally once per iteration of the Arduino
 * loop() method.  
 *
 * Because this method does not block, processing of other functions may
 * occur inside the loop() method.
 */

void CMRI::check()
{
    tickCount += 1;

    if (stream.available() < 1) {
        return;
    }

    while (stream.available()) {
        int b = stream.read();

        if (b >= 0) {
            // this need
            nextChar((uint8_t) b);
        } else {
            if (debug) {
                debug->println("error reading from available stream");
            }
        }
    }
}


/*
 * called by the user program to install a function to be called when
 * an initialization message (I) is received
 */
void CMRI::setInitHandler(bool(*initHandler) (uint8_t * data, int datalen))
{
    this->initHandler = initHandler;
}



/* 
 * called by the user program to install a function to be called when an
 * input poll message (P) is received
 */

void CMRI::setInputHandler(uint16_t numLines, bool(*inputHandler) (uint16_t line))
{
    if (this->inputHandler == NULL || inputs == NULL) {
        if (debug) {
            debug->println("inputHandler set");
        }

        this->inputHandler = inputHandler;

        if (debug)
            debug->println("creating inputs");

        numInputs = numLines;
        inputs = (bool *) calloc(sizeof(bool), numInputs);
    }
}

/* 
 * called by the user program to install a function to be called when an
 * output/transmit message (T) is received
 */

void CMRI::setOutputHandler(uint16_t numLines,
                            void (*perLineOutputHandler) (uint16_t line, bool isOn),
                            void (*overallOutputHandler) (uint16_t numOutputs, bool outputs[]))
{
    if (perLineOutputHandler == NULL || overallOutputHandler == NULL || outputs == NULL) {
        this->perLineOutputHandler = perLineOutputHandler;
        this->overallOutputHandler = overallOutputHandler;

        if (debug)
            debug->println("creating outputs");

        numOutputs = numLines;
        outputs = (bool *) calloc(sizeof(bool), numOutputs);
    }
}

/*
 * Let the user program add a stream to use for debug messages 
 * 
 * This can be a second serial port, or an established network
 * connection. Both have been tested.  No reconnection efforts are taken,
 * so network connections are not the most reliable.
 */

void CMRI::addDebugStream(Stream * s)
{
    debug = s;
}


/*
 * A debugging routine to print some information about the CMRI object to
 * the debug stream.  
 */

void CMRI::printSummary()
{
    if (debug) {
        debug->println("CMRI communications summary");
        debug->print("  tickCount: ");
        debug->println(tickCount);
        debug->print("  charCount: ");
        debug->println(charCount);
        debug->print("  messagesSeen: ");
        debug->println(messagesSeen);
        debug->print("  messagesProcessed: ");
        debug->print(messagesProcessed);
        debug->println("");
    }
}





/******************************************************************************
 *
 * These are the utility functions used by the CMRI object
 *
 ******************************************************************************
 */


/* Read a specific bit in an array of unsigned bytes */
static bool getBit(uint8_t * data, uint16_t dataLen, uint16_t bit)
{
    bool value = false;

    // do not allow overflow
    uint16_t byte = bit / 8;
    uint8_t offset = bit % 8;

    if (byte < dataLen) {
        value = data[byte] & (1 << offset);
    }

    return value;
}

/* Set a specific bit in an array of unsigned bytes */
static void setBit(uint8_t * data, uint16_t dataLen, uint16_t bit, bool value)
{
    uint16_t byte = bit / 8;
    uint8_t offset = bit % 8;

    if (byte < dataLen) {
        if (value) {
            data[byte] |= 1 << offset;
        } else {
            data[byte] &= ~(1 << offset);
        }
    }
}




/******************************************************************************
 *
 * These are the PRIVATE methods of the CMRI object 
 *
 ******************************************************************************
 */


/*
 * note an error during stream parsing, keeping track of an error count 
 */
CMRI::cmriStreamState CMRI::error()
{
    errorCount += 1;
    return START;
}


/*
 * Determine whether the current message should be processed by this node.
 *
 * There are expectations of a future protocol extension to allow for
 * a broadcast message, which should be processed by every node on the
 * CMRI network.
 */

bool CMRI::isForMe()
{
    if (messageDest == nodeId)
        return true;
    else
        return false;
}


/*
 * print the current message to the debug stream in a text form
 *
 */

void CMRI::printCurrentMessage(const char *tag)
{
    if (debug) {
        if (tag)
            debug->print(tag);

        debug->print(messageDest - 65); // display user friendly value
        debug->print("  type: 0x");
        debug->print(messageType, HEX);
        if (isprint(messageType)) {
            debug->print(" '");
            debug->print((char) messageType);
            debug->print("'");
        }
        debug->print("\n---- ");

        if (messageLength > 0) {
            for (int i = 0; i < messageLength; i++) {
                debug->print(buf[i], HEX);
                debug->print(" ");
            }
            debug->println("");
        }
    }
}



/*
 * Do whatever we are supposed to do with the current message (this may
 * include doing nothing because the message is not for this node).
 *
 * Future code will implement the extended CMRInet protocol as proposed by
 * Catania & Neumann (2013).   This will be done either by directly 
 * adding the code in this library, or by allowing for user programs to
 * register callback handlers for specific CMRI protocol message types.
 *
 * Unknown message types are silently ignored.
 * 
 */

void CMRI::processMessage()
{
    messagesSeen += 1;

    if (debug) {
        printCurrentMessage("---- complete message received: dest ");
    }
    // do not process the message if it is not addressed to us
    if (!isForMe())
        return;


    // now do something with the message
    if (debug) {
        debug->println("---- processing this message");
    }

    messagesProcessed += 1;

    switch (messageType) {
    case 'I':
        processInit();
        break;
    case 'T':
        processOutputs();
        break;
    case 'P':
        pollInputs();
        break;
    default:
        // can't do anything with this message, I don't know what it is
        break;
    }
}


/* Add the given character to the current message, checking to make sure
 * the message buffer is not overflowed.
 */

bool CMRI::addCharToMessage(uint8_t b)
{
    // we cannot save the data if it exceeds our maximum message length
    if (messageLength >= MAX_MESG_LEN) {
        return false;
    }

    buf[messageLength++] = b;
    return true;
}


/*
 * Throw away all knowledge about the current message
 */

void CMRI::resetMessage()
{
    messageType = 0;
    messageDest = -1;
    messageLength = 0;
}


/* 
 * Return a textual version of the cmriStreamState enum, used for debugging 
 */
char *CMRI::printState(cmriStreamState state)
{
    char *str;
    switch (state) {
    case START:
        str = "START";
        break;
    case ATTN_NEXT:
        str = "ATTN_NEXT";
        break;
    case STX_NEXT:
        str = "STX_NEXT";
        break;
    case ADDR_NEXT:
        str = "ADDR_NEXT";
        break;
    case TYPE_NEXT:
        str = "TYPE_NEXT";
        break;
    case MAYBE_DATA_NEXT:
        str = "MAYBE_DATA_NEXT";
        break;
    case DATA_NEXT:
        str = "DATA_NEXT";
        break;
    default:
        str = "*** UNKNOWN ***";
        break;
    }

    return str;
}


/*
 * Set the current state to the given state.
 *
 * This is used to centralize debugging messages, hence the inputChar
 * argument, which is expected to be the character that was just read
 * to move the state machine to the new state.
 */

void CMRI::changeState(cmriStreamState newState, uint8_t inputChar)
{
    if (0 && debug) {
        debug->print("Current state: ");
        debug->print(printState(currentState));
        debug->print(", input char 0x");
        debug->print((int) inputChar, HEX);
        debug->print("  ===>  ");
        debug->println(printState(newState));

        if (newState == START) {
            debug->print("\n\nStart of new message processing\n");
        }
    }

    currentState = newState;
}


/*
 * Given the CMRI stream state machine, and a new character that
 * has been read from the serial stream, move to the next state
 * depending on what the input character is.
 *
 */

void CMRI::nextChar(uint8_t b)
{
    charCount += 1;

    switch (currentState) {
    case START:
        // we can only leave START with an ATTN byte
        changeState((b == ATTN) ? ATTN_NEXT : error(), b);
        resetMessage();
        break;

    case ATTN_NEXT:
        // but we must have two of them in a row
        changeState((b == ATTN) ? STX_NEXT : error(), b);
        break;

    case STX_NEXT:
        // two ATTNs should be followed by STX, or else we start over
        changeState((b == STX) ? ADDR_NEXT : error(), b);
        break;

    case ADDR_NEXT:
        // once we've started the message, the next couple of bytes
        // are of fixed interpretation.   First comes the message
        // destination byte
        messageDest = (uint8_t) b;
        changeState(TYPE_NEXT, b);
        break;

    case TYPE_NEXT:
        // and then comes the message type byte
        messageType = b;
        changeState(MAYBE_DATA_NEXT, b);
        break;

    case MAYBE_DATA_NEXT:
        // after the destination and type comes the data portion of the
        // message.  This portion may be empty.  To put the characters
        // ETX, STX or DLE into the message, the byte is preceded by
        // DLE.  
        switch (b) {
        case ETX:
	    // we have reached the end of the message, so we do something
	    // with it, and then start on the next one
            processMessage();
            changeState(START, b);
            break;
        case DLE:
	    // the next character will be a data character (the DLE escapes
	    // an ETX, STX, or DLE
            changeState(DATA_NEXT, b);
            break;
        default:
	    // if it's not a special character, then we try to add this to
	    // the message buffer.  If that fails (the message is too long),
	    // then this is an error.
            changeState(addCharToMessage(b) ? MAYBE_DATA_NEXT : error(), b);
            break;
        }
        break;

    case DATA_NEXT:
	// the next character will be added to the message buffer, even if
	// it is special to the protocol (in other words, a DLE escape is
	// in effect

#if STRICT_PROTOCOL_CHECKING
        // to be completely true to the protocol, this should probably reject
        // escaped data characters that are not STX, ETX or DLE

        if ((b == STX || b == ETX || b == DLE) && addCharToMessage(b)) {
            changeState(MAYBE_DATA_NEXT, b);
        } else {
            changeState(error(), b);
        }
#else
        // be liberal in what you accept, strict in what you emit

        changeState(addCharToMessage(b) ? MAYBE_DATA_NEXT : error(), b);
#endif
        break;

    default:
        // THIS CASE SHOULD NEVER HAPPEN.  I don't know what else to do
        // here except to start reading the next message
        if (debug) {
            debug->print("Unknown CMRI state transition: state is ");
            debug->print(currentState);
            debug->print(", input char is ");
            debug->println(b, HEX);
        }
        changeState(error(), b);
    }
}



/*
 * set the state of the local model for the output lines, checking to make 
 * sure we don't process more lines than we initialized
 *
 * The user defined output handler function is called, with the current
 * output line number and the current state (on, !on)
 */
void CMRI::setOutput(uint16_t line, bool isOn)
{
    if (outputs != NULL && line < numOutputs) {
        outputs[line] = isOn;
        if (perLineOutputHandler != NULL) {
            (*perLineOutputHandler) (line, isOn);
        }
    }
}


/*
 * this is used to respond to the P message.  
 *
 * for each of the input lines that we have configured, call the
 * inputHandler to get the current state of the input line.
 *
 * Then assemble each of the returned bit values into a byte array of
 * suitable size and then send the collection of values back in a
 * response (R) message.
 */

void CMRI::pollInputs()
{
    if (debug) {
        debug->println("pollInputs()");
    }

    if (inputHandler == NULL || inputs == NULL) {
        return;
    }

    for (uint16_t i = 0; i < numInputs; i++) {
        if (0 && debug) {
            debug->print("checking input ");
            debug->println(i);
        }

        bool val = (*inputHandler) (i);

        inputs[i] = val;
    }

    uint16_t messageByteCount = (numInputs / 8) + 1;
    if (messageByteCount < MAX_MESG_LEN) {
        for (int i = 0; i < numInputs; i++) {
            setBit(buf, MAX_MESG_LEN, i, inputs[i]);
        }

        messageType = 'R';
        messageLength = messageByteCount;
        sendMessage();
    }
}


/*
 * this is used to respond to the initialization (I) message
 * 
 * call the install initHandler (if there is one) with the
 * message contents
 */

void CMRI::processInit()
{
    if (initHandler) {

        bool rv = (*initHandler) (buf, messageLength);

        // potentially do something with the initialization data
    }
}


/*
 * this is used to process the transmit (T) message
 *
 * for each bit in the sent collection of output bits,
 * compare the value to the local model values
 *
 * If there is any change, call setOutput which will do whatever needs to
 * be done (update the local model and then call the outputHandler
 * callback).
 *
 */

void CMRI::processOutputs()
{
    bool anyChanges = false;

    if (debug) {
        debug->println("processOutputs()");
    }

    if (outputs == NULL) {
        if (debug)
            debug->println("no outputs object");
        return;
    }


    // go through each output bit, and see if it has changed
    // from the last time around.  If it has, perform a callback
    // with that state.

    for (uint16_t i = 0; i < numOutputs; i++) {
        if (0 && debug) {
            debug->print("checking status of line ");
            debug->println(i);
        }

        bool local = outputs[i];
        bool incoming = getBit(buf, messageLength, i);

        if (local != incoming) {
            anyChanges = true;
            setOutput(i, incoming);
        }
    }

    // lastly, we call the overallOutputHandler to let the
    // user deal with things in bulk.  This is only done if
    // anything has changed since the last time around, and
    // if the overallOutputHandler is actually defined.

    if (anyChanges && overallOutputHandler != NULL) {
	(*overallOutputHandler)(numOutputs, outputs);
    }

}


/*
 * this is used to send the current message contents to the serial stream
 * (as from the response to the 'P' message)
 *
 * This needs a better API to allow user handlers to other message types to
 * be able to send responses.  Right now this method depends entirely on
 * private data types (which pollInputs has access to).
 *
 */

void CMRI::sendMessage()
{
    if (debug) {
        printCurrentMessage("---- complete message being sent: sender ");
    }

    stream.write(0xff);
    stream.write(0xff);
    stream.write(STX);

    stream.write(nodeId);
    stream.write(messageType);

    for (int i = 0; i < messageLength; i++) {
        if (buf[i] == ETX || buf[i] == STX || buf[i] == DLE) {
            stream.write(DLE);
        }
        stream.write(buf[i]);
    }

    stream.write(ETX);
}
