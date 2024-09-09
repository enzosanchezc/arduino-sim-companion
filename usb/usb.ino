#include "HID-Project.h"
#define PACKET_SIZE 5
//#define DEBUG

// Initialize buffer
uint8_t buffer[PACKET_SIZE];
uint8_t index = 0;

// Set H Shifter to neutral
uint8_t lastHGearState = 0;

void sync() {
  const uint8_t SYNC_SEQ[] = {0xA3, 0x7C, 0xE9};
  const uint8_t CALIBRATION_BYTE = 0xFA;
  index = 0;
  bool syncing = true;
  
  #ifdef DEBUG
  Serial.println("Syncing...");
  #endif
  while (syncing) {
    if (Serial1.available() > 0) {
      uint8_t receivedByte = Serial1.read();  // Read each incoming byte

      if (receivedByte == SYNC_SEQ[index]) {
        index++;
        if (index == sizeof(SYNC_SEQ)) {
          #ifdef DEBUG
          Serial.println("Synced");
          #endif
          syncing = false;
        }
      } else {
        index = 0;  // Reset the buffer index if it doesn't match
      }
    }
  }
  // Send ACK
  Serial1.write(SYNC_SEQ, sizeof(SYNC_SEQ));
}

void setup() {
  // Initialize interface with I/O
  Serial1.begin(9600);

  #ifdef DEBUG
  // Initialize serial for debugging
  Serial.begin(9600);
  #endif

  // Initialize Gamepad Library
  Gamepad.begin();
  Gamepad.releaseAll();

  // Sync with Uno
  sync();
}

void loop() {
  if (Serial1.available() >= PACKET_SIZE) {
    Serial1.readBytes(buffer, PACKET_SIZE);

    uint8_t gearState = buffer[0];
    
    #ifdef DEBUG
    Serial.print("H Shifter Gear: ");
    Serial.println((gearState & 0b00000111));
    Serial.print("Sequential Gear: ");
    Serial.print((gearState & 0b00001000));
    Serial.print(" ");
    Serial.println((gearState & 0b00010000));
    #endif
  
    // Update H Gear Shift
    uint8_t newHGearState = gearState & 0b00000111; // Extract the new gear state
    
    // If the gear has changed
    if (newHGearState != lastHGearState) {
      // If no gear is engaged, release all buttons
      if (newHGearState == 0) {
        for (int i = 0; i < 8; i++) {
          Gamepad.release(i);
        }
      } else {
        // If a previous gear was engaged, release it
        if (lastHGearState != 0) {
          Gamepad.release(lastHGearState - 1);
        }
        // Engage the new gear (mapping 1 to 0, 2 to 1, ..., 6 to 5)
        Gamepad.press(newHGearState);
      }
    
      // Update lastHGearState to the new gear state
      lastHGearState = newHGearState;
    }
  
    
    uint8_t shiftUp = (gearState & 0b00001000) >> 3;
    uint8_t shiftDown = (gearState & 0b00010000) >> 4;
    if (shiftUp) {
      Gamepad.press(8);
    } else {
      Gamepad.release(8);
    }
    if (shiftDown) {
      Gamepad.press(9);
    } else {
      Gamepad.release(9);
    }
      
    // Set Handbrake
    #ifdef DEBUG
    Serial.print("Handbrake: ");
    Serial.println(buffer[1]);
    #endif
    Gamepad.xAxis(map(buffer[1], 0, 255, -32768, 32767));
    // Set Throttle
    #ifdef DEBUG
    Serial.print("Throttle: ");
    Serial.println(buffer[2]);
    #endif
    Gamepad.yAxis(map(buffer[2], 0, 255, -32768, 32767));
    // Set Brake
    #ifdef DEBUG
    Serial.print("Brake: ");
    Serial.println(buffer[3]);
    #endif
    Gamepad.rxAxis(map(buffer[3], 0, 255, -32768, 32767));
    // Set Clutch
    #ifdef DEBUG
    Serial.print("Clutch: ");
    Serial.println(buffer[4]);
    #endif
    Gamepad.ryAxis(map(buffer[4], 0, 255, -32768, 32767));
    // Send to Gamepad
    Gamepad.write();
  }
}
