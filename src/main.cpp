#include <Homie.h>

#define HW_UART_SPEED    115200L
#define LED_BUILTIN      33

#define PIN_POWER_SENSE  14
#define PIN_POWER_BTN    15

const int STATE_UPDATE_INTERVAL = 300;

bool isAtxPowered = false;
bool lastAtxStatus = false;
unsigned long lastStateSent = 0;

HomieNode powerNode("power", "Power", "switch");

void IRAM_ATTR updatePowerState() {
  isAtxPowered = !digitalRead(PIN_POWER_SENSE); // read the input pin
}

bool powerOnHandler(const HomieRange& range, const String& value) {
  Homie.getLogger() << "Received " << value << endl;
  if (value != "true" && value != "false") 
  {
    return false;
  }
  bool powerOn = value == "true";

  if ((powerOn && isAtxPowered) || (!powerOn && !isAtxPowered)){
    Homie.getLogger() << "Power is already " << (powerOn ? "on" : "off") << endl;
    return false;
  }

  if (powerOn){
    // Trigger PC power button
    digitalWrite(PIN_POWER_BTN, LOW);
    delay(750);
    digitalWrite(PIN_POWER_BTN, HIGH);
    Homie.getLogger() << "Power on command send to PC" << endl;
  } else {
    Homie.getLogger() << "Request to power off unsupported" << endl;
  }

  return true;
}

void loopHandler() {
  if (millis() - lastStateSent >= STATE_UPDATE_INTERVAL * 1000UL || lastStateSent == 0 || 
      isAtxPowered != lastAtxStatus) {
    powerNode.setProperty("state").send(isAtxPowered ? "true" : "false");
    Homie.getLogger() << "Computer is " << (isAtxPowered ? "on" : "off") << endl;
    lastAtxStatus = isAtxPowered;
    lastStateSent = millis();
  }
}

void setup() {
  Serial.begin(HW_UART_SPEED);
  Serial << endl << endl;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_POWER_SENSE, INPUT);
  pinMode(PIN_POWER_BTN, OUTPUT);
  digitalWrite(PIN_POWER_BTN, HIGH);

  isAtxPowered = !digitalRead(PIN_POWER_SENSE);
  
  attachInterrupt(PIN_POWER_SENSE, updatePowerState, CHANGE);

  Homie_setFirmware("atx-remote", "1.0.0"); // The underscore is not a typo! See Magic bytes
  Homie.setLedPin(LED_BUILTIN, LOW);
  Homie.setLoopFunction(loopHandler);
  powerNode.advertise("state").setName("State").setDatatype("boolean").settable(powerOnHandler);
  Homie.setup();
}

void loop() {
  Homie.loop();
}