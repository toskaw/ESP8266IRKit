#include "AsyncClient.h"
#include "IrPacker.h"
#include <EEPROM.h>

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRtimer.h>
#include <IRutils.h>
#include <ir_Argo.h>
#include <ir_Daikin.h>
#include <ir_Fujitsu.h>
#include <ir_Kelvinator.h>
#include <ir_LG.h>
#include <ir_Mitsubishi.h>
#include <ir_Trotec.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <aJSON.h>
#include <base64.h>
#define VERSION "3.1.0.3.esp8266"
#define HOST "http://deviceapi.getirkit.com"
#define RECV_TIMEOUT 100U
#define RECV_BUFF 1024
#define JSON_BUFF 4096
#define SHARED_BUFFER_SIZE 512

const int send_pin = 14;
const int recv_pin = 5;
const char WAIT_RESPONSE = 'a';
const char DONE = 'b';
static uint32_t newest_message_id = 0;
char localName[10] = "IRKit";

int polling = 0;
struct CONFIG {
  char init;
  char ssid[32];
  char pass[64];
  char devicekey[32];
};
struct irpacker_t packer_state;
CONFIG gSetting;
ESP8266WebServer webServer(80);
AsyncClient polling_client;
char sharedbuffer[ SHARED_BUFFER_SIZE ];

IRsend irsend(send_pin);
IRrecv irrecv(recv_pin, RECV_BUFF, RECV_TIMEOUT, true);

#define ERR_NOBODY -1
#define ERR_FORMAT -2
#define ERR_EMPTY -3
char json[JSON_BUFF];

int irsendMessage(String& req) {
  int ret = 0;
  if (req.length() == 0) return ERR_EMPTY;
  irrecv.disableIRIn();
  
  req.toCharArray(json, JSON_BUFF);
  Serial.println(json);
  aJsonObject* root = aJson.parse(json);
  if (root != NULL) {
    aJsonObject* freq = aJson.getObjectItem(root, "freq");
    aJsonObject* data = aJson.getObjectItem(root, "data");
    if (freq != NULL && data != NULL) {
      aJsonObject* node = data->child;
      if (node != NULL) {
        uint16_t d_size = 0;
        while (node) {
          d_size += 1;
          node = node->next;       
        }
        uint16_t rawData[d_size];
        int i = 0;
        node = data->child;
        while (node)  {
          rawData[i] = node->valueint;
          i += 1;
          node = node->next;
        }
        if (freq->type == aJson_Int) {
          irsend.sendRaw(rawData, d_size, (uint16_t)freq->valueint);
        }
        else if (freq->type == aJson_Float) {
          irsend.sendRaw(rawData, d_size, (uint16_t)freq->valuefloat);          
        }
        else {
          // 本当はエラーだけど38決め打ち
          irsend.sendRaw(rawData, d_size, 38);
        }
        Serial.println("irsend");
        // polling suspend
        polling = 1;
      }
      else {
        ret = ERR_FORMAT;
      }
      aJsonObject* id = aJson.getObjectItem(root, "id");
      if (id != NULL) {
        newest_message_id = id->valueint;
       }
    } else {
      ret =  ERR_FORMAT;
    }
    aJson.deleteItem(root);
  } else {
    ret = ERR_NOBODY;
  }
  irrecv.enableIRIn();
  return ret;  
}

void sendheader() {
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  String server = "IRkit/";
  server += VERSION;
  webServer.sendHeader("Server", server);
}

void handleMessages() {
  Serial.println(webServer.uri().c_str());
  sendheader();
  if (!webServer.hasHeader("X-Requested-With")) {
    webServer.send(400, "text/plain", "Invalid Request");
    return;
  }
  if (webServer.method() == HTTP_POST) {
    Serial.println("post");
    String req = webServer.arg(0);
    if (req != NULL) {
      int err = irsendMessage(req);
      if (err == 0) {
        webServer.send(200, "text/plain", "ok");
      }
      else if (err == ERR_FORMAT) {
        webServer.send(400, "text/plain", "Invalid JSON format");
      }
      else {
        webServer.send(400, "text/plain", "No Data");
      }
    }
    else {
      Serial.println("err");
      webServer.send(400, "text/plain", "No Data");      
    }
  }
  else if (webServer.method() == HTTP_GET) {
    String s = "{\"format\":\"raw\",\"freq\":38,\"data\":[";
    int length = irpacker_length(&packer_state);
    irpacker_unpack_start(&packer_state);
    for (int i= 0; i < length - 1; i += 1) {
      s += irpacker_unpack(&packer_state);
      s += ",";
    }
    s += irpacker_unpack(&packer_state);
    s += "]}";
    webServer.send(200, "text/plain", s);
  }
}

