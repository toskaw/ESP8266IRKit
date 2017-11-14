/**
 * AsyncClient.h
 * 非同期Httpクライアント
 */

#ifndef ASYNCCLIENT_H_
#define ASYNCCLIENT_H_
#include <ESP8266HTTPClient.h>

class AsyncClient : public HTTPClient
{
public:
	AsyncClient();
	virtual ~AsyncClient();

	// 非同期リクエスト
	int AsyncGET();

	// レスポンスチェック（Loopで毎回たたいてください）
	int handleClientLoop();
private:
  String _Encoding;
  unsigned long _lastDataTime;
};
#endif
