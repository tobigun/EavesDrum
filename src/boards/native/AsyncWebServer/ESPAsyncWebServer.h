/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  Copyright (c) 2025 Tobias Gunkel. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <functional>
#include <map>

class AsyncWebSocket;
class AsyncWebSocketClient;
class AsyncWebServer;
using AsyncStaticWebHandler = AsyncWebServer;
using AsyncWebServerRequest = AsyncWebServer;
using ArRequestHandlerFunction = std::function<void(AsyncWebServerRequest* request)>;

typedef enum {
  WS_EVT_CONNECT,
  WS_EVT_DISCONNECT,
  WS_EVT_PONG,
  WS_EVT_ERROR,
  WS_EVT_DATA
} AwsEventType;

typedef std::function<
    void(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)>
    AwsEventHandler;

class AsyncWebServer {
public:
  AsyncWebServer(int port)
    : _port(port) {}

  void begin();

  static void poll();

  void addHandler(AsyncWebSocket* webSocketServer);

  AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = nullptr) {
    // dummy
    return *this;
  }

  AsyncStaticWebHandler& setDefaultFile(const char* filename) {
    _defaultFilename = filename;
    return *this;
  }
  void onNotFound(ArRequestHandlerFunction onRequest) {
    // dummy
  }

  void send(int code, const char* content_type = "text/html", const String& content = "") {
    // dummy
  }

private:
  int _port;
  const char* _defaultFilename;

public:
  AsyncWebSocket* _webSocketServer;
};

class AsyncWebSocketClient {
public:
  AsyncWebSocketClient(void* connection)
    : _connection(connection) {}

  uint32_t id() const;
  void text(const String& t);
  bool canSend() { return true; }

  void binary(uint8_t* message, size_t len);

private:
  void* _connection;
};

class AsyncWebSocket {
public:
  AsyncWebSocket(const char* root)
    : _root(root) {}

  void addHandler(AsyncWebServer* webServer) {
    _webServer = webServer;
  }

  size_t count() {
    return _connections.size();
  }

  bool hasClient(int id) {
    return _connections.find(id) != _connections.end();
  }

  AsyncWebSocketClient*& client(int id) {
    return _connections[id];
  }

  void onEvent(AwsEventHandler handler) {
    _handler = handler;
  }
  
  void removeClient(int id) {
    _connections.erase(id);
  }
  
private:
  const char* _root;
  std::map<int, AsyncWebSocketClient*> _connections;

public:
  AwsEventHandler _handler;
  AsyncWebServer* _webServer;
};
