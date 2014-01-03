#include <Bounce.h>

#include <CMRI.h>

int out_leds[] = {
     2, 3, 4, 5, 6, 7 // ,8, 9, 10, 11, 12, 13
};

int in_buttons[] = { A5, A4, A3, A2, A1, A0 };



#define ledCount (sizeof(out_leds) / sizeof(int))

#define buttonCount (sizeof(in_buttons) / sizeof(int))

Bounce buttons[buttonCount];


#define rxPin 0
#define txPin 1

CMRI cmri(Serial1, 30);		// I/O device, my Node Number


bool cmriInputHandler(uint16_t inputLine)
{
    bool inputValue = false;
    

    if (inputLine < buttonCount) {
       inputValue = buttons[inputLine].read();
    }

    Serial.print("checking the input on line ");
    Serial.print(inputLine);
    Serial.print(": ");
    Serial.println(inputValue ? "ON" : "OFF");

    return inputValue;
}


void cmriOutputHandler(uint16_t outputLine, bool isOn)
{
    Serial.print("changing output line ");
    Serial.print(outputLine);
    Serial.print(" to ");
    Serial.println(isOn ? "ON" : "OFF");
    
    if (outputLine < ledCount) {
      digitalWrite(out_leds[outputLine], isOn ? HIGH : LOW);
    }
}


////////////////////////////////////////////////////////////////

void setup()
{
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
  
    Serial.begin(9600);

    // won't run until the console is active, remove for normal operations

    while (!Serial)
      ;
      
    delay(5000);
    Serial.println("starting CMRI setup");

    Serial1.begin(57600, SERIAL_8N1);

    cmri.addDebugStream(&Serial);

    cmri.setOutputHandler(ledCount, cmriOutputHandler);
 

    cmri.setInputHandler(buttonCount, cmriInputHandler);   

 
    for (int i = 0; i < ledCount; i++) {
	pinMode(out_leds[i], OUTPUT);
	digitalWrite(out_leds[i], LOW);
    }
    
    for (int i = 0; i < buttonCount; i++) {
       pinMode(in_buttons[i], INPUT);
       
       buttons[i].attach(in_buttons[i]);
       buttons[i].interval(10);
    } 

    Serial.println("Done with setup");
}



////////////////////////////////////////////////////////////////


int counter = 0;
void loop()
{
    
    // put your main code here, to run repeatedly:
  cmri.check();

  for (int i=0; i < buttonCount; i++) {
     buttons[i].update();
  }

}
