/****************************************************************************************************************************
  DebugPrintMacros.h
  
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

#ifndef _DEBUG_PRINT_MACROS_H
#define _DEBUG_PRINT_MACROS_H

// Some customizable print macros to suite the debug needs de jour.

// Debug macros
// #include <pgmspace.h>
// https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
// This value is resolved at compile time.
#define _FILENAME_ strrchr("/" __FILE__, '/')

/////////////////////////////////////////////////////////

#if defined(DEBUG_ESP_PORT) && !defined(DEBUG_TIME_STAMP_FMT)
  #define DEBUG_TIME_STAMP_FMT    "%06u.%03u "
  
  struct _DEBUG_TIME_STAMP 
  {
    unsigned dec;
    unsigned whole;
  };
  
  inline struct _DEBUG_TIME_STAMP debugTimeStamp() 
  {
    struct _DEBUG_TIME_STAMP st;
    unsigned now = millis() % 1000000000;
    st.dec = now % 1000;
    st.whole = now / 1000;
    return st;
  }
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_ESP_PORT) && !defined(DEBUG_GENERIC)
  #define DEBUG_GENERIC( module, format, ... ) \
    do { \
      struct _DEBUG_TIME_STAMP st = debugTimeStamp(); \
      DEBUG_ESP_PORT.printf( DEBUG_TIME_STAMP_FMT module " " format, st.whole, st.dec, ##__VA_ARGS__ ); \
    } while(false)
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_ESP_PORT) && !defined(DEBUG_GENERIC_P)
  #define DEBUG_GENERIC_P( module, format, ... ) \
    do { \
      struct _DEBUG_TIME_STAMP st = debugTimeStamp(); \
      DEBUG_ESP_PORT.printf_P(PSTR( DEBUG_TIME_STAMP_FMT module " " format ), st.whole, st.dec, ##__VA_ARGS__ ); \
    } while(false)
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_GENERIC) && !defined(ASSERT_GENERIC)
  #define ASSERT_GENERIC( a, module ) \
    do { \
      if ( !(a) ) { \
        DEBUG_GENERIC( module, "%s:%s:%u: ASSERT("#a") failed!\n", __FILE__, __func__, __LINE__); \
        DEBUG_ESP_PORT.flush(); \
      } \
    } while(false)
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_GENERIC_P) && !defined(ASSERT_GENERIC_P)
  #define ASSERT_GENERIC_P( a, module ) \
    do { \
      if ( !(a) ) { \
        DEBUG_GENERIC_P( module, "%s:%s:%u: ASSERT("#a") failed!\n", __FILE__, __func__, __LINE__); \
        DEBUG_ESP_PORT.flush(); \
      } \
    } while(false)
#endif

/////////////////////////////////////////////////////////

#ifndef DEBUG_GENERIC
  #define DEBUG_GENERIC(...) do { (void)0;} while(false)
#endif

/////////////////////////////////////////////////////////

#ifndef DEBUG_GENERIC_P
  #define DEBUG_GENERIC_P(...) do { (void)0;} while(false)
#endif

/////////////////////////////////////////////////////////

#ifndef ASSERT_GENERIC
  #define ASSERT_GENERIC(...) do { (void)0;} while(false)
#endif

/////////////////////////////////////////////////////////

#ifndef ASSERT_GENERIC_P
  #define ASSERT_GENERIC_P(...) do { (void)0;} while(false)
#endif

/////////////////////////////////////////////////////////

#ifndef DEBUG_ESP_PRINTF
  #define DEBUG_ESP_PRINTF( format, ...) DEBUG_GENERIC_P("[%s]", format, &_FILENAME_[1], ##__VA_ARGS__)
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_ESP_ASYNC_TCP) && !defined(ASYNC_TCP_DEBUG)
  #define ASYNC_TCP_DEBUG( format, ...) DEBUG_GENERIC_P("[ASYNC_TCP]", format, ##__VA_ARGS__)
#endif

/////////////////////////////////////////////////////////

#ifndef ASYNC_TCP_ASSERT
  #define ASYNC_TCP_ASSERT( a ) ASSERT_GENERIC_P( (a), "[ASYNC_TCP]")
#endif

/////////////////////////////////////////////////////////

#if defined(DEBUG_ESP_TCP_SSL) && !defined(TCP_SSL_DEBUG)
  #define TCP_SSL_DEBUG( format, ...) DEBUG_GENERIC_P("[TCP_SSL]", format, ##__VA_ARGS__)
#endif

/////////////////////////////////////////////////////////

#endif //_DEBUG_PRINT_MACROS_H
