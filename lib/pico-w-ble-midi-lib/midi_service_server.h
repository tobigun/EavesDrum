/**
 * MIT License
 *
 * Copyright (c) 2023 rppicomidi
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

#pragma once

#include <stdint.h>
#include "bluetooth.h"
#include "btstack_defines.h"

#if defined __cplusplus
extern "C" {
#endif

/* API_START */

/**
 * @text A Bluetooth LE MIDI Service server implementation.
 *
 * To use with your application, add `#import <midi_service.gatt>' to your .gatt file
 * and call all functions below. All strings and blobs need to stay valid after calling the functions.
 */

/**
 * @brief Init MIDI Service Server with ATT DB
 * @param packet_handler for events and midi data from peer
 */
void midi_service_server_init(btstack_packet_handler_t packet_handler);

void midi_service_server_deinit();

/** 
 * @brief Queue send request. When called, one packet can be sent via ble_midi_service_send below
 * @param request
 * @param con_handle
 * @return true if successful, false otherwise
 */
bool midi_service_server_request_can_send_now(btstack_context_callback_registration_t * request, hci_con_handle_t con_handle);

/**
 * @brief Send data
 * @param con_handle
 * @param data
 * @param size
 */
int midi_service_server_send(hci_con_handle_t con_handle, const uint8_t * data, uint16_t size);

/* API_END */

#if defined __cplusplus
}
#endif