void handleKeys() {
  // devicekey=[0-9A-F]{32}
  if (!webServer.hasHeader("X-Requested-With")) {
    webServer.send(400, "text/plain", "Invalid Request");
    return;
  }
  char body[64];
  char url[64];
  sprintf(body, "devicekey=%s", gSetting.devicekey);
  HTTPClient client;
  sprintf(url, "%s%s", HOST, "/k");
  client.begin(url);
  client.setUserAgent("IRKit");
  client.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = client.POST((uint8_t*)body, strlen(body));
  if (code == HTTP_CODE_OK) {
    sendheader();
    webServer.send(200, "text/plain", client.getString()); 
  }
  client.end();
}
void handleIndex() {
  String s = "<html lang=\"en\"><head><meta charset=\"utf-8\"/><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>minIRum</title><link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/css/bootstrap.min.css\" /><script src=\"https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/js/bootstrap.min.js\"></script></head><body><div class=\"container\"><div class=\"row\"><div class=\"col-md-12\"><h1>minIRum console <small>";
  s += localName;
  s += ".local</small></h1><p>IP address: ";
  s += String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
  s += "</p><hr><div class=\"form-group\"><textarea class=\"form-control\" id=\"message\" rows=\"10\"></textarea></div><button class=\"btn btn-primary\" id=\"get\">GET</button> <button class=\"btn btn-success\" id=\"post\">POST</button> <button class=\"btn btn-info\" id=\"ifttt\">IFTTT PARAM</button></div>";
  s += "<script>var xhr = new XMLHttpRequest();var textarea = document.getElementById(\"message\");document.getElementById(\"get\").addEventListener(\"click\", function () {xhr.open('GET', '/messages', true);xhr.setRequestHeader('X-Requested-With', 'curl');xhr.onreadystatechange = function() {if(xhr.readyState == 4) {textarea.value =xhr.responseText;}};xhr.send(null);});";
  s += "document.getElementById(\"post\").addEventListener(\"click\", function () {data = textarea.value;xhr.open('POST', '/messages', true);xhr.onreadystatechange = function() {if(xhr.readyState == 4) {alert(xhr.responseText);}};xhr.setRequestHeader('Content-Type', 'application/json');xhr.setRequestHeader('X-Requested-With', 'curl');xhr.send(data);});";
  s += "document.getElementById(\"ifttt\").addEventListener(\"click\", function () {xhr.open('POST', '/keys', true);xhr.onreadystatechange = function() {if(xhr.readyState == 4) {data = JSON.parse(xhr.responseText);param = 'clienttoken=' + data.clienttoken; var xhr2 = new XMLHttpRequest(); xhr2.open('POST', 'https://api.getirkit.com/1/keys', true);xhr2.onreadystatechange = function() {if(xhr2.readyState == 4) {data=JSON.parse(xhr2.responseText);message = textarea.value;param = 'clientkey=' + data.clientkey + '&deviceid=' + data.deviceid +'&message=' + message; textarea.value = param;}};xhr2.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');xhr2.send(param);}};xhr.setRequestHeader('Content-Type', 'application/json');xhr.setRequestHeader('X-Requested-With', 'curl');xhr.send('');});";
  s += "</script></body></html>";
  sendheader();
  webServer.send(200, "text/html", s);
}

void handleNotFound() {
  Serial.println(webServer.uri().c_str());
  Serial.println("404");
  sendheader();
  webServer.send(404, "text/plain", "Not found.");
}

void handleReset() {
  sendheader();
  if (!webServer.hasHeader("X-Requested-With")) {
    webServer.send(400, "text/plain", "Invalid Request");
    return;
  }
  webServer.send(200, "text/plain", "Reboot.");
  webServer.close();
  gSetting.init = 0;
  Serial.println("reset");
  EEPROM.put<CONFIG>(0, gSetting);
  EEPROM.commit();
  EEPROM.end();

  // reset
  ESP.reset();
}

void decodeParam(String& param, char *buffer) {
  int length = param.length();
  char high = 0;
  char low = 0;
  for (int i = 0; length > i; i+=2) {
      high = param.charAt(i);
      if (high < 'A') {
        high = high - '0';
      }
      else {
        high = high - 'A' + 10;
      }
      high *= 16;
      low = param.charAt(i + 1);
      if (low < 'A') {
        low = low - '0';
      }
      else {
        low = low - 'A' + 10;
      }
      *buffer = (char)(high + low);
      buffer += 1;
  }
  *buffer = '\0';
}

