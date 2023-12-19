#include <EEPROM.h>
#include <TimerOne.h>
#include <GroveEncoder.h>
#include <TM1637.h>

#define MODE            7 // Switch
#define BUTTON_FOCUSING 6 // Button
#define DIAL            0 // Encoder
#define LED_FOCUS       2 // LED
#define LED_SHUTTER     3 // LED
#define SIGNAL_FOCUS    5 // LED
#define SIGNAL_SHUTTER  4 // LED
#define ON  true
#define OFF false

// 4-Digit Display http://wiki.seeed.cc/Grove-4-Digit_Display/
#define CLK 8
#define DIO 9
TM1637 tm1637(CLK, DIO);

// Encoder http://wiki.seeed.cc/Grove-Encoder/
GroveEncoder encoder(DIAL, NULL);

// タイマーの実行開始フラグ
boolean isTimerStarted = false;
// フォーカシング時間
volatile unsigned int focusingTime = 1;
// シャッターインターバル
unsigned int shutterIntervalTime = 2;
// フォーカシング時間設定用ボタンの状態
int buttonState = LOW;

// フォーカスボタン(シャッターボタン半押し)
void setFocusButton(boolean focusButton) {
  if (focusButton == ON) {
    digitalWrite(LED_FOCUS, HIGH);
    digitalWrite(SIGNAL_FOCUS, HIGH);
  } else {
    digitalWrite(LED_FOCUS, LOW);
    digitalWrite(SIGNAL_FOCUS, LOW);
  }
}

// シャッターボタン
void setShutterButton(boolean shutterButton) {
  if (shutterButton == ON) {
    digitalWrite(LED_SHUTTER, HIGH);
    digitalWrite(SIGNAL_SHUTTER, HIGH);
  } else {
    digitalWrite(LED_SHUTTER, LOW);
    digitalWrite(SIGNAL_SHUTTER, LOW);
  }
}

// Fire shutter
void shutter() {
  if (isTimerStarted && digitalRead(MODE) == HIGH) {
    setFocusButton(ON);
    for (int i = 1; i <= focusingTime * 10000; i++) {
      delayMicroseconds(100);
    }
    setShutterButton(ON);
    for (int i = 1; i <= 100; i++) {
      delayMicroseconds(100);
    }
    setShutterButton(OFF);
    setFocusButton(OFF);
  } else {
    Timer1.stop();
  }
}

void displayTime(unsigned int focusingTime, unsigned int shutterIntervalTime) {
  int8_t disp[] = { 0x00, 0x00, 0x00, 0x00 };
  disp[3] = shutterIntervalTime % 10;
  shutterIntervalTime /= 10;
  disp[2] = shutterIntervalTime % 10;
  shutterIntervalTime /= 10;
  disp[1] = shutterIntervalTime % 10;
  disp[0] = focusingTime % 10;
  tm1637.display(disp);
}

void setup() {
  Serial.begin(9600);
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.init();

  // ピン設定
  pinMode(MODE, INPUT);
  pinMode(BUTTON_FOCUSING, INPUT);
  pinMode(LED_FOCUS, OUTPUT);
  pinMode(LED_SHUTTER, OUTPUT);
  pinMode(SIGNAL_FOCUS, OUTPUT);
  pinMode(SIGNAL_SHUTTER, OUTPUT);

  isTimerStarted = false;
  focusingTime = 2;
  shutterIntervalTime = 4;
  displayTime(focusingTime, shutterIntervalTime);
}

void loop() {
  // スイッチHIGHで実行
  if (digitalRead(MODE) == HIGH) {
    // 実行中のダイヤル回転は値を上書きして無効にする
    encoder.setValue(shutterIntervalTime);

    // 実行開始
    if (isTimerStarted == false) {
      Timer1.stop();
      // 設定値はマイクロ秒のため10万倍する
      Timer1.initialize(shutterIntervalTime * 1000000);
      Timer1.attachInterrupt(shutter);
      Timer1.start();
      isTimerStarted = true;
    }
  } else {
    Timer1.stop();

    // 値の取得
    int newValue = encoder.getValue();

    // 値は1から999(秒)に制限する
    if (newValue > 999) {
      shutterIntervalTime = 999;
    } else if (newValue < 1) {
      shutterIntervalTime = 1;
    } else {
      shutterIntervalTime = newValue;
    }
    encoder.setValue(shutterIntervalTime);

    if (shutterIntervalTime <= focusingTime) {
      focusingTime = shutterIntervalTime - 1;
    }

    if (digitalRead(BUTTON_FOCUSING) == HIGH && buttonState == LOW) {
      focusingTime++;
      if (focusingTime >= shutterIntervalTime || focusingTime > 9) {
        focusingTime = 0;
      }
      delay(10);  //チャタリング防止
    }
    buttonState = digitalRead(BUTTON_FOCUSING);

    displayTime(focusingTime, shutterIntervalTime);
    isTimerStarted = false;
  }
}
