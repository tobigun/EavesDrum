/****************************************************************************************************************************
  AsyncTCP_RP2040W_Debug.h
  
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

#pragma once

#ifndef ASYNCTCP_RP2040W_DEBUG_H
#define ASYNCTCP_RP2040W_DEBUG_H

// Change _ASYNCTCP_RP2040W_LOGLEVEL_ to set tracing and logging verbosity
// 0: DISABLED: no logging
// 1: ERROR: errors
// 2: WARN: errors and warnings
// 3: INFO: errors, warnings and informational (default)
// 4: DEBUG: errors, warnings, informational and debug

#ifndef _ASYNCTCP_RP2040W_LOGLEVEL_
  #define _ASYNCTCP_RP2040W_LOGLEVEL_       1
#endif

/////////////////////////////////////////////////////////

#define ATCP_PRINT_MARK      ATCP_PRINT("[ATCP] ")
#define ATCP_PRINT_SP        ATCP_PRINT(" ")
#ifdef ASYNCTCP_RP2040W_DEBUG_PORT
  #define DBG_PORT_ATCP      ASYNCTCP_RP2040W_DEBUG_PORT
  #define ATCP_PRINT           DBG_PORT_ATCP.print
  #define ATCP_PRINTLN         DBG_PORT_ATCP.println
#else
  #define ATCP_PRINT(...)
  #define ATCP_PRINTLN(...)
#endif


/////////////////////////////////////////////////////////

#define ATCP_LOGERROR(x)         if(_ASYNCTCP_RP2040W_LOGLEVEL_>0) { ATCP_PRINT_MARK; ATCP_PRINTLN(x); }
#define ATCP_LOGERROR0(x)        if(_ASYNCTCP_RP2040W_LOGLEVEL_>0) { ATCP_PRINT(x); }
#define ATCP_LOGERROR1(x,y)      if(_ASYNCTCP_RP2040W_LOGLEVEL_>0) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINTLN(y); }
#define ATCP_LOGERROR2(x,y,z)    if(_ASYNCTCP_RP2040W_LOGLEVEL_>0) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINTLN(z); }
#define ATCP_LOGERROR3(x,y,z,w)  if(_ASYNCTCP_RP2040W_LOGLEVEL_>0) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINT(z); ATCP_PRINT_SP; ATCP_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define ATCP_LOGWARN(x)          if(_ASYNCTCP_RP2040W_LOGLEVEL_>1) { ATCP_PRINT_MARK; ATCP_PRINTLN(x); }
#define ATCP_LOGWARN0(x)         if(_ASYNCTCP_RP2040W_LOGLEVEL_>1) { ATCP_PRINT(x); }
#define ATCP_LOGWARN1(x,y)       if(_ASYNCTCP_RP2040W_LOGLEVEL_>1) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINTLN(y); }
#define ATCP_LOGWARN2(x,y,z)     if(_ASYNCTCP_RP2040W_LOGLEVEL_>1) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINTLN(z); }
#define ATCP_LOGWARN3(x,y,z,w)   if(_ASYNCTCP_RP2040W_LOGLEVEL_>1) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINT(z); ATCP_PRINT_SP; ATCP_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define ATCP_LOGINFO(x)          if(_ASYNCTCP_RP2040W_LOGLEVEL_>2) { ATCP_PRINT_MARK; ATCP_PRINTLN(x); }
#define ATCP_LOGINFO0(x)         if(_ASYNCTCP_RP2040W_LOGLEVEL_>2) { ATCP_PRINT(x); }
#define ATCP_LOGINFO1(x,y)       if(_ASYNCTCP_RP2040W_LOGLEVEL_>2) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINTLN(y); }
#define ATCP_LOGINFO2(x,y,z)     if(_ASYNCTCP_RP2040W_LOGLEVEL_>2) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINTLN(z); }
#define ATCP_LOGINFO3(x,y,z,w)   if(_ASYNCTCP_RP2040W_LOGLEVEL_>2) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINT(z); ATCP_PRINT_SP; ATCP_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define ATCP_LOGDEBUG(x)         if(_ASYNCTCP_RP2040W_LOGLEVEL_>3) { ATCP_PRINT_MARK; ATCP_PRINTLN(x); }
#define ATCP_LOGDEBUG0(x)        if(_ASYNCTCP_RP2040W_LOGLEVEL_>3) { ATCP_PRINT(x); }
#define ATCP_LOGDEBUG1(x,y)      if(_ASYNCTCP_RP2040W_LOGLEVEL_>3) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINTLN(y); }
#define ATCP_LOGDEBUG2(x,y,z)    if(_ASYNCTCP_RP2040W_LOGLEVEL_>3) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINTLN(z); }
#define ATCP_LOGDEBUG3(x,y,z,w)  if(_ASYNCTCP_RP2040W_LOGLEVEL_>3) { ATCP_PRINT_MARK; ATCP_PRINT(x); ATCP_PRINT_SP; ATCP_PRINT(y); ATCP_PRINT_SP; ATCP_PRINT(z); ATCP_PRINT_SP; ATCP_PRINTLN(w); }

/////////////////////////////////////////////////////////

#endif    //ASYNCTCP_RP2040W_DEBUG_H
