/*
 * This will take a unique code from the phone or a switch press
 * on the board to unlock (turn the light on).
 *
 */

#include <ArduinoBLE.h>

enum Pins {
  // LEDs
  Locked = 2,
  Unlocked = 3,
  Connected = 4,
  // Switches and buttons
  Button = 5,
  Switch = 6,
  // RFID RC522 SPI device
  SDA = 10,
  SCK = 13,
  MOSI = 11,
  MISO = 12,
  RST = 9
};

// The lock service
BLEService lockService("D270");

BLECharacteristic unlockChar("D271", BLEWrite, 20);
BLECharacteristic statusChar("D272", BLENotify, 20);

BLEDescriptor unlcokDesc("2901", "Unlock");
BLEDescriptor statusDesc("2901", "Status");

const char *secretCode = "abc123";

long lastOpenned = 0;
int lastBtnState = 0;
int lastSwState = 0;

void unlockMessageWritten(BLEDevice central, BLECharacteristic characteristic)
{
  Serial.print("Message received: ");
  char* message = (char *)characteristic.value();
  const int len = characteristic.valueLength();
  message[len] = '\0';

  if (strcmp(message, secretCode) == 0)
  {
    Serial.println("Valid code received - unlocking");
    lastOpenned = millis();
    digitalWrite(Pins::Unlocked, HIGH);
    digitalWrite(Pins::Locked, LOW);
    Serial.println(lastOpenned);
    statusChar.setValue("Unlocked");
  }
  else
  {
    Serial.print("Invalid code received: \"");
    Serial.print(message);
    Serial.print("\" -> ");
    Serial.println(len);
    statusChar.setValue("Locked");
  }
  characteristic.writeValue((byte)0);
}

void connectedHandler(BLEDevice central)
{
  Serial.print("Client connected: ");
  Serial.println(central.address());
  digitalWrite(Pins::Connected, HIGH);
}

void disconnectedHandler(BLEDevice central)
{
  Serial.print("Client disconnected: ");
  Serial.println(central.address());
  digitalWrite(Pins::Connected, LOW);
}



void setup() {
  Serial.begin(9600);
  while (!Serial)
  {
    delay(1);
  }

  if (!BLE.begin())
  {
    Serial.println("Failed to start BLE");
    while (true)
    {
      delay(100);
    }
  }

  Serial.print("KJD BLE Lock: ");
  Serial.println(BLE.address());

  pinMode(Pins::Connected, OUTPUT);
  pinMode(Pins::Locked, OUTPUT);
  pinMode(Pins::Unlocked, OUTPUT);
  pinMode(Pins::Button, INPUT);
  pinMode(Pins::Switch, INPUT);

  digitalWrite(Pins::Connected, LOW);
  digitalWrite(Pins::Locked, HIGH);
  digitalWrite(Pins::Unlocked, LOW);

  BLE.setDeviceName("Kris's BLE Lock (Device)");
  BLE.setLocalName("Kris's BLE Lock (local)");

  unlockChar.addDescriptor(unlcokDesc);
  statusChar.addDescriptor(statusDesc);

  unlockChar.setEventHandler(BLEWritten, unlockMessageWritten);

  lockService.addCharacteristic(unlockChar);
  lockService.addCharacteristic(statusChar);

  BLE.setEventHandler(BLEConnected, connectedHandler);
  BLE.setEventHandler(BLEDisconnected, disconnectedHandler);

  BLE.addService(lockService);
  BLE.advertise();
  statusChar.setValue("Locked");
  // put your setup code here, to run once:

}

typedef void inputToggleCallback(int pin, int newState);

void handleButtonPress(int pin, int newState)
{
  static long lastChange = millis();
  Serial.print(String("BUTTON HANDLER: (") + lastChange + "ms) Pin: " + pin + " toggled to ");
  Serial.println(newState ? "HIGH" : "LOW");
}

void handleSwitchPress(int pin, int newState)
{
  static long lastChange = millis();
  Serial.print(String("SWITCH HANDLER: (") + lastChange + "ms) Pin: " + pin + " toggled to ");
  Serial.println(newState ? "HIGH" : "LOW");
}

void checkInput(int pin, int *lastState, inputToggleCallback callback = nullptr)
{
  const int state1 = digitalRead(pin);
  // Debounce
  delay(10);
  const int state2 = digitalRead(pin);
  if ((state1 == state2) && (state1 != *lastState))
  {
    Serial.print(String("Input ") + pin + " toggled: ");
    Serial.println(state1 ? "HIGH": "LOW");
    *lastState = state1;
    if (callback != nullptr)
    {
      callback(pin, state1);
    }
  }
}

void loop() {
  BLE.poll();
  // put your main code here, to run repeatedly:
  if (lastOpenned != 0)
  {
    const long delta = millis() - lastOpenned;
    if (delta > 4000)
    {
      Serial.print("Locking again... ");
      lastOpenned = 0;
      digitalWrite(Pins::Unlocked, LOW);
      digitalWrite(Pins::Locked, HIGH);
      Serial.println("Locked");
    }
  }

  checkInput(Pins::Button, &lastBtnState, handleButtonPress);
  checkInput(Pins::Switch, &lastSwState, handleSwitchPress);


  // const int btnState1 = digitalRead(Pins::Button);
  // delay(10);
  // const int btnState2 = digitalRead(Pins::Button);
  // const bool btnMatch = btnState1 == btnState2;
  // if (btnMatch && lastBtnState != btnState1)
  // {
  //   Serial.print("Button ");
  //   Serial.println(btnState1 ? "Pressed!" : "Released!");
  //   lastBtnState = btnState1;
  // }

  // const int switchState1 = digitalRead(Pins::Switch);
  // delay(10);
  // const int switchState2 = digitalRead(Pins::Switch);
  // const bool swMatch = switchState1 == switchState2;
  // if (swMatch && lastSwState != switchState1)
  // {
  //   Serial.print("Switch ");
  //   Serial.println(switchState1 ? "Pressed!" : "Released!");
  //   lastSwState = switchState1;
  // }
}
