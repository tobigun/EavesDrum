/****************************************************************************************************************************
  AsyncPrinter.cpp
  
  For RP2040W with CYW43439 WiFi
     
  AsyncTCP_RP2040W is a library for the RP2040W with CYW43439 WiFi
  
  Based on and modified from AsyncTCP (https://github.com/me-no-dev/ESPAsyncTCP)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncTCP_RP2040W
  
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License 
  as published bythe Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.  
  If not, see <https://www.gnu.org/licenses/>.
 
  Version: 1.2.0
  
  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      13/08/2022 Initial coding for RP2040W with CYW43439 WiFi
  1.1.0   K Hoang      25/09/2022 Fix issue with slow browsers or network. Clean up. Remove hard-code if possible
  1.2.0   K Hoang      02/02/2023 Add Client and Server examples
 *****************************************************************************************************************************/
/*
  Asynchronous TCP library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
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

#if !defined(_ASYNCTCP_RP2040W_LOGLEVEL_)
  #define _ASYNCTCP_RP2040W_LOGLEVEL_     1
#endif

#include "AsyncPrinter.h"
#include "debug.h"

/////////////////////////////////////////////////////////

AsyncPrinter::AsyncPrinter()
  : _client(NULL)
  , _data_cb(NULL)
  , _data_arg(NULL)
  , _close_cb(NULL)
  , _close_arg(NULL)
  , _tx_buffer(NULL)
  , _tx_buffer_size(TCP_MSS)
  , next(NULL)
{}

/////////////////////////////////////////////////////////

AsyncPrinter::AsyncPrinter(AsyncClient *client, size_t txBufLen)
  : _client(client)
  , _data_cb(NULL)
  , _data_arg(NULL)
  , _close_cb(NULL)
  , _close_arg(NULL)
  , _tx_buffer(NULL)
  , _tx_buffer_size(txBufLen)
  , next(NULL)
{
  _attachCallbacks();
  _tx_buffer = new (std::nothrow) cbuf(_tx_buffer_size);
  
  if (_tx_buffer == NULL) 
  {
    panic(); //What should we do?
  }
}

/////////////////////////////////////////////////////////

AsyncPrinter::~AsyncPrinter() 
{
  _on_close();
}

/////////////////////////////////////////////////////////

void AsyncPrinter::onData(ApDataHandler cb, void *arg) 
{
  _data_cb = cb;
  _data_arg = arg;
}

/////////////////////////////////////////////////////////

void AsyncPrinter::onClose(ApCloseHandler cb, void *arg) 
{
  _close_cb = cb;
  _close_arg = arg;
}

/////////////////////////////////////////////////////////

int AsyncPrinter::connect(IPAddress ip, uint16_t port) 
{
  if (_client != NULL && connected())
    return 0;
    
  _client = new (std::nothrow) AsyncClient();
  
  if (_client == NULL) 
  {
    panic();
  }

  _client->onConnect([](void *obj, AsyncClient * c) 
  {
    ((AsyncPrinter*)(obj))->_onConnect(c);
  }, this);
  
  if (_client->connect(ip, port)) 
  {
    while (_client && _client->state() < 4)
      delay(1);
      
    return connected();
  }
  
  return 0;
}

/////////////////////////////////////////////////////////

int AsyncPrinter::connect(const char *host, uint16_t port) 
{
  if (_client != NULL && connected())
    return 0;
    
  _client = new (std::nothrow) AsyncClient();
  
  if (_client == NULL) 
  {
    panic();
  }

  _client->onConnect([](void *obj, AsyncClient * c) 
  {
    ((AsyncPrinter*)(obj))->_onConnect(c);
  }, this);
  
  if (_client->connect(host, port)) 
  {
    while (_client && _client->state() < 4)
      delay(1);
      
    return connected();
  }
  
  return 0;
}

/////////////////////////////////////////////////////////

void AsyncPrinter::_onConnect(AsyncClient *c) 
{
  ASYNCTCP_RP2040W_UNUSED(c);
  
  if (_tx_buffer != NULL) 
  {
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  
  _tx_buffer = new (std::nothrow) cbuf(_tx_buffer_size);
  
  if (_tx_buffer) 
  {
    panic();
  }

  _attachCallbacks();
}

/////////////////////////////////////////////////////////

AsyncPrinter::operator bool() 
{
  return connected();
}

/////////////////////////////////////////////////////////

AsyncPrinter & AsyncPrinter::operator=(const AsyncPrinter &other) 
{
  if (_client != NULL) 
  {
    _client->close(true);
    _client = NULL;
  }
  
  _tx_buffer_size = other._tx_buffer_size;
  
  if (_tx_buffer != NULL) 
  {
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  
  _tx_buffer = new (std::nothrow) cbuf(other._tx_buffer_size);
  
  if (_tx_buffer == NULL) 
  {
    panic();
  }

  _client = other._client;
  _attachCallbacks();
  
  return *this;
}

/////////////////////////////////////////////////////////

size_t AsyncPrinter::write(uint8_t data) 
{
  return write(&data, 1);
}

/////////////////////////////////////////////////////////

size_t AsyncPrinter::write(const uint8_t *data, size_t len) 
{
  if (_tx_buffer == NULL || !connected())
    return 0;
    
  size_t toWrite = 0;
  size_t toSend = len;
  
  while (_tx_buffer->room() < toSend) 
  {
    toWrite = _tx_buffer->room();
    _tx_buffer->write((const char*)data, toWrite);
    
    while (connected() && !_client->canSend())
      delay(0);
      
    if (!connected())
      return 0; // or len - toSend;
      
    _sendBuffer();
    toSend -= toWrite;
  }
  
  _tx_buffer->write((const char*)(data + (len - toSend)), toSend);
  
  while (connected() && !_client->canSend()) 
    delay(0);
    
  if (!connected()) 
    return 0; // or len - toSend;
    
  _sendBuffer();
  
  return len;
}

/////////////////////////////////////////////////////////

bool AsyncPrinter::connected() 
{
  return (_client != NULL && _client->connected());
}

/////////////////////////////////////////////////////////

void AsyncPrinter::close() 
{
  if (_client != NULL)
    _client->close(true);
}

/////////////////////////////////////////////////////////

size_t AsyncPrinter::_sendBuffer() 
{
  size_t available = _tx_buffer->available();
  
  if (!connected() || !_client->canSend() || available == 0)
    return 0;
    
  size_t sendable = _client->space();
  
  if (sendable < available)
    available = sendable;
    
  char *out = new (std::nothrow) char[available];
  
  if (out == NULL) 
  {
    panic(); // Connection should be aborted instead
  }

  _tx_buffer->read(out, available);
  size_t sent = _client->write(out, available);
  delete out;
  
  return sent;
}

/////////////////////////////////////////////////////////

void AsyncPrinter::_onData(void *data, size_t len) 
{
  if (_data_cb)
    _data_cb(_data_arg, this, (uint8_t*)data, len);
}

/////////////////////////////////////////////////////////

void AsyncPrinter::_on_close() 
{
  if (_client != NULL) 
  {
    _client = NULL;
  }
  
  if (_tx_buffer != NULL) 
  {
    cbuf *b = _tx_buffer;
    _tx_buffer = NULL;
    delete b;
  }
  
  if (_close_cb)
    _close_cb(_close_arg, this);
}

/////////////////////////////////////////////////////////

void AsyncPrinter::_attachCallbacks()
{
  _client->onPoll([](void *obj, AsyncClient * c)
  {
    ASYNCTCP_RP2040W_UNUSED(c);
    ((AsyncPrinter*)(obj))->_sendBuffer();
  }, this);

  _client->onAck([](void *obj, AsyncClient * c, size_t len, uint32_t time)
  {
    ASYNCTCP_RP2040W_UNUSED(c);
    ASYNCTCP_RP2040W_UNUSED(len);
    ASYNCTCP_RP2040W_UNUSED(time);

    ((AsyncPrinter*)(obj))->_sendBuffer();
  }, this);

  _client->onDisconnect([](void *obj, AsyncClient * c)
  {
    ((AsyncPrinter*)(obj))->_on_close(); delete c;
  }, this);

  _client->onData([](void *obj, AsyncClient * c, void *data, size_t len)
  {
    ASYNCTCP_RP2040W_UNUSED(c);
    ((AsyncPrinter*)(obj))->_onData(data, len);
  }, this);
}

/////////////////////////////////////////////////////////
