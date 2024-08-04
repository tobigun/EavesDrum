/****************************************************************************************************************************
  AsyncTCP_RP2040W_buffer.h
  
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
/**
 * @file AsyncTCP_RP2040W_buffer.h
 * @date  22.01.2016
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the Asynv TCP for ESP.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef ASYNCTCP_RP2040W_BUFFER_H_
#define ASYNCTCP_RP2040W_BUFFER_H_

#ifndef DEBUG_ASYNC_TCP
  #define DEBUG_ASYNC_TCP(...)
#endif

#include <Arduino.h>
#include <cbuf.h>

#include "AsyncTCP_RP2040W.h"

typedef enum 
{
  ATB_RX_MODE_NONE,
  ATB_RX_MODE_FREE,
  ATB_RX_MODE_READ_BYTES,
  ATB_RX_MODE_TERMINATOR,
  ATB_RX_MODE_TERMINATOR_STRING
} atbRxMode_t;

/////////////////////////////////////////////////////////

class AsyncTCPbuffer: public Print 
{

  public:

    typedef std::function<size_t(uint8_t * payload, size_t length)> AsyncTCPbufferDataCb;
    typedef std::function<void(bool ok, void * ret)> AsyncTCPbufferDoneCb;
    typedef std::function<bool(AsyncTCPbuffer * obj)> AsyncTCPbufferDisconnectCb;

    AsyncTCPbuffer(AsyncClient* c);
    virtual ~AsyncTCPbuffer();

    size_t write(String & data);
    size_t write(uint8_t data);
    size_t write(const char* data);
    size_t write(const char *data, size_t len);
    size_t write(const uint8_t *data, size_t len);

    void flush();

    void noCallback();

    void readStringUntil(char terminator, String * str, AsyncTCPbufferDoneCb done);

    // TODO implement read terminator non string
    //void readBytesUntil(char terminator, char *buffer, size_t length, AsyncTCPbufferDoneCb done);
    //void readBytesUntil(char terminator, uint8_t *buffer, size_t length, AsyncTCPbufferDoneCb done);

    void readBytes(char *buffer, size_t length, AsyncTCPbufferDoneCb done);
    void readBytes(uint8_t *buffer, size_t length, AsyncTCPbufferDoneCb done);

    // TODO implement
    // void setTimeout(size_t timeout);

    void onData(AsyncTCPbufferDataCb cb);
    void onDisconnect(AsyncTCPbufferDisconnectCb cb);

    IPAddress remoteIP();
    uint16_t  remotePort();
    IPAddress localIP();
    uint16_t  localPort();

    bool connected();

    void stop();
    void close();

  protected:
    AsyncClient* _client;
    cbuf * _TXbufferRead;
    cbuf * _TXbufferWrite;
    cbuf * _RXbuffer;
    atbRxMode_t _RXmode;
    size_t _rxSize;
    char _rxTerminator;
    uint8_t * _rxReadBytesPtr;
    String * _rxReadStringPtr;

    AsyncTCPbufferDataCb _cbRX;
    AsyncTCPbufferDoneCb _cbDone;
    AsyncTCPbufferDisconnectCb _cbDisconnect;

    void _attachCallbacks();
    void _sendBuffer();
    void _on_close();
    void _rxData(uint8_t *buf, size_t len);
    size_t _handleRxBuffer(uint8_t *buf, size_t len);
};

#endif /* ASYNCTCP_RP2040W_BUFFER_H_ */
