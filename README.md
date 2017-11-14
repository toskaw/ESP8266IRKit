ESP8266IRKit
=====

IRKit Clone on ESP8266

## これは何

SWITCH SCIENCEのESPR IR 赤外線ボード<http://ssci.to/2740>でIRKit互換のAPIを実装しました。  
Android, ios のIRKit対応アプリでセットアップ、ボタン登録、操作が可能です。
ポートの変更等を行えば、ESP-WROOM-02の自作ボード等でも使用可能です。

## 作り方
### 1.Arduino IDE環境をセットアップ  

  ここが詳しいです。<http://trac.switch-science.com/wiki/esp_dev_arduino_ide>

### 2.esp8266ライブラリを最新のものに更新 

  この辺を参考に2.4.0-rc1以上に更新してください。
  <https://www.mgo-tec.com/arduino-esp8266-latest-releases-howto>

  2.3.0はメモリーリークしているので数分で落ちます(^^;

### 3.依存ライブラリーをインストール

* [aJson](https://github.com/interactive-matter/aJson)
* [IRremoteESP8266](https://github.com/markszabo/IRremoteESP8266)

### 4.定数を変更

  環境に合わせてESP8266IRKit.inoの定数を変更してください。
  Espr IR赤外線ボードの場合は変更は必要ありません。
  
*const int send_pin = 14; 赤外線出力ポート
*const int recv_pin = 5;  赤外線入力ポート

## 使い方

電源を入れるとアクセスポイントになっています。

スマホのIRKit対応アプリで初期設定を行ってください。

Wifi環境設定後は、ブラウザでアクセスすると簡易的なコンソールが使用できます。

IPアドレス、ホスト名はスマホアプリで確認できます。

## 簡易コンソールの使い方
![webconsole](https://raw.githubusercontent.com/toskaw/ESP8266IRKit/master/console.JPG)

*GET

GET /message動作です。

学習させるリモコンのボタンを押して、GETをクリックでパラメータを取得できます

*POST

POST /message動作です。text areaの内容を送信します

*IFTTT PARAM

text areaの内容でIFTTTのレシピに登録するパラメータを作成します。


## 参考にしたもの

*[minlRum](https://github.com/9SQ/minIRum)

*[IRKit](http://getirkit.com/)

