/**
 * AsyncClient.cpp
 * 非同期Httpクライアント 実装ファイル
 */

#include "AsyncClient.h"

AsyncClient::AsyncClient()
	:HTTPClient()
{
  
}

AsyncClient::~AsyncClient()
{
}

int AsyncClient::AsyncGET()
{
  // connect to server
  if (!connect()) {
    return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
  }
  // send Header
  if(!sendHeader("GET")) {
    return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
  }
  _returnCode = -1;
  _size = -1;
  _transferEncoding = HTTPC_TE_IDENTITY;
  _lastDataTime =  millis();
  return 0;
}

int AsyncClient::handleClientLoop()
{
    if(!connected()) {
        return HTTPC_ERROR_NOT_CONNECTED;
    }

    if (connected()) {
        size_t len = _tcp->available();
        if(len > 0) {
            String headerLine = _tcp->readStringUntil('\n');
            headerLine.trim(); // remove \r
            _lastDataTime = millis();
            DEBUG_HTTPCLIENT("[HTTP-Client][handleClientLoop] RX: '%s'\n", headerLine.c_str());

            if(headerLine.startsWith("HTTP/1.")) {
                _returnCode = headerLine.substring(9, headerLine.indexOf(' ', 9)).toInt();
            } else if(headerLine.indexOf(':')) {
                String headerName = headerLine.substring(0, headerLine.indexOf(':'));
                String headerValue = headerLine.substring(headerLine.indexOf(':') + 1);
                headerValue.trim();

                if(headerName.equalsIgnoreCase("Content-Length")) {
                    _size = headerValue.toInt();
                }

                if(headerName.equalsIgnoreCase("Connection")) {
                    _canReuse = headerValue.equalsIgnoreCase("keep-alive");
                }

                if(headerName.equalsIgnoreCase("Transfer-Encoding")) {
                    _Encoding = headerValue;
                }

                for(size_t i = 0; i < _headerKeysCount; i++) {
                    if(_currentHeaders[i].key.equalsIgnoreCase(headerName)) {
                        _currentHeaders[i].value = headerValue;
                        break;
                    }
                }
            }

            if(headerLine == "") {
                DEBUG_HTTPCLIENT("[HTTP-Client][handleClientLoop] code: %d\n", _returnCode);

                if(_size > 0) {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleClientLoop] size: %d\n", _size);
                }

                if(_Encoding.length() > 0) {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleClientLoop] Transfer-Encoding: %s\n", _Encoding.c_str());
                    if(_Encoding.equalsIgnoreCase("chunked")) {
                        _transferEncoding = HTTPC_TE_CHUNKED;
                    } else {
                        return HTTPC_ERROR_ENCODING;
                    }
                } else {
                    _transferEncoding = HTTPC_TE_IDENTITY;
                }

                if(_returnCode) {
                    return _returnCode;
                } else {
                    DEBUG_HTTPCLIENT("[HTTP-Client][handleClientLoop] Remote host is not an HTTP Server!");
                    return HTTPC_ERROR_NO_HTTP_SERVER;
                }
            }

        } else {
            if((millis() - _lastDataTime) > _tcpTimeout) {
                return HTTPC_ERROR_READ_TIMEOUT;
            }
        }
        return 0;
    }

    return HTTPC_ERROR_CONNECTION_LOST;
}
