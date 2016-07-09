/*****************************************************************
 *                                                               *
 * Reiley's Wheel Widget                                         *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, June 2016                                 )'( *
 *                                                               *
 *****************************************************************/

#define ENCODER_0_A_PIN A0
#define ENCODER_0_B_PIN A1
#define ENCODER_1_A_PIN A2
#define ENCODER_1_B_PIN A3
#define ENCODER_2_A_PIN A4
#define ENCODER_2_B_PIN A5
#define ENCODERS_VDD_PIN 8
#define ENCODER_ACTIVE_PIN 7

#define GREEN_LED_PIN 4
#define ORANGE_LED_PIN 5

#define SPIN_ACTIVITY_DETECT_MS 50L
#define SPIN_INACTIVITY_TIMEOUT_MS 500L
#define XMIT_INTERVAL_MS 1000L

#define NUM_ENCODERS 3
#define NUM_STEPS_PER_REV 18

volatile uint8_t g_lastEncoderStates = 0;
volatile int g_encoderValues[] = {0, 0, 0};
const int8_t g_greyCodeToEncoderStepMap[] = {
       // last this
/*
   0,  //  00   00
  -1,  //  00   01
   1,  //  00   10
   0,  //  00   11
   1,  //  01   00
   0,  //  01   01
   0,  //  01   10
  -1,  //  01   11
  -1,  //  10   00
   0,  //  10   01
   0,  //  10   10
   1,  //  10   11
   0,  //  11   00
   1,  //  11   01
  -1,  //  11   10
   0,  //  11   11
*/
       // this is not really a Grey code
   0,  //  00   00
   1,  //  00   01
   1,  //  00   10
   1,  //  00   11
   1,  //  01   00
   0,  //  01   01
   1,  //  01   10
   1,  //  01   11
   1,  //  10   00
   1,  //  10   01
   0,  //  10   10
   1,  //  10   11
   1,  //  11   00
   1,  //  11   01
   1,  //  11   10
   0,  //  11   11
};


void setUpPinChangeInterrupt(uint8_t pin) 
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group 
}


// Service pin change interrupt for A0 - A5.
ISR (PCINT1_vect)
{
  uint8_t encoderStates = PINC;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastEncoderStates;

#ifdef GREEN_LED_PIN
//  digitalWrite(GREEN_LED_PIN,  encoderStates & 0b101010);
//  digitalWrite(ORANGE_LED_PIN, encoderStates & 0b010101);
  digitalWrite(GREEN_LED_PIN,  encoderStates & 0b000001);
  digitalWrite(ORANGE_LED_PIN, encoderStates & 0b000010);
//  digitalWrite(GREEN_LED_PIN,  encoderStates & 0b000100);
//  digitalWrite(ORANGE_LED_PIN, encoderStates & 0b001000);
//  digitalWrite(GREEN_LED_PIN,  encoderStates & 0b010000);
//  digitalWrite(ORANGE_LED_PIN, encoderStates & 0b100000);
#endif

  for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
    uint8_t idx = ((lastStates & 0b11) << 2) | (curStates & 0b11);
    g_encoderValues[i] += g_greyCodeToEncoderStepMap[idx];
    curStates >>= 2;
    lastStates >>= 2;
  }

  g_lastEncoderStates = encoderStates;
}


