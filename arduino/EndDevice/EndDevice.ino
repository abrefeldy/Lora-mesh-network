// include libraries
#include <SPI.h>

#include <LoRa.h>

#include <EEPROM.h>

// define some parameters
#define HIGH_BAND 868E6
#define LOW_BAND 169E6
#define TX_POWER 20

// LoRa radio chip select
const int csPin = 15;
// LoRa radio reset
const int resetPin = 0;
// must be a hardware interrupt pin
const int irqPin = 4;

const int spreadingFactor = 10;
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

void setup() {
  // initialize serial
  Serial.begin(115200);
  while (!Serial);

  delay(100);
  Serial.println();
  Serial.println("--------START OF PROGRAM--------");

  // read eeprom to get ID:
  EEPROM.begin(512);
  localAddress = EEPROM.read(0);
  Serial.println("Local address: " + String(localAddress, HEX));
  EEPROM.end();

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
  //LoRa.setSyncWord(syncWord);
  //LoRa.enableCrc();
  //LoRa.setCodingRate4(codingRateDenominator);
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    // send a message
    String message = "HeLoRa World!";
    sendMessage(message);
    Serial.println("Sending " + message);

    // timestamp the message
    lastSendTime = millis();

    // 2-3 seconds
    interval = random(2000) + 1000;
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing) {
  // start packet
  LoRa.beginPacket();
  // add destination address
  LoRa.write(destination);
  // add sender address
  LoRa.write(localAddress);
  // add message ID
  LoRa.write(msgCount);
  // add payload length
  LoRa.write(outgoing.length());
  // add payload
  LoRa.print(outgoing);
  // finish packet and send it
  LoRa.endPacket();
  // increment message ID
  msgCount++;
}

void onReceive(int packetSize) {
  // if there's no packet, return
  if (packetSize == 0) return;

  // read packet header bytes, start with recipient address
  int recipient = LoRa.read();
  // sender address
  byte sender = LoRa.read();
  // incoming msg ID
  byte incomingMsgId = LoRa.read();
  // incoming msg length
  byte incomingLength = LoRa.read();

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char) LoRa.read();
  }

  // check length for error
  if (incomingLength != incoming.length()) {
    Serial.println("error: message length does not match length");
    // skip rest of function
    return;
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    // skip rest of function
    return;
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}
