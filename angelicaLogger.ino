/*
Groveの湿度センサとSparkFunのmicroSDカードシールドを使った簡易データロガー
2016-06-11 @shuzo_kino
参考もと
- http://qiita.com/kinu/items/6cd5da0415e31834e7da
- https://www.switch-science.com/catalog/814/
- http://arms22.blog91.fc2.com/blog-entry-502.html
*/

/*
 ##やってること##
 + 一定時間ごとに湿り気を取る
 + 湿り気値をフラグにして(watteState)、バルブを開放
 + 一定時間の始まり時にLEDを点滅させる
*/
  
#include <SD.h>

void blinkLED(int v);

File fds;
//ピン配置
#define chipSelect   8 //SparkFun は8
#define solenoidCtrl 7
const int errorLamp = 6; // エラーLED
const int pingLamp = 5; // 白色LED（死活監視）

//ステータス関連
#define interval 5 // 5秒 = 5 * 1000

// エラー監視時間
const unsigned long LOW_TIMER_LIMIT = 600000; // 10min
const unsigned long HIGH_TIMER_LIMIT = 86400000; // 24h

bool errorLampIsHigh = false;

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; } //動作待機
  
  // SSピンをアクティブ化しないとSDライブラリが動作しなくなります。
  pinMode(SS, OUTPUT);

  //ソレノイドバルブを制御。デフォではLOW
  pinMode(solenoidCtrl, OUTPUT);
  digitalWrite(solenoidCtrl, LOW);
  pinMode(errorLamp, OUTPUT);
  digitalWrite(errorLamp, LOW);
  pinMode(pingLamp, OUTPUT);
  digitalWrite(pingLamp, LOW);

  //見出し
  lineWrite();
  
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  //書込先ファイルの存在を確認
  if (SD.exists("data.txt")) {
    Serial.println("data.txt exists.");
  } else {
    Serial.println("data.txt doesn't exist.");
  }

  //見出し
  lineWrite();
}
  
void loop() {
/*
  //死活監視用LED。二回点滅する。
  blinkLED();
  // 規定時間毎に値を読みだして書込。
  unsigned char sensorValue = analogRead(A0);
  dataWrite(sensorValue);
  //乾いているようなら、開放。
  if (sensorValue < watteState) { openValve(); }
  delay(interval);
*/
  unsigned char sensorValue;
  unsigned long lowTimer;
  unsigned long highTimer;

  unsigned long closeTime = 5; // 水分閾値量が上回ってから、ディレイする時間（可変抵抗読み取り）
  int watteState = 50; // 水分閾値量（可変抵抗読み取り）

  while(true){
    // highタイマ計測開始／リセット
    highTimer = millis();
    while(true){
      //死活監視用LED。
      blinkLED(5);
      // ディレイ時間／水分閾値量を更新
      closeTime = analogRead(A1) * 6 / 10;
      watteState = analogRead(A2) / 2;
      variableValuePrint(closeTime, watteState);
      // しきい値チェック
      sensorValue = analogRead(A0);
      dataWrite(sensorValue);
      if(sensorValue < watteState){
        // バルブを開ける
        openValve();
        // 赤色LEDを消灯
        errorLedOff();
        // lowタイマ計測開始／リセット
        lowTimer = millis();
        while(true){
          //死活監視用LED。二回点滅する。
          blinkLED(5);
          // ディレイ時間／水分閾値量を更新
          closeTime = analogRead(A1) * 6 / 10;
          watteState = analogRead(A2) / 2;
          sensorValue = analogRead(A0);
          // しきい値チェック
          variableValuePrint(closeTime, watteState);
          dataWrite(sensorValue);
          if(sensorValue > watteState){
            // ディレイタイムだけ待つ
            int elapsedSeconds = 0;
            while(elapsedSeconds < closeTime){
              closeTime = analogRead(A1) * 6 / 10;
              variableValuePrint(closeTime, watteState);
              Serial.print("elapsed seconds: ");
              Serial.println(elapsedSeconds, DEC);
              elapsedSeconds++;
              blinkLED(1);
            }
            // バルブを閉める
            closeValve();
            // highタイマ計測開始／リセット
            highTimer = millis();
            break;
          }
          // lowタイマが10分以上経過していれば、エラー処理後、何もしない状態に遷移
          Serial.print("low timer: ");
          Serial.println((millis() - lowTimer), DEC);
          if((millis() - lowTimer) > LOW_TIMER_LIMIT){
            // エラーログ出力
            errorLog("low error");
            // 赤色LED常時点灯
            errorLed();
            // バルブを閉める
            closeValve();
            goto LOW_ERROR;
          }
          blinkLED(5);
        }
      }
      // highタイマが24時間以上経過していれば、エラー処理後、通常状態に遷移
      Serial.print("high timer: ");
      Serial.println((millis() - highTimer), DEC);
      if((millis() - highTimer) > HIGH_TIMER_LIMIT){
        // エラーログ出力
        errorLog("high error");
        // 赤色LED常時点灯
        errorLed();
        break;
      }
    }
  }

  LOW_ERROR:
  while(true){
    blinkLED(1);
  }
}

// 関数群
void lineWrite() {
  String line = "***********************";
  Serial.println(line);
  fds.println(line);
}
  
void dataWrite(int data) {
  // ファイルの書込みオープン
  // ファイルが無い場合は作成されます、あればファイルの最後に追加されます。
  fds = SD.open("data.txt",FILE_WRITE) ;
  if (fds) {
    String time    = String(millis(),DEC);
    String datastr = time + "," + String(data,DEC);
    //ファイルに書く
    fds.println(datastr);
    //シリアルに書く
    Serial.println(datastr);
    // ファイルのクローズ
    fds.close() ;
   } else {
    // ファイルのオープンエラー
    Serial.println("error opening") ;
   }
}
    
void openValve() {
  //バルブを開ける。
  //湿り気のフィードバックは取らず、一定時間HIGHにするのも
  digitalWrite(solenoidCtrl, HIGH);
  Serial.println("##OPEN VALVE##");
}

void closeValve() {
  //バルブを閉める
  digitalWrite(solenoidCtrl, LOW);
  Serial.println("##CLOSE VALVE##");
}

void stateLedOn(bool flag) {
  (flag == true) ? (digitalWrite(pingLamp, HIGH)) : (digitalWrite(pingLamp, LOW));
  delay(500);
}

void blinkLED(int count) {
  for(int i=0; i<count; i++){
    stateLedOn(true);
    stateLedOn(false);
  }
}

void errorLog(const char mes[]){
  fds = SD.open("data.txt",FILE_WRITE) ;
  if (fds) {
    //ファイルに書く
    fds.println(mes);
    //シリアルに書く
    Serial.println(mes);
    // ファイルのクローズ
    fds.close() ;
   } else {
    // ファイルのオープンエラー
    Serial.println("error opening") ;
   }
}

void errorLed(){
  errorLampIsHigh = true;
  digitalWrite(errorLamp, HIGH);
}

void errorLedOff(){
  errorLampIsHigh = false;
  digitalWrite(errorLamp, LOW);
}

void variableValuePrint(int closeTime, int watteState){
  Serial.print("close time: ");
  Serial.println(closeTime, DEC);
  Serial.print("watte state: ");
  Serial.println(watteState, DEC);
  Serial.print("error lamp is high?: ");
  Serial.println(errorLampIsHigh);
}

