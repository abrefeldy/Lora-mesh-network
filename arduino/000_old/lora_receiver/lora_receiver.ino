#include <SPI.h>
#include <LoRa.h>

#define HIGH_BAND 868E6
#define LOW_BAND 169E6

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");
  LoRa.setSPIFrequency(1000000);
  LoRa.setPins(15, 0, 16);
  
  if (!LoRa.begin(LOW_BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(12);
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}
