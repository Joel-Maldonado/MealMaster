#include <Elegoo_GFX.h>     // Core graphics library
#include <Elegoo_TFTLCD.h>  // Hardware-specific library
#include <TouchScreen.h>
#include "Servo.h"

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

#define YP A3  // analog pin
#define XM A2  // analog pin
#define YM 9   // can be digital pin
#define XP 8   // can be digital pin

#define TS_MINX 74
#define TS_MINY 106

#define TS_MAXX 925
#define TS_MAXY 946

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Assign names to common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GRAY 0xCCCCCC


Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


Servo servo;

#define SERVO_PIN 29
// - A pulse width of 1ms causes the servo to rotate forward at its fastest speed.
// - Increasing the pulse width from 1ms to 1.5ms decreases the forward speed, with 1.5ms causing the servo to stop.
// - A pulse width of 1.5ms to 2ms causes the servo to rotate backward, with speeds increasing as the pulse width approaches 2ms.

const float SECS_FOR_DEGREE = 0.006;  // Retrieved from servo specifications

enum Page {
  HOME,
  EDIT_TIMES,
  HIDDEN,
};

Page page;


// Time slots
int hour[3] = { 9, 12, 6 };
int minute[3] = { 30, 0, 50 };
int isPM[3] = { 1, -1, -1 };

bool buttonsActive = true;

