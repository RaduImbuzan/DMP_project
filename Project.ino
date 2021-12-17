#include <TimerOne.h>

#include "DHT.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define time_text_position_x 30
#define time_text_position_y 2

#define date_text_position_x 0
#define date_text_position_y 10

#define temperature_text_position_x 0
#define temperature_text_position_y 18

#define humidity_text_position_x 0
#define humidity_text_position_y 26

#define font_width_pixels 5
#define font_height_pixels 7

unsigned int seconds = 0, minutes = 44, hours = 15;
unsigned int lastSecond = 61;
char secondString[] = "00";
char minuteString[] = "00";
char hourString[] = "00";
char time[9];
char timeUpdateMask = 0;

unsigned int day = 0, month = 0, year = 0;
char dayString[] = "00";
char monthString[] = "00";
char yearString[] = "0000";
char date[11];

float currentTemperature, pastTemperature;
float currentHumidity, pastHumidity;

/// Buttons pins
const int setupPin = 3;
const int increaseMinutesPin = 4;
const int increaseHoursPin = 5;

int inSetupState = 0;
int increaseMinutesButtonState;
int lastIncreaseMinutesButtonState = LOW;

int increaseHoursButtonState;
int lastIncreaseHoursButtonState = LOW;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(incrementSeconds);

  attachInterrupt(digitalPinToInterrupt(setupPin), changeSetupState, RISING);

  pinMode(increaseMinutesPin, INPUT);
  pinMode(increaseHoursPin, INPUT);

  // Init the TFT display
  tft.initR(INITR_GREENTAB);
  tft.fillScreen(ST77XX_BLACK);

  dht.begin();

  intToString(hours, hourString);
  intToString(minutes, minuteString);
  intToString(seconds, secondString);

  sprintf(time, "%s:%s:%s", hourString, minuteString, secondString);
  sprintf(date, "%s/%s/%s", dayString, monthString, yearString);
}

void loop() {
  if (!inSetupState)
  {
    if (lastSecond != seconds) {
      displayTimeAndDate();
      displayTemperature();
      displayHumidity();

      lastSecond = seconds;
    }
  }
  else {
    updateIncreaseMinutesButtonState();
    updateIncreaseHourButtonState();
  }
}

void incrementSeconds() {
  timeUpdateMask = 0;

  seconds += 1;
  timeUpdateMask = timeUpdateMask | 1;

  if (seconds >= 60) {
    minutes++;
    timeUpdateMask = timeUpdateMask | 2;

    seconds = 0;

    if (minutes >= 60)
    {
      hours++;
      timeUpdateMask = timeUpdateMask | 4;

      minutes = 0;
      intToString(hours, hourString);
    }
    intToString(minutes, minuteString);
  }
  intToString(seconds, secondString);

  sprintf(time, "%s:%s:%s", hourString, minuteString, secondString);
}

char* intToString(int nr, char retString[])
{
  int index = 1;
  if (nr < 10)
  {
    retString[0] = '0';
    retString[index] = '0' + (nr % 10);
  }
  else {
    while (nr) {
      retString[index] = '0' + (nr % 10);
      nr = nr / 10;
      index--;
    }
  }
  retString[2] = '\0';
}

void clearCharPosition(int x0, int y0, int width, int height)
{
  tft.fillRect(x0, y0, width, height, ST77XX_BLACK);
}

void displayTimeAndDate() {
  // Clear the seconds position
  if (timeUpdateMask & 1)
  {
    clearCharPosition(time_text_position_x + 42, time_text_position_y, font_width_pixels, font_height_pixels);
    clearCharPosition(time_text_position_x + 36, time_text_position_y, font_width_pixels, font_height_pixels);
  }

  // Clear the minutes position
  if (timeUpdateMask & 2)
  {
    clearCharPosition(time_text_position_x + 24, time_text_position_y, font_width_pixels, font_height_pixels);
    clearCharPosition(time_text_position_x + 18, time_text_position_y, font_width_pixels, font_height_pixels);
  }

  // Clear the hour position
  if (timeUpdateMask & 4)
  {
    clearCharPosition(time_text_position_x + 6, time_text_position_y, font_width_pixels, font_height_pixels);
    clearCharPosition(time_text_position_x, time_text_position_y, font_width_pixels, font_height_pixels);
  }

  tft.setCursor(time_text_position_x, time_text_position_y);
  tft.print(time);

  tft.setCursor(date_text_position_x, date_text_position_y);
  tft.print(date);
}

void displayTemperature() {
  currentTemperature = dht.readTemperature();
  if (currentTemperature != pastTemperature) {
    pastTemperature = currentTemperature;

    // Clear the entire temperature line
    clearCharPosition(0, temperature_text_position_y, 128, font_height_pixels);

    tft.setCursor(temperature_text_position_x, temperature_text_position_y);
    tft.print(currentTemperature);
    tft.print(" C");
  }
}

void displayHumidity() {
  currentHumidity = dht.readHumidity();
  if (currentHumidity != pastHumidity)
  {
    pastHumidity = currentHumidity;

    // Clear the entire humidity line
    clearCharPosition(0, humidity_text_position_y, 128, font_height_pixels);

    tft.setCursor(humidity_text_position_x, humidity_text_position_y);
    tft.print(currentHumidity);
    tft.print("%");
  }
}

void changeSetupState() {
  inSetupState = (inSetupState + 1) % 2;

  // Reset the seconds
  seconds = -1;

  if (inSetupState == 1)
  {
    Timer1.stop();
  }
  else {
    Timer1.restart();
  }
}

void updateIncreaseMinutesButtonState() {
  int increaseMinutesButtonReading = digitalRead(increaseMinutesPin);

  if (increaseMinutesButtonReading != lastIncreaseMinutesButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (increaseMinutesButtonReading != increaseMinutesButtonState) {
      increaseMinutesButtonState = increaseMinutesButtonReading;

      if (increaseMinutesButtonState == HIGH) {
        minutes = (minutes + 1) % 60;

        intToString(minutes, minuteString);

        /// Print the new time on the display
        sprintf(time, "%s:%s:%s", hourString, minuteString, secondString);
        clearCharPosition(0, time_text_position_y, 128, font_height_pixels);
        tft.setCursor(time_text_position_x, time_text_position_y);
        tft.print(time);
      }
    }
  }

  lastIncreaseMinutesButtonState = increaseMinutesButtonReading;
}

void updateIncreaseHourButtonState() {
  int increaseHoursButtonReading = digitalRead(increaseHoursPin);

  if (increaseHoursButtonReading != lastIncreaseHoursButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (increaseHoursButtonReading != increaseHoursButtonState) {
      increaseHoursButtonState = increaseHoursButtonReading;

      if (increaseHoursButtonState == HIGH) {
        hours = (hours + 1) % 24;

        intToString(hours, hourString);

        // Print the new time on the display
        sprintf(time, "%s:%s:%s", hourString, minuteString, secondString);
        clearCharPosition(0, time_text_position_y, 128, font_height_pixels);
        tft.setCursor(time_text_position_x, time_text_position_y);
        tft.print(time);
      }
    }
  }

  lastIncreaseHoursButtonState = increaseHoursButtonReading;
}
