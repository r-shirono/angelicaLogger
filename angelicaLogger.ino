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

File fds;
//ピン配置
#define chipSelect   8 //SparkFun は8
#define solenoidCtrl 7
const int errorLamp = 12; // エラーLED

//ステータス関連
#define interval   5000 //5秒 = 5 * 1000

// エラー監視時間
const unsigned long LOW_TIMER_LIMIT = 600000; // 10min
const unsigned long HIGH_TIMER_LIMIT = 86400000; // 24h

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

  int closeTime = 5000; // 水分閾値量が上回ってから、ディレイする時間（可変抵抗読み取り）
  int watteState = 50; // 水分閾値量（可変抵抗読み取り）

  while(true){
    // highタイマ計測開始／リセット
    highTimer = millis();
    while(true){
      //死活監視用LED。二回点滅する。
      blinkLED();
      // ディレイ時間／水分閾値量を更新
      //closeTime = analogRead(A1);
      //watteState = analogRead(A2);
      variableValuePrint(analogRead(A1), analogRead(A2));
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
          blinkLED();
          sensorValue = analogRead(A0);
          dataWrite(sensorValue);
          if(sensorValue > watteState){
            // ディレイタイムだけ待つ
            delay(closeTime);
            // バルブを閉める
            closeValve();
            break;
          }
          // lowタイマが5分以上経過していれば、エラー処理後、何もしない状態に遷移
          if((millis() - lowTimer) > LOW_TIMER_LIMIT){
            // エラーログ出力
            errorLog("low error");
            // 赤色LED常時点灯
            errorLed();
            goto LOW_ERROR;
          }
          delay(interval);
        }
      }
      // highタイマが24時間以上経過していれば、エラー処理後、通常状態に遷移
      if((millis() - highTimer) > HIGH_TIMER_LIMIT){
        // エラーログ出力
        errorLog("high error");
        // 赤色LED常時点灯
        errorLed();
        break;
      }
      delay(interval);
    }
  }

  LOW_ERROR:
  while(true){
    delay(1000);
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
  (flag == true) ? (digitalWrite(13, HIGH)) : (digitalWrite(13, LOW));
  delay(500);
}

void blinkLED() {
  stateLedOn(true);
  stateLedOn(false);
  stateLedOn(true);
  stateLedOn(false);
  stateLedOn(true);
  stateLedOn(false);
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
  digitalWrite(errorLamp, HIGH);
}

void errorLedOff(){
  digitalWrite(errorLamp, LOW);
}

void variableValuePrint(int closeTime, int watteState){
  Serial.print("close time: ");
  Serial.println(closeTime, DEC);
  Serial.print("watte state: ");
  Serial.println(watteState, DEC);
}

