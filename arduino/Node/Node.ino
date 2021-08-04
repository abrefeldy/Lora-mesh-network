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
// Ack indication
const int AckInd = 100;
// outgoing message
String outgoing;
// count of outgoing messages
byte localCount = 0;
// address of this device, will be read from EEPROM later
byte localAddress = 0x00;
// destination to send to
byte destination = 0xFF;
// table to store the received counts
bool counts[2][255] = {false};
// table to store the received ACKs
bool acks[2][255] = {false};


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

  Serial.println("isEndDevice: " + String(isEndDevice));

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
  if (isEndDevice) {
    char incomingMsg[100];
    int index = 0;
    bool gotMsg = Serial.available() > 0;
    while (Serial.available() > 0) {
      char incomingByte = Serial.read();
      incomingMsg[index] = incomingByte;
      index++;
      delay(5);
    }
    incomingMsg[index - 1] = '\0';
    if (gotMsg) {
      Serial.println("Sent Message (" + (String)localCount + "): " + (String)incomingMsg);
      sendMessage(destination, localAddress, localCount, incomingMsg);
      localCount++;
    }
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void strobeLEDs() {
  if (isEndDevice) {
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
    }
  } else {
    for (int i = 0; i < 5; i++) {
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
    char* msgChar;
    msg.toCharArray(msgChar, msgLength);
    if (count == AckInd) {
      Serial.println("ACK MECHANISM " + String(src) + ", " + String(atoi(msgChar)));
      if (!acks[src - 1][atoi(msgChar)]) {
        Serial.println("ACK MECHANISM didn't see this ack");
        acks[src - 1][atoi(msgChar)] = true;
        Serial.println("ACK (count = " + String(msgChar) + ", " + String(src, HEX) + "->" + String(dst, HEX) + ")");
        sendMessage(dst, src, AckInd, String(count) + " >ACK> from node");
        return;
      } else {
        Serial.println("Already seen this ACK (" + String(msgChar) + "), not forwarding");
        return;
      }
    }

    // check if the massege wasn't recieved yet & if the massage is not ack
    if (!counts[src - 1][count]) {
      Serial.println("This message is not for me (count = " + String(count) + ", " + String(src, HEX) + "->" + String(dst, HEX) + ")" );
      Serial.println("Forwarding the message.");
      sendMessage(dst, src, count, msg + " >MSG> from node");
      // Add the massage to the recieved ones
      counts[src - 1][count] = true;
      if (dst != 0xff) {
        return;
      }
    } else {
      Serial.println("Already seen this message (" + String(count) + "), not forwarding");
      return;
    }
  }

  // if message is for this device, or broadcast, print details:
  if (dst == localAddress) {
    // If Ack
    if (count == AckInd) {
      Serial.println("ACK (" + msg + ")");
      Serial.println("--------------------------------");
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
    if (isEndDevice && count != AckInd && dst != 0xff) {
      sendMessage(destination, localAddress, AckInd, String(count));
    }
  }
}
