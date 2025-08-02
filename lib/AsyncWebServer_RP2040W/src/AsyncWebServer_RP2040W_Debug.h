/****************************************************************************************************************************
  AsyncWebServer_RP2040W_Debug.h

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

#ifndef RP2040W_ASYNC_WEBSERVER_DEBUG_H
#define RP2040W_ASYNC_WEBSERVER_DEBUG_H

/////////////////////////////////////////////////

// Change _RP2040W_AWS_LOGLEVEL_ to set tracing and logging verbosity
// 0: DISABLED: no logging
// 1: ERROR: errors
// 2: WARN: errors and warnings
// 3: INFO: errors, warnings and informational (default)
// 4: DEBUG: errors, warnings, informational and debug

#ifndef _RP2040W_AWS_LOGLEVEL_
  #define _RP2040W_AWS_LOGLEVEL_       0
#endif

/////////////////////////////////////////////////////////

#define AWS_PRINT_MARK      AWS_PRINT("[AWS] ")
#define AWS_PRINT_SP        AWS_PRINT(" ")
#ifdef RP2040W_ASYNCWEBSERVER_DEBUG_PORT
#define DBG_PORT_AWS        RP2040W_ASYNCWEBSERVER_DEBUG_PORT
#define AWS_PRINT           DBG_PORT_AWS.print
#define AWS_PRINTLN         DBG_PORT_AWS.println
#else
#define AWS_PRINT(...)
#define AWS_PRINTLN(...)
#endif

/////////////////////////////////////////////////////////

#define AWS_LOGERROR(x)         if(_RP2040W_AWS_LOGLEVEL_>0) { AWS_PRINT_MARK; AWS_PRINTLN(x); }
#define AWS_LOGERROR0(x)        if(_RP2040W_AWS_LOGLEVEL_>0) { AWS_PRINT(x); }
#define AWS_LOGERROR1(x,y)      if(_RP2040W_AWS_LOGLEVEL_>0) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINTLN(y); }
#define AWS_LOGERROR2(x,y,z)    if(_RP2040W_AWS_LOGLEVEL_>0) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINTLN(z); }
#define AWS_LOGERROR3(x,y,z,w)  if(_RP2040W_AWS_LOGLEVEL_>0) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINT(z); AWS_PRINT_SP; AWS_PRINTLN(w); }

/////////////////////////////////////////////////

#define AWS_LOGWARN(x)          if(_RP2040W_AWS_LOGLEVEL_>1) { AWS_PRINT_MARK; AWS_PRINTLN(x); }
#define AWS_LOGWARN0(x)         if(_RP2040W_AWS_LOGLEVEL_>1) { AWS_PRINT(x); }
#define AWS_LOGWARN1(x,y)       if(_RP2040W_AWS_LOGLEVEL_>1) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINTLN(y); }
#define AWS_LOGWARN2(x,y,z)     if(_RP2040W_AWS_LOGLEVEL_>1) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINTLN(z); }
#define AWS_LOGWARN3(x,y,z,w)   if(_RP2040W_AWS_LOGLEVEL_>1) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINT(z); AWS_PRINT_SP; AWS_PRINTLN(w); }

/////////////////////////////////////////////////

#define AWS_LOGINFO(x)          if(_RP2040W_AWS_LOGLEVEL_>2) { AWS_PRINT_MARK; AWS_PRINTLN(x); }
#define AWS_LOGINFO0(x)         if(_RP2040W_AWS_LOGLEVEL_>2) { AWS_PRINT(x); }
#define AWS_LOGINFO1(x,y)       if(_RP2040W_AWS_LOGLEVEL_>2) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINTLN(y); }
#define AWS_LOGINFO2(x,y,z)     if(_RP2040W_AWS_LOGLEVEL_>2) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINTLN(z); }
#define AWS_LOGINFO3(x,y,z,w)   if(_RP2040W_AWS_LOGLEVEL_>2) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINT(z); AWS_PRINT_SP; AWS_PRINTLN(w); }

/////////////////////////////////////////////////

#define AWS_LOGDEBUG(x)         if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT_MARK; AWS_PRINTLN(x); }
#define AWS_LOGDEBUG0(x)        if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT(x); }
#define AWS_LOGDEBUG0HEX(x)     if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT(x, HEX); }
#define AWS_LOGDEBUG1(x,y)      if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINTLN(y); }
#define AWS_LOGDEBUG2(x,y,z)    if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINTLN(z); }
#define AWS_LOGDEBUG3(x,y,z,w)  if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINT(z); AWS_PRINT_SP; AWS_PRINTLN(w); }
#define AWS_LOGDEBUG5(x,y,z,w,xx,yy)  if(_RP2040W_AWS_LOGLEVEL_>3) { AWS_PRINT_MARK; AWS_PRINT(x); AWS_PRINT_SP; AWS_PRINT(y); AWS_PRINT_SP; AWS_PRINT(z); AWS_PRINT_SP; AWS_PRINT(w); AWS_PRINT_SP; AWS_PRINT(xx); AWS_PRINT_SP; AWS_PRINTLN(yy);}

/////////////////////////////////////////////////

#endif    //RP2040W_ASYNC_WEBSERVER_DEBUG_H