void setup(void) {
  Serial.begin(9600);
  servo.attach(SERVO_PIN);  // attach the servo
  tft.reset();

  uint16_t identifier = tft.readID();
  if (identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if (identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if (identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  } else if (identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if (identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if (identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if (identifier == 0x0101) {
    identifier = 0x9341;
    Serial.println(F("Found 0x9341 LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier = 0x9341;
  }

  tft.begin(identifier);
  tft.setRotation(1);  // Or 1
  // HiddenPage();
  HomePage();
}


void loop() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    // p.x and p.y seem to be switched?!
    int px = map(p.y, TS_MINX, TS_MAXX, tft.width(), 0);
    int py = tft.height() - map(p.x, TS_MINY, TS_MAXY, tft.height(), 0);

    if (page == Page::HIDDEN) {
      HomePage();
    }

    // (tft.width() - 200)/2,170, 200, 50)
    else if (page == Page::HOME) {
      if (px >= (tft.width() - 200) / 2 && px <= (tft.width() - 200) / 2 + 200) {
        // Check Dispense Now Button
        if ((tft.height() - py) >= 110 && (tft.height() - py) <= 160) {
          Serial.println("DISPENSING");
          rotateCounterClockwiseDeg(50, 2);
        }

        // Check EDIT Times Button
        else if ((tft.height() - py) >= 170 && (tft.height() - py) <= 220) {
          Serial.println("EDIT TIMES");
          EditTimesPage();
        }
      }
    }


    else if (page == Page::EDIT_TIMES) {
      // tft.fillRoundRect((tft.width() - 180)/2, 175, 190, 40, 8, BLACK);
      if (px >= (tft.width() - 180) / 2 && px <= (tft.width() - 180) / 2 + 180) {
        // Check Back Button
        if ((tft.height() - py) >= 175 && (tft.height() - py) <= 175 + 40) {
          HomePage();
          return;
        }
      }

      HandleEditTimes(px, py);
    }
  }
}

void HomePage() {
  page = Page::HOME;
  tft.fillScreen(WHITE);

  // Dispensing Time (TIME)
  String nextDispensingTime = "09:30 PM";
  tft.setCursor((tft.width() - nextDispensingTime.length() * 4 * 6) / 2, 30);
  tft.setTextColor(BLACK);
  tft.setTextSize(4);
  tft.print(nextDispensingTime);

  String nextDispensingTimeCaption = "Next Dispensing Time";
  tft.setCursor((tft.width() - nextDispensingTimeCaption.length() * 2 * 6) / 2, 70);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print(nextDispensingTimeCaption);

  // Dispense Now Button
  String dispenseNowText = "Dispense Now";
  // tft.fillRect((tft.width() - 200)/2,110, 200, 50, BLACK);
  tft.fillRoundRect((tft.width() - 200) / 2, 110, 200, 50, 12, BLACK);
  tft.setCursor((tft.width() - dispenseNowText.length() * 2 * 6) / 2, (110 + 50 / 2) - 8);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print(dispenseNowText);

  // Edit Time Button
  String editTimeText = "Edit Times";
  // tft.fillRect((tft.width() - 200)/2,170, 200, 50, BLACK);
  tft.fillRoundRect((tft.width() - 200) / 2, 170, 200, 50, 12, BLACK);
  tft.setCursor((tft.width() - editTimeText.length() * 2 * 6) / 2, (170 + 50 / 2) - 8);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print(editTimeText);
}

void EditTimesPage() {
  page = Page::EDIT_TIMES;

  tft.fillScreen(WHITE);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);

  char buff[10];
  String time;

  // Draw time slots with + and - buttons
  for (int i = 0; i < 3; i++) {
    strcpy(buff, "");
    sprintf(buff, "%02d:%02d", hour[i], minute[i]);
    Serial.println(buff);

    time = String(buff);


    if (isPM[i] == 1) {
      time += " PM";
    } else if (isPM[i] == 0) {
      time += " AM";
    } else {
      time = "--:-- --";
    }

    tft.drawRect((tft.width() - time.length() * 4 * 6) / 2 - 10, 25 + i * 50 - 10, 44 + 20, 30 + 20, BLACK);
    tft.drawRect((tft.width() - time.length() * 4 * 6) / 2 + 72 - 10, 25 + i * 50 - 10, 44 + 20, 30 + 20, BLACK);
    tft.drawRect((tft.width() - time.length() * 4 * 6) / 2 + 143 - 10, 25 + i * 50 - 10, 44 + 20, 30 + 20, BLACK);

    tft.setCursor((tft.width() - time.length() * 4 * 6) / 2, 25 + i * 50);
    tft.setTextColor(BLACK);
    tft.setTextSize(4);
    tft.print(time);

    Serial.println(time);

    // delay(100);
  }

  // Draw back button
  String backText = "Back";
  tft.fillRoundRect((tft.width() - 180) / 2, 175, 190, 40, 8, BLACK);
  tft.setCursor((tft.width() - backText.length() * 3 * 6) / 2 + 5, (175 + 40 / 2) - 10);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.print(backText);
}

void HandleEditTimes(int px, int py) {
  Serial.println("HANDLING");

  for (int i = 0; i < 3; i++) {
    if ((tft.height() - py) >= 25 + i * 50 - 10 && (tft.height() - py) <= 25 + i * 50 + 30 + 10) {
      if (px >= (tft.width() - 8 * 4 * 6) / 2 - 10 && px <= (tft.width() - 8 * 4 * 6) / 2 + 44 + 10) {
        hour[i] = hour[i] + 1;
      }

      if (px >= (tft.width() - 8 * 4 * 6) / 2 + 72 - 10 && px <= (tft.width() - 8 * 4 * 6) / 2 + 72 + 44 + 10) {
        minute[i] = minute[i] + 10;
      }

      if (px >= (tft.width() - 8 * 4 * 6) / 2 + 143 - 10 && px <= (tft.width() - 8 * 4 * 6) / 2 + 143 + 44 + 10) {
        isPM[i] += 1;
        if (isPM[i] == 2) {
          isPM[i] = -1;
        }
      }
    }

    if (minute[i] > 50) {
      minute[i] = 0;
    }

    if (hour[i] > 12) {
      hour[i] = 1;
    }
  }
  EditTimesPage();
}

void HiddenPage() {
  page = Page::HIDDEN;

  tft.fillScreen(BLACK);

  uint16_t pawColor = WHITE;

  // Main paw pad (centered and adjusted)
  tft.fillCircle(160, 120, 40, pawColor);  // Main pad

  // Top-left toe
  tft.fillCircle(120, 120, 20, pawColor);
  delay(200);

  // Top-right toe
  tft.fillCircle(200, 120, 20, pawColor);
  delay(200);

  // Bottom-left toe
  tft.fillCircle(140, 80, 15, pawColor);
  delay(200);

  // Bottom-right toe
  tft.fillCircle(180, 80, 15, pawColor);
  delay(200);
}



// ---------------- SERVO STUFF ----------------
void rotateClockwiseDeg(float degree, float speed) {
  // Start rotating at specified speed
  float microSeconds = 1500 - speed * 500;
  servo.write(microSeconds);

  // Stop at correct degree
  float stopTime_ms = SECS_FOR_DEGREE * degree * (1.0 / speed) * 1000.0;
  delay(stopTime_ms);

  // Stop
  servo.write(1500);
}

void rotateCounterClockwiseDeg(float degree, float speed) {
  // Start rotating at specified speed
  float microSeconds = 1500 + speed * 500;
  servo.write(microSeconds);

  // Stop at correct degree
  float stopTime_ms = SECS_FOR_DEGREE * degree * (1.0 / speed) * 1000.0;
  delay(stopTime_ms);

  // Stop
  servo.write(1500);
}
