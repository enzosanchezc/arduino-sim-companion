#define PACKET_SIZE 5
#define SYNC_TIMEOUT 10000
//#define DEBUG

#define SEQ_SHIFTER_UP 3
#define SEQ_SHIFTER_DOWN 4
#define HANDBRAKE A0
#define THROTTLE A1
#define BRAKE A2
#define CLUTCH A3
#define H_SHIFTER_X A4
#define H_SHIFTER_Y A5
#define H_SHIFTER_R 2

// Initialize H shifter state
uint8_t hGearState = 0;

// Initialize sequential shifter state
uint8_t seqState[2] = {1, 1};
// Time since last sequential gear pushed (debouncing)
uint32_t seqLastChange[2] = {0, 0};

// Initialize axis states
uint8_t axisStates[4] = {0, 0, 0, 0};
uint16_t hGearAxes[2] = {0, 0}; 

void sync() {
  const uint8_t SYNC_SEQ[] = {0xA3, 0x7C, 0xE9};
  bool syncing = true;
  int32_t syncStart;

  while (syncing) {
    uint8_t index = 0;
    Serial.write(SYNC_SEQ, sizeof(SYNC_SEQ));
    syncStart = millis();

    while (millis() - syncStart < SYNC_TIMEOUT) {
      if (Serial.available() > 0) {
        uint8_t receivedByte = Serial.read();// Read each incoming byte

        if (receivedByte == SYNC_SEQ[index]) {
          index++;
          if (index == sizeof(SYNC_SEQ)) {
            syncing = false;
          }
        } else {
          index = 0;  // Reset the buffer index if it doesn't match
        }
      }
    }
  }
}

void setup() {
  // Initialize interface with 16u2
  Serial.begin(9600);
  
  // Initialize inputs
  pinMode(H_SHIFTER_R, INPUT_PULLUP);
  pinMode(SEQ_SHIFTER_UP, INPUT_PULLUP);
  pinMode(SEQ_SHIFTER_DOWN, INPUT_PULLUP);

  // Sync with 16u2
  sync();
}

void loop() {
  // Read H Gear Shift
  hGearAxes[0] = analogRead(H_SHIFTER_X);
  hGearAxes[1] = analogRead(H_SHIFTER_Y);
  // Send H Gear Shift and Sequential
  hGearState = 0;
  // Check reverse
  if (digitalRead(H_SHIFTER_R) && hGearAxes[0] > 639 && hGearAxes[1] < 256) {
    hGearState = 7;
  } else {
    if (hGearAxes[0] < 385) {
      if (hGearAxes[1] > 768) hGearState = 1;
      if (hGearAxes[1] < 256) hGearState = 2;
    } else if (hGearAxes[0] > 639) {
      if (hGearAxes[1] > 768) hGearState = 5;
      if (hGearAxes[1] < 256) hGearState = 6;
    } else {
      if (hGearAxes[1] > 768) hGearState = 3;
      if (hGearAxes[1] < 256) hGearState = 4;
    }
  }

  // Debounce sequential gear shifter to avoid multiple gear changes with one push
  uint8_t butState = digitalRead(SEQ_SHIFTER_UP);
  if (butState != seqState[0] && millis() - seqLastChange[0] > 50) {
    seqLastChange[0] = 0;
    seqState[0] = butState;
  }
  butState = digitalRead(SEQ_SHIFTER_DOWN);
  if (butState != seqState[1] && millis() - seqLastChange[1] > 50) {
    seqLastChange[1] = 0;
    seqState[1] = butState;
  }
  
  // 3 LSB are for H state, 4th bit for seq+, 5th bit for seq-
  Serial.write(hGearState | !seqState[0] << 3 | !seqState[1] << 4);

  // Read axes and convert to 8bit
  axisStates[0] = analogRead(HANDBRAKE) >> 2;
  axisStates[1] = analogRead(THROTTLE) >> 2;
  axisStates[2] = analogRead(BRAKE) >> 2;
  axisStates[3] = analogRead(CLUTCH) >> 2;
  
  // Send axes
  for (int i = 0; i < 4; i++) Serial.write(axisStates[i]);

  // Debug delay
  delay(10);
}
