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

#define ARDUINO_NO_PINMODE
#include "ESPAsyncWebServer.h"

#include "mongoose.h"
#undef poll

// declare event manager globally as poll() is static
static struct mg_mgr manager;

static AsyncWebSocket* getWebSocket(struct mg_connection* connection) {
  AsyncWebServer* server = (AsyncWebServer*)connection->fn_data;
  AsyncWebSocket* socket = server->_webSocketServer;
  return socket;
}

static void logMessage(char c, void* param) {
  Serial.printf("Web: %c", c);
}

static void handleEvent(struct mg_connection* connection, int ev, void* ev_data) {
  if (ev == MG_EV_OPEN) {
    Serial.printf("Web: Connection %lu opened\n", connection->id);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
      // Upgrade to websocket. From now on, a connection is a full-duplex
      // Websocket connection, which will receive MG_EV_WS_MSG events.
      mg_ws_upgrade(connection, hm, NULL);
    } else if (mg_match(hm->uri, mg_str("/config.jsonc"), NULL) || mg_match(hm->uri, mg_str("/config.yaml"), NULL)) {
      const struct mg_http_serve_opts serveOpts = {.root_dir = "./config/"};
      mg_http_serve_dir(connection, hm, &serveOpts);
    } else { // Serve static files
      const struct mg_http_serve_opts serveOpts = {.root_dir = "./data/"};
      mg_http_serve_dir(connection, hm, &serveOpts);
    }
  } else if (ev == MG_EV_WS_MSG || ev == MG_EV_WS_OPEN) {
    AsyncWebSocket* socket = getWebSocket(connection);
    if (!socket->hasClient(connection->id)) {
      Serial.printf("Web: Connection %lu opened as websocket\n", connection->id);
      socket->client(connection->id) = new AsyncWebSocketClient(connection);
    }
    AsyncWebSocketClient* client = socket->client(connection->id);

    struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
    socket->_handler(socket, client,
        ev == MG_EV_WS_MSG ? WS_EVT_DATA : WS_EVT_CONNECT,
        NULL,
        (uint8_t*)wm->data.buf, wm->data.len);
  } else if (ev == MG_EV_CLOSE) {
    Serial.printf("Web: Connection %lu closed\n", connection->id);
    AsyncWebSocket* socket = getWebSocket(connection);
    if (socket->hasClient(connection->id)) {
      Serial.printf("Web: Websocket on connection %lu closed\n", connection->id);
      AsyncWebSocketClient* client = socket->client(connection->id);
      socket->_handler(socket, client, WS_EVT_DISCONNECT, NULL, NULL, 0);
      socket->removeClient(connection->id);
      delete client;
    }
  }
}

void AsyncWebServer::begin() {
  mg_log_set_fn(logMessage, NULL);
  mg_mgr_init(&manager); // Initialise event manager
  String url = String("http://0.0.0.0:") + _port;
  mg_connection* conn = mg_http_listen(&manager, url.c_str(), handleEvent, this);
  if (!conn) {
    Serial.printf("Web: Error: Could not listen to port %d\n", _port);
  }
}

void AsyncWebServer::addHandler(AsyncWebSocket* webSocketServer) {
  _webSocketServer = webSocketServer;
  webSocketServer->addHandler(this);
}

void AsyncWebServer::poll() {
  mg_mgr_poll(&manager, 0);
}

uint32_t AsyncWebSocketClient::id() const {
  return ((mg_connection*)_connection)->id;
}

void AsyncWebSocketClient::text(const String& t) {
  mg_connection* con = ((mg_connection*)_connection);
  mg_ws_send(con, t.c_str(), t.length(), WEBSOCKET_OP_TEXT);
}

void AsyncWebSocketClient::binary(uint8_t* message, size_t len) {
  mg_connection* con = ((mg_connection*)_connection);
  mg_ws_send(con, message, len, WEBSOCKET_OP_BINARY);
}
