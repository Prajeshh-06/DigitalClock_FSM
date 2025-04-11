#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

// Initialize RTC and LCD
RTC_DS1307 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define buttons on the LCD keypad
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

// Define the frequency of the alarm tone (in Hz)
int toneFrequency = 1000;  // 1000 Hz for the alarm tone
int toneDuration = 100;    // Each beep duration (ms)
int totalDuration = 5000;  // Total duration of the alarm (5 seconds)

// Alarm variables
int alarmHour = 0, alarmMinute = 0;
bool alarmSet = false;
int selectedValue = 0;  // 0: Hours, 1: Minutes

// Stopwatch variables
unsigned long stopwatchStartTime = 0;
unsigned long stopwatchElapsedTime = 0;
bool stopwatchRunning = false;

// Timer variables
unsigned long timerStartTime = 0;
unsigned long timerElapsedTime = 0;
unsigned long timerDuration = 0;
bool timerRunning = false;

// Buzzer pin
const int buzzerPin = 12;  // Connect buzzer to pin 12

// Define FSM states
enum State {NORMAL, SET_TIME, SET_ALARM, STOPWATCH, SET_TIMER};
State currentState = NORMAL;

// Read buttons
int read_LCD_buttons() {
  int adc_key_in = analogRead(0); // Read input from A0
  
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 200)  return btnUP;
  if (adc_key_in < 400)  return btnDOWN;
  if (adc_key_in < 600)  return btnLEFT;
  if (adc_key_in < 800)  return btnSELECT;
  
  return btnNONE;
}

// Function to print time in HH:MM:SS format
void printTime(int h, int m, int s) {
  if (h < 10) lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if (m < 10) lcd.print("0");
  lcd.print(m);
  if (s >= 0) {
    lcd.print(":");
    if (s < 10) lcd.print("0");
    lcd.print(s);
  }
}

// Function to display stopwatch time
void printStopwatchTime(unsigned long elapsedTime) {
  unsigned long seconds = (elapsedTime / 1000) % 60;
  unsigned long minutes = (elapsedTime / 60000) % 60;
  unsigned long hours = (elapsedTime / 3600000);
  
  printTime(hours, minutes, seconds);
}

// Function to display timer time
void printTimerTime(unsigned long elapsedTime) {
  unsigned long seconds = (elapsedTime / 1000) % 60;
  unsigned long minutes = (elapsedTime / 60000) % 60;
  unsigned long hours = (elapsedTime / 3600000);
  
  printTime(hours, minutes, seconds);
}

void setup() {
  lcd.begin(16, 2);
  Wire.begin();

  if (!rtc.begin()) {
    lcd.print("RTC Error!");
    while (1);
  }

  rtc.adjust(DateTime(__DATE__, __TIME__));
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
}