void setup()
{
  Serial.begin(115200);

  pinMode(ENCODER_0_A_PIN, INPUT); 
  pinMode(ENCODER_0_B_PIN, INPUT);
  pinMode(ENCODER_1_A_PIN, INPUT); 
  pinMode(ENCODER_1_B_PIN, INPUT);
  pinMode(ENCODER_2_A_PIN, INPUT); 
  pinMode(ENCODER_2_B_PIN, INPUT);
  pinMode(ENCODERS_VDD_PIN, OUTPUT);
  pinMode(ENCODER_ACTIVE_PIN, OUTPUT);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(ORANGE_LED_PIN, OUTPUT);

  // Turn on pullups.
  digitalWrite(ENCODER_0_A_PIN, HIGH);
  digitalWrite(ENCODER_0_B_PIN, HIGH);
  digitalWrite(ENCODER_1_A_PIN, HIGH);
  digitalWrite(ENCODER_1_B_PIN, HIGH);
  digitalWrite(ENCODER_2_A_PIN, HIGH);
  digitalWrite(ENCODER_2_B_PIN, HIGH);
  
  // Initially, turn on power to the encoders and set the active indicator low.
  digitalWrite(ENCODERS_VDD_PIN, HIGH);
  digitalWrite(ENCODER_ACTIVE_PIN, LOW);

  setUpPinChangeInterrupt(ENCODER_0_A_PIN);
  setUpPinChangeInterrupt(ENCODER_0_B_PIN);
  setUpPinChangeInterrupt(ENCODER_1_A_PIN);
  setUpPinChangeInterrupt(ENCODER_1_B_PIN);
  setUpPinChangeInterrupt(ENCODER_2_A_PIN);
  setUpPinChangeInterrupt(ENCODER_2_B_PIN);
}


void loop()
{
  static int32_t lastEncoderValues[3] = {0, 0, 0};
  static int32_t encoderValueDeltas[3] = {0, 0, 0};
  static bool encoderActive[3] = {false, false, false};
  static uint32_t lastEncoderInactiveMs[3] = {0, 0, 0};
  static uint32_t lastEncoderChangeMs[3] = {0, 0, 0};
  static uint32_t nextXmitMs = 0;

  unsigned long now = millis();

  for (int i = 0; i < NUM_ENCODERS; ++i) {
    bool encoderChanged = false;
    int thisEncoderValue = g_encoderValues[i];
    if (thisEncoderValue != lastEncoderValues[i]) {
      encoderChanged = true;
      lastEncoderChangeMs[i] = now;
      encoderValueDeltas[i] += (thisEncoderValue - lastEncoderValues[i]);
      lastEncoderValues[i] = thisEncoderValue;
    }

    if (!encoderActive[i]) {
      if (!encoderChanged && now - lastEncoderChangeMs[i] > SPIN_INACTIVITY_TIMEOUT_MS) {
        lastEncoderInactiveMs[i] = now;
      }
      else {
        if (now - lastEncoderInactiveMs[i] > SPIN_ACTIVITY_DETECT_MS) {
          encoderActive[i] = true;
          for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
            lastEncoderValues[i] = g_encoderValues[i];
          }
        }
      }
    }
    else {
      if (!encoderChanged) {
        if (now - lastEncoderChangeMs[i] > SPIN_INACTIVITY_TIMEOUT_MS) {
          encoderActive[i] = false;
        }
      }
    }
  }

  bool anyEncoderActive = false;
  for (int i = 0; i < NUM_ENCODERS; anyEncoderActive |= encoderActive[i++]);
  digitalWrite(ENCODER_ACTIVE_PIN, anyEncoderActive);

  if (now >= nextXmitMs) {
    long actualIntervalMs = now - nextXmitMs + XMIT_INTERVAL_MS;
    nextXmitMs = now + XMIT_INTERVAL_MS;
    for (int i = 0; i < NUM_ENCODERS; ++i) {
      int32_t encoderStepsPerSecond = 0;
      int32_t encoderRpm = 0;
      if (encoderValueDeltas[i] != 0) {
        encoderStepsPerSecond = encoderValueDeltas[i] * 1000L / actualIntervalMs;
        encoderRpm = encoderStepsPerSecond * 60L / NUM_STEPS_PER_REV;
      }
      Serial.print(i);
      Serial.print(",");
      Serial.print(encoderActive[i]);
      Serial.print(",");
      Serial.print(actualIntervalMs);
      Serial.print(",");
      Serial.print(g_encoderValues[i]);
      Serial.print(",");
      Serial.print(encoderValueDeltas[i]);
      Serial.print(",");
      Serial.print(encoderStepsPerSecond);
      Serial.print(",");
      Serial.println(encoderRpm);
      encoderValueDeltas[i] = 0;
    }
  }

}


