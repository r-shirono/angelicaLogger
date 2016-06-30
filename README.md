# 某案件向けSDカードデータロガー + バルブコントローラ

Groveの湿度センサとSparkFunのmicroSDカードシールドを使った簡易データロガー。

2016-06-11 @shuzo_kino

 ## やってること
 + 一定時間ごとに湿り気を取る
 + 湿り気値をフラグにして(watteState)、バルブを開放
 + 一定時間の始まり時にLEDを点滅させる

## 参考もと
### 技術面
-  [もう観葉植物を枯らさない！ Arduinoで鉢植えをしゃべらせる@qiita](http://qiita.com/kinu/items/6cd5da0415e31834e7da)
- [ArduinoでSDメモリカードを読み書きする @ なんでもつくっちゃうかも](http://arms22.blog91.fc2.com/blog-entry-502.html)
### 採用部品
- [バルブの商品ページ](https://www.switch-science.com/catalog/814/)

