/**************************************************************************
 Illumicone Controller System Manager Test Program

 Ross  31 Oct. 2021
 **************************************************************************/

#include <AStar32U4.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
// On a Pololu A-Star 32U4 Mini:  2(SDA),  3(SCL)
#define OLED_RESET       -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

constexpr uint8_t pin5vEn = 4;
constexpr uint8_t pinBatBu = 5;
constexpr uint8_t pinChgOn = 6;
constexpr uint8_t pinBatChg = 7;

constexpr uint8_t pinRpiGlobalEn = 8;
constexpr uint8_t pinBbbNSysReset = 9;

constexpr uint8_t pinVSysMon = A0;
constexpr uint8_t pinVBatMon = A1;
constexpr uint8_t pinV5MonMon = A2;

constexpr uint8_t relayOff = 0;
constexpr uint8_t relayOn = 1;


void setup() {

  pinMode(pin5vEn, OUTPUT);
  pinMode(pinBatBu, OUTPUT);
  pinMode(pinChgOn, OUTPUT);
  pinMode(pinBatChg, OUTPUT);

  pinMode(pinRpiGlobalEn, INPUT);   // high-z for now
  pinMode(pinBbbNSysReset, INPUT);  // high-z for now

  pinMode(pinVSysMon, INPUT);
  pinMode(pinVBatMon, INPUT);
  pinMode(pinV5MonMon, INPUT);

  digitalWrite(pin5vEn, relayOff);
  digitalWrite(pinBatBu, relayOff);
  digitalWrite(pinChgOn, relayOff);
  digitalWrite(pinBatChg, relayOff);

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  ledRed(0);
  ledYellow(0);
  ledGreen(0);

}

void loop() {

  static String statusText = "Start";

  ledGreen(1);

  digitalWrite(pin5vEn, relayOff);
  digitalWrite(pinBatBu, relayOff);
  digitalWrite(pinChgOn, relayOff);
  digitalWrite(pinBatChg, relayOff);


  uint16_t vSys = analogRead(pinVSysMon);
  uint16_t vBat = analogRead(pinVBatMon);
  uint16_t v5 = analogRead(pinV5MonMon);

  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.print(vSys);
  display.setCursor(0, 18);     // Start at top-left corner
  display.print(vBat);
  display.setCursor(64, 0);     // Start at top-left corner
  display.print(v5);
  display.setCursor(64, 18);     // Start at top-left corner
  display.print(statusText);
  display.display();

  delay(1000);
}
