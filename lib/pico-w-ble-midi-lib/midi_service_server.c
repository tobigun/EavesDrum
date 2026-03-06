/**
 * The portion of the software below that is totally original to rppicomidi
 * uses the following MIT License
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
#define BTSTACK_FILE__ "midi_service_server.c"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "btstack_defines.h"
#include "ble/att_db.h"
#include "ble/att_server.h"
#include "hci.h"
#include "btstack_util.h"
#include "bluetooth_gatt.h"
#include "btstack_debug.h"

#include "midi_service_server.h"

//
static att_service_handler_t  midi_service;
static btstack_packet_handler_t client_packet_handler;

static uint16_t midi_rx_value_handle;
static uint16_t midi_tx_value_handle;
static uint16_t midi_tx_client_configuration_handle;
static uint16_t midi_tx_client_configuration_value;

static void midi_service_emit_state(hci_con_handle_t con_handle, bool enabled){
    uint8_t event[5];
    uint8_t pos = 0;
    event[pos++] = HCI_EVENT_GATTSERVICE_META;
    event[pos++] = sizeof(event) - 2;
    event[pos++] = enabled ? GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED : GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED;
    little_endian_store_16(event,pos, (uint16_t) con_handle);
    pos += 2;
    (*client_packet_handler)(HCI_EVENT_PACKET, 0, event, pos);
}

// handle reads from the server
static uint16_t midi_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
	UNUSED(con_handle);
	UNUSED(offset);
	UNUSED(buffer_size);
	if (attribute_handle == midi_tx_client_configuration_handle){
		if (buffer != NULL){
			little_endian_store_16(buffer, 0, midi_tx_client_configuration_handle);
		}
		return 2;
	}
#if 0
    if (attribute_handle == midi_tx_value_handle) {
        // I think this should return a payload of 0 length
        uint8_t dummy;
        return att_read_callback_handle_blob(&dummy, 0, 0, buffer, buffer_size);
    }
#endif
	return 0;
}

// handle writes to the server
static int midi_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
	UNUSED(transaction_mode);
	UNUSED(offset);
	UNUSED(buffer_size);
	
	if (attribute_handle == midi_rx_value_handle){
        (*client_packet_handler)(RFCOMM_DATA_PACKET, (uint16_t) con_handle, buffer, buffer_size);
	}

	if (attribute_handle == midi_tx_client_configuration_handle){
		midi_tx_client_configuration_value = little_endian_read_16(buffer, 0);
		bool enabled = (midi_tx_client_configuration_value != 0);
        midi_service_emit_state(con_handle, enabled);
	}

	return 0;
}

void midi_service_server_init(btstack_packet_handler_t packet_handler){

    static const uint8_t midi_profile_uuid128[] = { 0x03, 0xB8, 0x0E, 0x5A, 0xED, 0xE8, 0x4B, 0x33, 0xA7, 0x51, 0x6C, 0xE3, 0x4E, 0xC4, 0xC7, 0x00 };
    static const uint8_t midi_characteristic_uuid128[] = { 0x77, 0x72, 0xE5, 0xDB, 0x38, 0x68, 0x41, 0x12, 0xA1, 0xA9, 0xF2, 0x66, 0x9D, 0x10, 0x6B, 0xF3 };

    client_packet_handler = packet_handler;

	// get service handle range
	uint16_t start_handle = 0;
	uint16_t end_handle   = 0xffff;
	int service_found = gatt_server_get_handle_range_for_service_with_uuid128(midi_profile_uuid128, &start_handle, &end_handle);
	btstack_assert(service_found != 0);
	UNUSED(service_found);

	// get characteristic value handle and client configuration handle
	midi_rx_value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid128(start_handle, end_handle, midi_characteristic_uuid128);
	midi_tx_value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid128(start_handle, end_handle, midi_characteristic_uuid128);
	midi_tx_client_configuration_handle = gatt_server_get_client_configuration_handle_for_characteristic_with_uuid128(start_handle, end_handle, midi_characteristic_uuid128);

	log_info("midi_rx_value_handle 					0x%02x", midi_rx_value_handle);
	log_info("midi_tx_value_handle 					0x%02x", midi_tx_value_handle);
	log_info("midi_tx_client_configuration_handle 	0x%02x", midi_tx_client_configuration_handle);
	
	// register service with ATT Server
	midi_service.start_handle   = start_handle;
	midi_service.end_handle     = end_handle;
	midi_service.read_callback  = &midi_service_read_callback;
	midi_service.write_callback = &midi_service_write_callback;
	att_server_register_service_handler(&midi_service);
}

void midi_service_server_deinit()
{
    // TODO
}

bool midi_service_server_request_can_send_now(btstack_context_callback_registration_t* request, hci_con_handle_t con_handle){
	return att_server_request_to_send_notification(request, con_handle) == ERROR_CODE_SUCCESS;
}

int midi_service_server_send(hci_con_handle_t con_handle, const uint8_t * data, uint16_t size){
	return att_server_notify(con_handle, midi_tx_value_handle, data, size);
}