void handleWifiPost() {
  if (!webServer.hasHeader("X-Requested-With")) {
    webServer.send(400, "text/plain", "Invalid Request");
    return;
  }

  String data = webServer.arg(0);
  Serial.println("wifi request");
  Serial.println(data.c_str());

  // decode ssid
  int index = data.indexOf('/');
  int end_index = data.indexOf('/', index + 1);
  String param = data.substring(index + 1, end_index);
  Serial.println(param.c_str());
  decodeParam(param, gSetting.ssid);
  Serial.println(gSetting.ssid);

  // decode password
  index = data.indexOf('/', end_index);
  end_index = data.indexOf('/', index + 1);
  param = data.substring(index + 1, end_index);
  Serial.println(param.c_str());
  decodeParam(param, gSetting.pass);
  Serial.println(gSetting.pass);

  // decode devicekey
  index = data.indexOf('/', end_index);
  end_index = data.indexOf('/', index + 1);
  param = data.substring(index + 1, end_index);
  strcpy(gSetting.devicekey, param.c_str());
  Serial.println(gSetting.devicekey);
  
  gSetting.init = WAIT_RESPONSE;
  EEPROM.put<CONFIG>(0, gSetting);
  EEPROM.commit();
  EEPROM.end();
  String html = "";
  html += "<h1>WiFi Settings</h1>";
  webServer.send(200, "text/html", html);
  webServer.close();
  // reset
  ESP.reset();
}

String dumpIRcode (decode_results *results) {
  String s = "";
  for (int i = 1;  i < results->rawlen;  i++) {
    s += results->rawbuf[i] * RAWTICK;
    if ( i < results->rawlen - 1 ) s += ",";
  }
  return s;
}

void APMode() {
    char* ssid;
    const char* password = "0000000000";

    ssid = localName;
    IPAddress local_IP(192,168,1,1);
    IPAddress subnet(255,255,255,0);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, local_IP, subnet);
    WiFi.softAP(ssid, password);
    delay(1000);
    Serial.println("");
    Serial.println("softAP Start");  
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    if (MDNS.begin(localName, WiFi.softAPIP())) {
      MDNS.addService("irkit", "tcp", 80);
      Serial.println("MDNS responder started.");
      Serial.print("http://");
      Serial.print(localName);
      Serial.println(".local");
    }
    webServer.on("/wifi", HTTP_POST, handleWifiPost);
}

#if 0
void dump(decode_results *results) {
  // Dumps out the decode_results structure.
  // Call this after IRrecv::decode()
  uint16_t count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } else if (results->decode_type == RC5X) {
    Serial.print("Decoded RC5X: ");
  } else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  } else if (results->decode_type == RCMM) {
    Serial.print("Decoded RCMM: ");
  } else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->address, HEX);
    Serial.print(" Value: ");
  } else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
  } else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  } else if (results->decode_type == AIWA_RC_T501) {
    Serial.print("Decoded AIWA RC T501: ");
  } else if (results->decode_type == WHYNTER) {
    Serial.print("Decoded Whynter: ");
  } else if (results->decode_type == NIKAI) {
    Serial.print("Decoded Nikai: ");
  }
  serialPrintUint64(results->value, 16);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): {");

  for (uint16_t i = 1; i < count; i++) {
    if (i % 100 == 0)
      yield();  // Preemptive yield every 100th entry to feed the WDT.
    if (i & 1) {
      Serial.print(results->rawbuf[i] * RAWTICK, DEC);
    } else {
      Serial.print(", ");
      Serial.print((uint32_t) results->rawbuf[i] * RAWTICK, DEC);
    }
  }
  Serial.println("};");
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  EEPROM.begin(200);
  EEPROM.get<CONFIG>(0, gSetting);
 
  byte mac[6];
  char buf[6];
  WiFi.macAddress(mac);
  sprintf(buf, "%02X%02X", mac[4], mac[5]);
  strcat(localName, buf);
  const char * headerkeys[] = {"X-Requested-With"};
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  webServer.collectHeaders(headerkeys, headerkeyssize );
  char* ssid;
  char* password;
  if (gSetting.init != DONE && gSetting.init != WAIT_RESPONSE) {
    APMode();
  }
  else {
    ssid = gSetting.ssid;
    password = gSetting.pass;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    irsend.begin();
    irrecv.enableIRIn();
    uint8 retry = 1000;
    while (WiFi.status() != WL_CONNECTED && retry) {
      delay(200);
      Serial.print(".");
      retry -= 1;
    }
    if (retry == 0 && gSetting.init == WAIT_RESPONSE) {
      // wifi error 
      gSetting.init = 0;
      EEPROM.put<CONFIG>(0, gSetting);
      EEPROM.commit();
      EEPROM.end();
      ESP.reset();
    }
    else if (retry == 0 && gSetting.init == DONE) {
      // 繋がらないのでとりあえずAPモード（設定は消さない)
      APMode();
    }
    else {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      if (MDNS.begin(localName, WiFi.localIP())) {
        //MDNS.addService("http", "tcp", 80);
        MDNS.addService("irkit", "tcp", 80);
        Serial.println("MDNS responder started.");
        Serial.print("http://");
        Serial.print(localName);
        Serial.println(".local");
      }
      webServer.on("/messages", handleMessages);
      webServer.on("/keys", handleKeys);
      webServer.on("/reset", handleReset);
 #if 1
      webServer.on("/", handleIndex);
 #endif
    }
  }
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  Serial.println("Web server started.");
}

