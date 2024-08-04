/****************************************************************************************************************************
  async_config.h
  
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

#ifndef _RP2040W_ASYNC_CONFIG_H_
#define _RP2040W_ASYNC_CONFIG_H_

#ifndef TCP_MSS
  // May have been definded as a -DTCP_MSS option on the compile line or not.
  // Arduino core 2.3.0 or earlier does not do the -DTCP_MSS option.
  // Later versions may set this option with info from board.txt.
  // However, Core 2.4.0 and up board.txt does not define TCP_MSS for lwIP v1.4
  #define TCP_MSS       MBED_CONF_LWIP_TCP_MSS        //(1460)
#endif

#endif // _RP2040W_ASYNC_CONFIG_H_
