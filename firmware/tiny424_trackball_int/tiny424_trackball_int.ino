// 1Uトラックボールユニットのホイールセンサーの入力をI2Cに流す

// 開発環境の作り方
// https://ameblo.jp/pta55/entry-12654450554.html

#include <Wire.h>
#include <avr/pgmspace.h>

// ATTiny202 本体のアドレス
#define I2C_SLAVE_ADD 0x0A

// PIN
#define PIN_HALL_POWER  PIN_PA7
#define PIN_HALL_LEFT   PIN_PA4
#define PIN_HALL_RIGHT  PIN_PA1
#define PIN_HALL_UP     PIN_PA5
#define PIN_HALL_DOWN   PIN_PA3
#define PIN_BUTTON      PIN_PA6
#define PIN_INT         PIN_PA2

// カウントの方式(0=通常 / 1=AZ用)
uint8_t count_type = 0;

// INT 設定フラグ
short int_flag;

// 送信バッファ
uint8_t send_input[5];

// 最後にホールセンサからデータを受け取ったミリ秒
unsigned long last_time[4];

// I2Cイベント
void receiveEvent(int data_len); // データを受け取った
void requestEvent(); // データ要求を受け取った

// ホールセンサー読み込み用割り込み関数
void attachLeft();
void attachRight();
void attachUp();
void attachDown();

void setup() {
    // ロータリーエンコーダ読み込みピンの初期化
    pinMode(PIN_HALL_LEFT, INPUT); // 左
    pinMode(PIN_HALL_RIGHT, INPUT); // 右
    pinMode(PIN_HALL_UP, INPUT); // 上
    pinMode(PIN_HALL_DOWN, INPUT); // 下
    pinMode(A6, INPUT_PULLUP); // 中央タクトスイッチ
    pinMode(PIN_HALL_POWER, OUTPUT); // ホールセンサー電源 ON/OFF
    pinMode(PIN_INT, OUTPUT); // INTピン

    // 出力ピン初期化
    digitalWrite(PIN_HALL_POWER, 0); // HALLセンサーON
    digitalWrite(PIN_INT, 0); // INT OFF

    // バッファの初期化
    memset(send_input, 0x00, 5);
    memset(last_time, 0x00, 4 * sizeof(unsigned long));

    // I2C スレーブ初期化
    Wire.begin(I2C_SLAVE_ADD);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    // ホールセンサーの回転健知時のイベント登録
    attachInterrupt(PIN_HALL_LEFT, attachLeft, CHANGE);
    attachInterrupt(PIN_HALL_RIGHT, attachRight, CHANGE);
    attachInterrupt(PIN_HALL_UP, attachUp, CHANGE);
    attachInterrupt(PIN_HALL_DOWN, attachDown, CHANGE);

    int_flag = 0;
}

// コマンドを受け取った
void receiveEvent(int data_len) {
  // 最初の1バイトだけ取得
  uint8_t d = Wire.read();
  if (d == 0x40) {
      // HALLセンサーOFF
      digitalWrite(PIN_HALL_POWER, 1); // HALLセンサーOFF
  } else  if (d == 0x41) {
      // HALLセンサーON
      digitalWrite(PIN_HALL_POWER, 0); // HALLセンサーON
  } else if (d == 0x90) {
    count_type = 0;
  } else if (d == 0x91) {
    count_type = 1;
  }
  // 残りは空読み
  while (Wire.available()) {
    Wire.read();
  }
}

// I2Cデータ要求を受け取った時の処理
void requestEvent() {
  // トラックボール操作の情報を送る
  Wire.write(send_input, 5);
  // バッファをリセット
  memset(send_input, 0x00, 4);
  // INTピン OFF
  if (int_flag) {
    digitalWrite(PIN_INT, 0); // INT OFF
    int_flag = 0;
  }
}

// ホールセンサーの回転を検知した時のイベント 0=左 / 1=右 / 2=上 / 3=下
void attachExec(int i) {
  unsigned long n = millis(); // 現在の時間
  unsigned long t = (n - last_time[i]); // 前回実行時からどれくらい時間が経ったか
  last_time[i] = n; // 最後に実行した時間を更新
  if (send_input[i] > 250) return; // 250超えをMAX値とする
  if (count_type == 0) {
    // 通常モードは常に１ずつ移動
    if (t > 1) {
      send_input[i] += 1;
    }
  } else if (count_type == 1) {
    // 経過時間別に移動距離加算
    if (t > 50) {
      send_input[i] += 1;
    } else if (t > 30) {
      send_input[i] += 2;
    } else if (t > 1) {
      send_input[i] += 3;
    }
  }
  // INTピン ON
  if (int_flag == 0) {
    digitalWrite(PIN_INT, 1); // INT ON
    int_flag = 1;
  }
}

// 左ホイール回転検知
void attachLeft() {
  attachExec(0);
}

// 右ホイール回転検知
void attachRight() {
  attachExec(1);
}

// 上ホイール回転検知
void attachUp() {
  attachExec(2);
}

// 下ホイール回転検知
void attachDown() {
  attachExec(3);
}

void loop() {
  // タクトスイッチの状態を送信バッファに反映
  if (!digitalRead(A6)) {
    send_input[4] = 0x80;
  } else {
    send_input[4] = 0x00;
  }
  delay(2);
}
