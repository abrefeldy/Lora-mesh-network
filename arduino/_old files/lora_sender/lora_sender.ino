#include <SPI.h>
#include <LoRa.h>

#define HIGH_BAND 868E6
#define LOW_BAND 169E6

int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender");
  LoRa.setSPIFrequency(1000000);
  LoRa.setPins(15, 0, 4);
  
  if (!LoRa.begin(LOW_BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(12);
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;

  delay(500);
}
