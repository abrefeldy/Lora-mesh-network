#include <EEPROM.h>

// change this to be the ID of your node in the mesh network
uint8_t nodeId = 5;

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port to be available
  EEPROM.begin(512);
  Serial.println("setting nodeId...");

  EEPROM.write(0, nodeId);
  Serial.print(F("set nodeId = "));
  Serial.println(nodeId);

  uint8_t readVal = EEPROM.read(0);

  Serial.print(F("read nodeId: "));
  Serial.println(readVal);

  if (nodeId != readVal) {
    Serial.println(F("*** FAIL ***"));
  } else {
    Serial.println(F("SUCCESS"));
  }

  Serial.println("");
  if (EEPROM.commit()) {
    Serial.println("Data successfully committed");
  } else {
    Serial.println("ERROR! Data commit failed");
  }
}

void loop() {

}