void loop() {
  static DateTime now = rtc.now();
  int button = read_LCD_buttons();

  switch (currentState) {
    case NORMAL:
      now = rtc.now();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      printTime(now.hour(), now.minute(), now.second());
      lcd.setCursor(0, 1);
      lcd.print("Alarm: ");
      if (alarmSet) {
        printTime(alarmHour, alarmMinute, -1);
      } else {
        lcd.print("--:--");
      }
      if (alarmSet && now.hour() == alarmHour && now.minute() == alarmMinute && now.second() <= 5) {
        tone(buzzerPin, toneFrequency);
        delay(toneDuration);
        noTone(buzzerPin);
        delay(toneDuration);
      } else {
        digitalWrite(buzzerPin, LOW);
      }
      if (button == btnSELECT) currentState = SET_TIME;
      if (button == btnUP) currentState = STOPWATCH;
      if (button == btnDOWN) currentState = SET_TIMER;
      break;
    
    case SET_TIME:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Time:");
      lcd.setCursor(0, 1);
      lcd.print(selectedValue == 0 ? ">Hour:" : " Hour:");
      lcd.print(now.hour());
      lcd.print(" ");
      lcd.print(selectedValue == 1 ? ">Min:" : " Min:");
      lcd.print(now.minute());
      if (button == btnUP) {
        if (selectedValue == 0) now = DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute(), now.second());
        else now = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute() + 1, now.second());
      }
      if (button == btnDOWN) {
        if (selectedValue == 0) now = DateTime(now.year(), now.month(), now.day(), now.hour() - 1, now.minute(), now.second());
        else now = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute() - 1, now.second());
      }
      if (button == btnLEFT || button == btnRIGHT) selectedValue = !selectedValue;
      if (button == btnSELECT) {
        rtc.adjust(now);
        currentState = SET_ALARM;
      }
      break;
    
    case SET_ALARM:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Alarm:");
      lcd.setCursor(0, 1);
      lcd.print(selectedValue == 0 ? ">Hour:" : " Hour:");
      lcd.print(alarmHour);
      lcd.print(" ");
      lcd.print(selectedValue == 1 ? ">Min:" : " Min:");
      lcd.print(alarmMinute);
      if (button == btnUP) {
        if (selectedValue == 0) alarmHour = (alarmHour + 1) % 24;
        else alarmMinute = (alarmMinute + 1) % 60;
      }
      if (button == btnDOWN) {
        if (selectedValue == 0) alarmHour = (alarmHour + 23) % 24;
        else alarmMinute = (alarmMinute + 59) % 60;
      }
      if (button == btnLEFT || button == btnRIGHT) selectedValue = !selectedValue;
      if (button == btnSELECT) {
        alarmSet = true;
        currentState = NORMAL;
      }
      break;

    case STOPWATCH:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stopwatch:");
      lcd.setCursor(0, 1);
      if (stopwatchRunning) {
        stopwatchElapsedTime = millis() - stopwatchStartTime;
        printStopwatchTime(stopwatchElapsedTime);
      } else {
        printStopwatchTime(stopwatchElapsedTime);
      }

      if (button == btnSELECT) {
        if (stopwatchRunning) {
          stopwatchRunning = false;
        } else {
          stopwatchStartTime = millis() - stopwatchElapsedTime;
          stopwatchRunning = true;
        }
      }
      if (button == btnUP) {
        stopwatchElapsedTime = 0;  // Reset stopwatch
      }
      if (button == btnDOWN) {
        currentState = NORMAL;  // Return to normal mode
      }
      break;

    case SET_TIMER:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Timer:");
      lcd.setCursor(0, 1);
      lcd.print(selectedValue == 0 ? ">Hour:" : " Hour:");
      lcd.print(timerDuration / 3600000);
      lcd.print(" ");
      lcd.print(selectedValue == 1 ? ">Min:" : " Min:");
      lcd.print((timerDuration / 60000) % 60);
      if (button == btnUP) {
        if (selectedValue == 0) timerDuration += 3600000;  // Add 1 hour
        else timerDuration += 60000;  // Add 1 minute
      }
      if (button == btnDOWN) {
        if (selectedValue == 0) timerDuration -= 3600000;  // Subtract 1 hour
        else timerDuration -= 60000;  // Subtract 1 minute
      }
      if (button == btnLEFT || button == btnRIGHT) selectedValue = !selectedValue;
      if (button == btnSELECT) {
        timerStartTime = millis();
        timerRunning = true;
        currentState = NORMAL;
      }
      break;
  }

  // Timer countdown logic
  if (timerRunning) {
    timerElapsedTime = millis() - timerStartTime;
    if (timerElapsedTime >= timerDuration) {
      tone(buzzerPin, toneFrequency, toneDuration);
      delay(500);
      tone(buzzerPin, toneFrequency, toneDuration);
      delay(500);
      tone(buzzerPin, toneFrequency, toneDuration);
      delay(500);  // Tone duration
      noTone(buzzerPin);  // Stop tone
      timerRunning = false;  // Stop timer after time is up
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timer:");
      lcd.setCursor(0, 1);
      printTimerTime(timerDuration - timerElapsedTime);
    }
  }

  delay(200);
}
