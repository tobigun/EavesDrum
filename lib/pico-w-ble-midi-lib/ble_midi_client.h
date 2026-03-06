/**
 * MIT License
 *
 * Copyright (c) 2024 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * To the extent this code is solely for use on the Rapsberry Pi Pico W or
 * Pico WH, the license file ${PICO_SDK_PATH}/src/rp2_common/pico_btstack/LICENSE.RP may
 * apply.
 * 
 */

/**
 * This file uses code from various BlueKitchen example files, which contain
 * the following copyright notice, included per the notice below.
 *
 * Copyright (C) 2018 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

#pragma once

#include <stdint.h>
#include "bluetooth.h"
#include "btstack_defines.h"

#if defined __cplusplus
extern "C" {
#endif

/* API_START */

/**
 * @brief Initialize the BLE-MIDI client
 * 
 * @param profile_name the name of the profile to advertise
 * @param profile_name_len the number of characters in the profile name
 */
void ble_midi_client_init(const char* profile_name, uint8_t profile_name_len, io_capability_t iocaps, uint8_t secmask);

/**
 * @brief de-initialize the BLE-MIDI client in preparation to switching to server mode
 *
 */
void ble_midi_client_deinit();

/**
 * @brief set the BDADDR and type of the last connected BLE MIDI server
 *
 * @param addr_type the BLE address type
 * @param addr pointer to the 6-byte address
 */
void ble_midi_client_set_last_connected(int addr_type, uint8_t* addr);

/**
 * @brief get the BDADDR and type of the last connected BLE MIDI server
 *
 * @param addr a pointer to the 6 byte bdaddr to receive the address
 * @return the BLE address type or BD_ADDR_TYPE_UNKNOWN if never set
 */
int ble_midi_client_get_last_conntected(uint8_t* addr);

/**
 * @brief Switch to client mode if necessary, and start a BLE scan for BLE-MIDI servers
 *
 */
void ble_midi_client_scan_begin();

/**
 * @brief End a BLE scan for BLE-MIDI servers
 *
 */
void ble_midi_client_scan_end();

/**
 * @brief print (to the console) a list of all devices discovered in the scan
 *
 */
void ble_midi_client_dump_midi_peripherals();

/**
 * @brief request connection to a device discovered during the scan
 *
 * @param idx the index number of the device as displayed by ble_midi_client_dump_midi_peripherals()
 * @return true if the connection request is successful
 * @return false if the device was never discovered or the connection request failed immediately.
 */
bool ble_midi_client_request_connect(uint8_t idx);

/**
 * @brief If already connected, disconnect from the connected server.
 *
 * This function returns before the disconnection is complete.
 */
void ble_midi_client_request_disconnect();

/**
 * @brief write a MIDI 1.0 byte stream nbytes long
 *
 * The MIDI byte stream will be timestamped with the time the stream
 * is writen and will be sent when the MIDI service allows it. The
 * stream may not use running status. However, Bluetooth MIDI packets
 * will be encoded with running status if it is possible to do so.
 * 
 * @param nbytes the number of bytes in the MIDI byte stream
 * @param midi_stream_bytes a pointer to the MIDI 1.0 byte stream storage
 * @return uint8_t the number of bytes successfully written
 */
uint8_t ble_midi_client_stream_write(uint8_t nbytes, const uint8_t* midi_stream_bytes);

/**
 * @brief read a MIDI 1.0 byte stream for a single timestamp up to max_bytes long
 *
 * The application that calls this function should call it in a loop until
 * the function returns 0 (no more timestamped byte streams available to read)
 *
 * This function removes any Bluetooth MIDI running status from the byte stream
 * to simplify parsing for the application that calls this function
 *
 * @param con_handle the HCI connection handle for the connection providing the MIDI stream
 * @param max_bytes The length of the midi_stream_bytes array
 * @param midi_stream_bytes a pointer to the MIDI 1.0 formatted byte stream storage
 * @param timestamp a pointer to the MIDI byte stream message timestamp. If this
 * pointer is NULL, then no timestamp will be returned.
 * @return uint8_t the number of bytes read for the timestamped byte stream or
 * zero if there are no more bytes to read
 */
uint8_t ble_midi_client_stream_read(uint8_t max_bytes, uint8_t* midi_stream_bytes, uint16_t* timestamp);

/**
 * @brief
 *
 * @return true if the client is connected to a server, false otherwise
 */
bool ble_midi_client_is_connected(void);

/**
 * @brief
 *
 * @return true if the client is connected to a server and the
 * client is ready to send and receive MIDI data
 */
bool ble_midi_client_is_ready(void);

/**
 * @brief
 *
 * @return true if the client is completely de-initialized
 */
bool ble_midi_client_is_off(void);

bool ble_midi_client_waiting_for_connection();

void ble_midi_client_cancel_connection_request();

bool ble_midi_client_get_keep_connected();
void ble_midi_client_set_keep_connected(bool keep_client_connected_);
#if defined __cplusplus
}
#endif