void loop() {
  webServer.handleClient();

  static char url[256];
  if (gSetting.init == WAIT_RESPONSE) {
    // post_door
    #define POST_DOOR_BODY_LENGTH 128
    char body[POST_DOOR_BODY_LENGTH];
    sprintf(body, "devicekey=%s&hostname=%s", gSetting.devicekey, localName);
    HTTPClient client;
    sprintf(url, "%s%s", HOST, "/d");
    client.begin(url);
    Serial.println(url);
    Serial.println(body);
    client.setUserAgent("IRKit");
    client.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int code = client.POST((uint8_t*)body, strlen(body));
    if (code == HTTP_CODE_OK) { 
      gSetting.init = DONE;
      Serial.println("success");
      EEPROM.put<CONFIG>(0, gSetting);
      EEPROM.commit();
      EEPROM.end();
    }
    else {
      char buffer[10];
      sprintf(buffer, "%i", code);
      Serial.println(buffer);
    }
    client.end();
  }
  else if (gSetting.init == DONE) {
    // get messages
    sprintf(url, "%s/m?devicekey=%s&newer_than=%d", HOST, gSetting.devicekey, newest_message_id);
    if (polling_client.connected()) {
      int status = polling_client.handleClientLoop();
      if (status) {
        if (status == HTTP_CODE_OK) {
          String req = polling_client.getString();
          if (polling == 0 && req.length()) {
            irsendMessage(req);
          }
          else {
            polling = 0;
            if (req && req.length()) {
              // id 更新
              req.toCharArray(json, JSON_BUFF);  
              aJsonObject* root = aJson.parse(json);
              if (root != NULL) {
                aJsonObject* id = aJson.getObjectItem(root, "id");
                if (id != NULL) {
                  ::newest_message_id = id->valueint;
                }
                aJson.deleteItem(root);
              }
            }
          }
        }
        polling_client.end();
      }
    }
    else {
      polling_client.begin(url);
      polling_client.setTimeout(50 * 1000);
      polling_client.setUserAgent("IRKit");
      if (polling_client.AsyncGET() < 0) {
        // 切断
        polling_client.end();
      }
    }

    decode_results  results;
    
    if (irrecv.decode(&results)) {
      //dump(&results);
      // pack
      memset( (void*)sharedbuffer, 0, sizeof(uint8_t) * SHARED_BUFFER_SIZE );
      irpacker_init( &packer_state, (uint8_t*)sharedbuffer );
       for (int i = 1;  i < results.rawlen;  i++) {
        irpacker_pack(&packer_state, results.rawbuf[i] * RAWTICK);
       }
       irpacker_packend(&packer_state);
      // post
      String body;
      body = base64::encode(sharedbuffer, irpacker_length(&packer_state));
      sprintf(url, "%s/p?devicekey=%s&freq=%d", HOST, gSetting.devicekey, 38);
      HTTPClient client;
      client.setTimeout(10000);
      client.begin(url);
      client.setUserAgent("IRKit");
      client.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int code = client.POST(body);
      if (code == HTTP_CODE_OK) { 
        Serial.println("post success");
      }
      else {
        Serial.println("post error");
      }
      client.end();
      irrecv.resume();
    }
  

#if 0 
    long mem = ESP.getFreeHeap();
    Serial.print("freeMemory()=");
    Serial.println(mem);
#endif
  }
  else {
    ;
  }
}

