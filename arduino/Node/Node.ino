// include libraries
#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>

// define some parameters
#define HIGH_BAND 868E6
#define LOW_BAND 169E6
#define TX_POWER 20
#define BANDWIDTH 125E3


// Is the device an end device or node
bool isEndDevice = false;
// LoRa radio chip select
const int csPin = 15;
// LoRa radio reset
const int resetPin = 0;
// must be a hardware interrupt pin
const int irqPin = 4;
// set spreading factor
const int spreadingFactor = 12;
// set LoRa Tx power
const int txPower = 20;
// LoRa sync word, see LoRa-master api
const byte syncWord = 0x12;
// LoRa coding rate, see LoRa-master api
const int codingRateDenominator = 2;
// outgoing message
String outgoing;
// count of outgoing messages
byte msgCount = 0;
// address of this device, will be read from EEPROM later
byte localAddress = 0x00;
// destination to send to
byte destination = 0xFF;
// last send time
long lastSendTime = 0;
// interval between sends
int interval = 2000;
// Ack indication
const int AckInd = 255;

void setup() {
  // initialize serial
  Serial.begin(9600);
  while (!Serial);

  delay(100);
  Serial.println();
  Serial.println("--------START OF PROGRAM--------");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // read eeprom to get ID:
  EEPROM.begin(512);
  localAddress = EEPROM.read(0);
  Serial.println("Local address: 0x" + String(localAddress, HEX));
  EEPROM.end();

  // check if this node is end device
  if (localAddress == 0x01 || localAddress == 0x02) {
    isEndDevice = true;
    if (localAddress == 0x01) {
      destination = 0x02;
    } else {
      destination = 0x01;
    }
  }

  Serial.println("isEndDevice: " + isEndDevice);

  strobeLEDs();

  // override the default CS, reset, and IRQ pins
  LoRa.setPins(csPin, resetPin, irqPin);

  // initialize LoRa at selected band
  if (!LoRa.begin(LOW_BAND)) {
    Serial.println("LoRa init failed. Check your connections.");
    // if failed, do nothing
    while (true);
  }

  // initialize LoRa transceiver with the chosen parameters
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setTxPower(txPower, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSignalBandwidth(BANDWIDTH);
  //LoRa.setSyncWord(syncWord);
  //LoRa.enableCrc();
  //LoRa.setCodingRate4(codingRateDenominator);
  Serial.print("SF: " + String(spreadingFactor) + ", BW: ");
  Serial.println(BANDWIDTH);
  Serial.println("LoRa init succeeded.");
  Serial.println("--------------------------------");
}

void loop() {
  // if end deivce, send message:
  /*if (isEndDevice && millis() - lastSendTime > interval) {
    // send a message
    String message = "HeLoRa World!";
    sendMessage(destination, localAddress, msgCount, message);
    msgCount++;
    Serial.println("Sending " + message);

    // timestamp the message
    lastSendTime = millis();

    // 2-3 seconds
    interval = random(2000) + 10000;
  }*/

  if(isEndDevice){
    char incomingMsg[100];
    int index = 0;
    bool gotMsg = Serial.available() > 0;
    while(Serial.available() > 0){
      char incomingByte = Serial.read();
      incomingMsg[index] = incomingByte;
      index++;
      delay(5);
    }
    incomingMsg[index - 1] = '\0';
    if(gotMsg){
      Serial.println("Sent Message (" + (String)msgCount + "): " + (String)incomingMsg);
      sendMessage(destination, localAddress, msgCount, incomingMsg);
      msgCount++;
    }
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void strobeLEDs(){
  if(isEndDevice){
    for(int i = 0; i < 10; i++){
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
    }
  } else {
    for(int i = 0; i < 5; i++){
      digitalWrite(LED_BUILTIN, LOW);
      delay(40);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(40);
      digitalWrite(LED_BUILTIN, LOW);
      delay(40);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(40);
      digitalWrite(LED_BUILTIN, LOW);
      delay(40);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
    }
  }
}
void sendMessage(byte dst, byte src, byte count, String msg) {
  // start packet
  LoRa.beginPacket();
  // add destination address
  LoRa.write(dst);
  // add sender address
  LoRa.write(src);
  // add message ID
  LoRa.write(count);
  // add payload length
  LoRa.write(msg.length());
  // add payload
  LoRa.print(msg);
  // finish packet and send it
  LoRa.endPacket();
}

void onReceive(int packetSize) {
  // if there's no packet, return
  if (packetSize == 0) return;

  // read packet header bytes, start with recipient address
  byte dst = LoRa.read();
  // sender address
  byte src = LoRa.read();
  // incoming msg ID
  byte count = LoRa.read();
  // incoming msg length
  byte msgLength = LoRa.read();

  String msg = "";

  while (LoRa.available()) {
    msg += (char) LoRa.read();
  }

  // check length for error
  if (msgLength != msg.length()) {
    Serial.println("error: message length does not match length");
    // skip rest of function
    return;
  }

  // if we are nodes and we are not the destination of the msg, forward it
  if (!isEndDevice && (dst != localAddress || dst == 0xff)) {
    Serial.println("This message is not for me.");
    Serial.println("Forwarding the message.");
    sendMessage(dst, localAddress, count, msg);
    if (dst != 0xff) {
      return;
    }
  }

  // if message is for this device, or broadcast, print details:
  if (dst == localAddress) {
    // If Ack
    if (count == AckInd) {
      Serial.println("ACK (" + msg + ")");
      return;
    }
    
    // Regular Message
    Serial.print("src: 0x" + String(src, HEX));
    Serial.print(" dst: 0x" + String(dst, HEX));
    Serial.print(" count: " + String(count));
    Serial.print(" length: " + String(msgLength));
    Serial.print(" rssi: " + String(LoRa.packetRssi()));
    Serial.println(" snr: " + String(LoRa.packetSnr()));
    Serial.println("msg: " + msg);
    Serial.println("--------------------------------");

    //Send Ack
    if (count != AckInd && dst != 0xff) {
      sendMessage(destination, localAddress, AckInd, String(count));
    }
  }
}
