/****************************************************************************************************************************
  AsyncWebSynchronization_RP2040W.h

  For RP2040W with CYW43439 WiFi

  AsyncWebServer_RP2040W is a library for the RP2040W with CYW43439 WiFi

  Based on and modified from ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncWebServer_RP2040W
  Licensed under GPLv3 license

  Version: 1.5.0

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      13/08/2022 Initial coding for RP2040W with CYW43439 WiFi
  ...
  1.3.0   K Hoang      10/10/2022 Fix crash when using AsyncWebSockets server
  1.3.1   K Hoang      10/10/2022 Improve robustness of AsyncWebSockets server
  1.4.0   K Hoang      20/10/2022 Add LittleFS functions such as AsyncFSWebServer
  1.4.1   K Hoang      10/11/2022 Add examples to demo how to use beginChunkedResponse() to send in chunks
  1.4.2   K Hoang      28/01/2023 Add Async_AdvancedWebServer_SendChunked_MQTT and AsyncWebServer_MQTT_RP2040W examples
  1.5.0   K Hoang      30/01/2023 Fix _catchAllHandler not working bug
 *****************************************************************************************************************************/

#pragma once

#ifndef RP2040W_ASYNCWEBSYNCHRONIZATION_H_
#define RP2040W_ASYNCWEBSYNCHRONIZATION_H_

#include <AsyncWebServer_RP2040W.h>

// This is the RP2040W version of the Sync Lock which is currently unimplemented
class AsyncWebLock
{
  private:
    mutable semaphore_t _lock;

  public:
    AsyncWebLock()  {
      sem_init(&_lock, 1, 1);
    }

    ~AsyncWebLock() {}

    /////////////////////////////////////////////////

    inline bool lock() const
    {
      sem_acquire_blocking(&_lock);
      return true;
    }

    /////////////////////////////////////////////////

    inline void unlock() const {
      sem_release(&_lock);
    }
};

class AsyncWebLockGuard
{
  private:
    const AsyncWebLock *_lock;

  public:

    /////////////////////////////////////////////////

    AsyncWebLockGuard(const AsyncWebLock &l)
    {
      if (l.lock())
      {
        _lock = &l;
      }
      else
      {
        _lock = NULL;
      }
    }

    /////////////////////////////////////////////////

    ~AsyncWebLockGuard()
    {
      if (_lock)
      {
        _lock->unlock();
      }
    }
};

#endif // RP2040W_ASYNCWEBSYNCHRONIZATION_H_
