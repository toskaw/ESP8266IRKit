ESP8266IRKit
=====

IRKit Clone on ESP8266

## これは何

SWITCH SCIENCEのESPR IR 赤外線ボード<http://ssci.to/2740>でIRKit互換のAPIを実装しました。  
Android, ios のIRKit対応アプリでセットアップ、ボタン登録、操作が可能です。
ポートの変更等を行えば、ESP-WROOM-02の自作ボード等でも使用可能です。

## 手っ取り早く試したい人

コンパイル済みのバイナリーファイルもあっぷしました。
以下の手順で書き込めば手っ取り早く試せます。

必要なもの

* ESPR IR 赤外線ボード<http://ssci.to/2740>
* FTDI USBシリアル変換アダプタ<http://ssci.to/1032>
* Flash Download Tools

1. [binファイル](https://raw.githubusercontent.com/toskaw/ESP8266IRKit/master/ESP8266IRKit.ino.generic.bin)をダウンロード
2. [Flash Download Tools(ESP8266 & ESP32)](http://espressif.com/en/support/download/other-tools)をダウンロード
3. Flash Download Tools を7zip等で展開
4. 赤外線ボードとシリアル変換アダプタを接続してボードのジャンパピンをPROGに接続、USBケーブルをパソコンにさす
4. ESPFlashDownloadTool を起動
5. ESP8266 Download Tool ボタンを押す
6. ファイル、COMポートなどを設定し、STARTボタンを押す
7. FINISHが出たら、ボードのジャンパピンを外して、電源を入れなおす
![FlashDownloadTool](https://raw.githubusercontent.com/toskaw/ESP8266IRKit/master/flashdownload.jpg)


## 作り方（ソースからコンパイルする方法）
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
  
* const int send_pin = 14; 赤外線出力ポート
* const int recv_pin = 5;  赤外線入力ポート

## 使い方

電源を入れるとアクセスポイントになっています。

スマホのIRKit対応アプリで初期設定を行ってください。  
初期パスワードは0000000000です。

Wifi環境設定後は、ブラウザでアクセスすると簡易的なコンソールが使用できます。

IPアドレス、ホスト名はスマホアプリで確認できます。


## 簡易コンソールの使い方
![webconsole](https://raw.githubusercontent.com/toskaw/ESP8266IRKit/master/console.JPG)

* GET

GET /message動作です。

学習させるリモコンのボタンを押して、GETをクリックでパラメータを取得できます

* POST

POST /message動作です。text areaの内容を送信します

* IFTTT PARAM

text areaの内容でIFTTTのレシピに登録するパラメータを作成します。

* ケース

3Dプリンターで作成してみた。
ABSなので出力時に101%に拡大しているので注意。

![box](https://raw.githubusercontent.com/toskaw/ESP8266IRKit/master/box.jpg)

[3Dデータ/tinkercad](https://www.tinkercad.com/things/042WRSUgcFM)


## 参考にしたもの

* [minlRum](https://github.com/9SQ/minIRum)

* [IRKit](http://getirkit.com/)

