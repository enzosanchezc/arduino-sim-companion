#include "HID-Project.h"
#define PACKET_SIZE 5
#define HANDBRAKE_MAX 216
#define HANDBRAKE_MIN 44
#define HANDBRAKE_GAMMA 1.63
#define THROTTLE_MAX 44
#define THROTTLE_MIN 216
#define THROTTLE_GAMMA 1.63
#define BRAKE_MAX 216
#define BRAKE_MIN 44
#define BRAKE_GAMMA 1.63
#define CLUTCH_MAX 216
#define CLUTCH_MIN 44
#define CLUTCH_GAMMA 1.63
//#define DEBUG

// Initialize buffer
uint8_t buffer[PACKET_SIZE];
uint8_t index = 0;

// Set H Shifter to neutral
uint8_t lastHGearState = 0;

// Function to apply gamma correction
float gammaCorrection(float value, float gamma) {
  if (value >= 0) {
    return pow(value, 1 / gamma);
  } else {
    return -pow(-value, 1 / gamma);
  }
}

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
    Serial.println((gearState & 7));
    Serial.print("Sequential Gear: ");
    Serial.print((gearState & 8));
    Serial.print(" ");
    Serial.println((gearState & 16));
    #endif
  
    // Update H Gear Shift
    uint8_t newHGearState = gearState & 7; // Extract the new gear state
    
    // If the gear has changed
    if (newHGearState != lastHGearState) {
      // If no gear is engaged, release all buttons
      for (int i = 0; i < 8; i++) {
        Gamepad.release(i);
      }
      if (newHGearState != 0) {
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
    int16_t handbrakeInput = map(constrain(buffer[1], min(HANDBRAKE_MIN, HANDBRAKE_MAX), max(HANDBRAKE_MIN, HANDBRAKE_MAX)), HANDBRAKE_MIN, HANDBRAKE_MAX, -32768, 32767);
    Gamepad.xAxis(gammaCorrection(handbrakeInput / 32767.0, HANDBRAKE_GAMMA) * 32767);

    // Set Throttle with gamma correction
    #ifdef DEBUG
    Serial.print("Throttle: ");
    Serial.println(buffer[2]);
    #endif
    int16_t throttleInput = map(constrain(buffer[2], min(THROTTLE_MIN, THROTTLE_MAX), max(THROTTLE_MIN, THROTTLE_MAX)), THROTTLE_MIN, THROTTLE_MAX, -32768, 32767);
    Gamepad.yAxis(gammaCorrection(throttleInput / 32767.0, THROTTLE_GAMMA) * 32767);

    // Set Brake with gamma correction
    #ifdef DEBUG
    Serial.print("Brake: ");
    Serial.println(buffer[3]);
    #endif
    int16_t brakeInput = map(constrain(buffer[3], min(BRAKE_MIN, BRAKE_MAX), max(BRAKE_MIN, BRAKE_MAX)), BRAKE_MIN, BRAKE_MAX, -32768, 32767);
    Gamepad.rxAxis(gammaCorrection(brakeInput / 32767.0, BRAKE_GAMMA) * 32767);

    // Set Clutch with gamma correction
    #ifdef DEBUG
    Serial.print("Clutch: ");
    Serial.println(buffer[4]);
    #endif
    int16_t clutchInput = map(constrain(buffer[4], min(CLUTCH_MIN, CLUTCH_MAX), max(CLUTCH_MIN, CLUTCH_MAX)), CLUTCH_MIN, CLUTCH_MAX, -32768, 32767);
    Gamepad.ryAxis(gammaCorrection(clutchInput / 32767.0, CLUTCH_GAMMA) * 32767);

    // Send to Gamepad
    Gamepad.write();
  }
}
