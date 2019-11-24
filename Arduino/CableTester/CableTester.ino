
/***********
 * Options *
 ***********/

//#define ENABLE_DEBUG_PRINT


/************
 * Includes *
 ************/

#include <RBD_Button.h>


/*****************
 * Configuration *
 *****************/

constexpr uint8_t numCableSignals = 8;
constexpr uint8_t pinCableOut[numCableSignals] = {21, 22, 23, 24, 25, 26, 27, 28};
constexpr uint8_t pinCableIn[numCableSignals] = {31, 32, 33, 34, 35, 36, 37, 38};

constexpr uint8_t pinSwStartTest = 39;
constexpr bool    pullupSwStartTest = true;

constexpr uint8_t pinLedTestRunning  = LED_BUILTIN;
constexpr uint8_t ledTestRunningOn = HIGH;
constexpr uint8_t ledTestRunningOff = LOW;

constexpr uint8_t pinLedBad = 29;
constexpr uint8_t pinLedOk  = 30;
constexpr uint8_t ledOn = LOW;
constexpr uint8_t ledOff = HIGH;


/***********************
 * Types and Constants *
 ***********************/

enum class OperatingState {
  IDLE,
  RUN_TEST_START,
  RUN_TEST_SET_SIGNAL,
  RUN_TEST_READ_SIGNAL,
  RUN_TEST_STOP
};


/***********
 * Globals *
 ***********/

static OperatingState opState = OperatingState::IDLE;

static RBD::Button swStartTest(pinSwStartTest, pullupSwStartTest);


/***********
 * Helpers *
 ***********/


/******************
 * Initialization *
 ******************/

void setup()
{
  Serial.begin(9600);

  for (uint8_t i = 0; i < numCableSignals; ++i) {
    pinMode(pinCableIn[i], INPUT);
    pinMode(pinCableOut[i], OUTPUT);
    digitalWrite(pinCableOut[i], LOW);
  }

  pinMode(pinLedTestRunning, OUTPUT);
  pinMode(pinLedOk, OUTPUT);
  pinMode(pinLedBad, OUTPUT);

  digitalWrite(pinLedTestRunning, ledTestRunningOff);
  digitalWrite(pinLedOk, ledOff);
  digitalWrite(pinLedBad, ledOff);
}


/************
 * Run Loop *
 ************/

void loop()
{
  static uint8_t activeSignal;
  static bool cableOpen;
  static bool cableShorted;

  uint32_t now = millis();

  switch(opState) {

    case OperatingState::IDLE:
      if (swStartTest.onPressed()) {
        opState = OperatingState::RUN_TEST_START;
      }
      break;

    case OperatingState::RUN_TEST_START:
      cableOpen = cableShorted = false;
      activeSignal = 0;
      digitalWrite(pinLedTestRunning, ledTestRunningOn);
      digitalWrite(pinLedOk, ledOff);
      digitalWrite(pinLedBad, ledOff);
      for (uint8_t i = 0; i < numCableSignals; ++i) {
        digitalWrite(pinCableOut[i], LOW);
      }
      opState = OperatingState::RUN_TEST_SET_SIGNAL;
      break;

    case OperatingState::RUN_TEST_SET_SIGNAL:
      if (swStartTest.onPressed()) {
        // TODO:  turn on ok and bad if test didn't run long enough to check all signals
        opState = OperatingState::RUN_TEST_STOP;
        break;
      }
      digitalWrite(pinCableOut[activeSignal], LOW);
      if (++activeSignal >= numCableSignals) {
        activeSignal = 0;
      }
      digitalWrite(pinCableOut[activeSignal], HIGH);
      opState = OperatingState::RUN_TEST_READ_SIGNAL;
      break;

    case OperatingState::RUN_TEST_READ_SIGNAL:
      for (uint8_t i = 0; i < numCableSignals; ++i) {
        uint8_t cableSignalIn = digitalRead(pinCableIn[i]);
        if (i == activeSignal) {
          if (cableSignalIn != HIGH) {
            cableOpen = true;
          }
        }
        else {
          if (cableSignalIn != LOW) {
            cableShorted = true;
          }
        }
      }
      if (cableOpen || cableShorted) {
        opState = OperatingState::RUN_TEST_STOP;
      }
      else {
        opState = OperatingState::RUN_TEST_SET_SIGNAL;
      }
      break;

    case OperatingState::RUN_TEST_STOP:
        digitalWrite(pinLedTestRunning, ledTestRunningOff);
        if (cableOpen || cableShorted) {
          digitalWrite(pinLedBad, ledOn);
        }
        else {
          digitalWrite(pinLedOk, ledOn);
        }
        opState = OperatingState::IDLE;
      break;
  }
}
