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

//ステータス関連
#define interval   5000 //5秒 = 5 * 1000
#define watteState 50   //バルブ開放しきい値

// 内蔵時間のカウント。ミリ秒単位
unsigned long time;
  
void setup() {
  Serial.begin(9600);
  while (!Serial) { ; } //動作待機
  
  // SSピンをアクティブ化しないとSDライブラリが動作しなくなります。
  pinMode(SS, OUTPUT);

  //ソレノイドバルブを制御。デフォではLOW
  pinMode(solenoidCtrl, OUTPUT);
  digitalWrite(solenoidCtrl, LOW);

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
  //死活監視用LED。二回点滅する。
  blinkLED();
  // 規定時間毎に値を読みだして書込。
  unsigned char sensorValue = analogRead(A0);
  dataWrite(sensorValue);
  //乾いているようなら、開放。
  if (sensorValue < watteState) { openValve(); }
  delay(interval);
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
  delay(interval);
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

