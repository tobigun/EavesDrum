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
#include "ble_midi_pkt_codec.h"
#include "midi_service_stream_handler.h"
#include "ring_buffer_lib.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <stdio.h>
#include "ble/att_server.h"
#include "btstack_debug.h"
#include "btstack_event.h"

// This structure defines the data context for each connection
typedef struct midi_service_stream_connection_s {
    int le_notification_enabled;
    hci_con_handle_t connection_handle;
    btstack_context_callback_registration_t send_request;
    ble_midi_codec_data_t* ble_midi_pkt_codec_data;
    char name[7]; //"MIDI x" where x is A, B, C, D
} midi_service_stream_connection_t;
static midi_service_stream_connection_t midi_service_stream_connection[BLE_MIDI_SERVER_MAX_CONNECTIONS];

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_handler_t client_packet_handler;

static void midi_can_send(void * void_context)
{
    midi_service_stream_connection_t* context = (midi_service_stream_connection_t*)void_context;
    ble_midi_packet_t pending_ble_pkt;

    if (ble_midi_pkt_codec_ble_pkt_pop(&pending_ble_pkt, context->ble_midi_pkt_codec_data) == sizeof(pending_ble_pkt)) {
        midi_service_server_send(context->connection_handle, pending_ble_pkt.pkt, pending_ble_pkt.nbytes);
        //printf_hexdump(pending_ble_pkt.pkt, pending_ble_pkt.nbytes);
    }
    else {
        printf("No MIDI to send\r\n");
    }
    if (ble_midi_pkt_codec_ble_pkt_available(context->ble_midi_pkt_codec_data)) {
        // more packets in the buffer
        if (!midi_service_server_request_can_send_now(&context->send_request, context->connection_handle)) {
            printf("midi_service_server_request_can_send_now failed\r\n");
        }
    }
}

static void hci_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    uint16_t conn_interval;
    hci_con_handle_t con_handle;
    uint8_t result;

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    // print connection parameters (without using float operations)
                    con_handle    = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
                    printf("LE Connection - Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                    printf("LE Connection - Connection Latency: %u\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
                    if (conn_interval > 6) {
                        // request min con interval 7.5ms. If the client cannot handle it, then we will hopefull get
                        // get something shorter. For example, per iOS per Apple accessory design guidelines, we can get 15ms:
                        // https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf
                        printf("LE Connection - Request 7.5ms connection interval\n");
                        result = gap_request_connection_parameter_update(con_handle, 6, 6, 0, 0x0048);
                        if (result != ERROR_CODE_SUCCESS) {
                            printf("gap_request_connection_parameter_update failed code=%u\r\n", result);
                        }
                    }
                    break;
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    // print connection parameters (without using float operations)
                    con_handle    = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
                    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                    printf("LE Connection - Connection Param update - connection interval %u.%02u ms, latency %u\n", conn_interval * 125 / 100,
                        25 * (conn_interval & 3), hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                    break;
                default:
                    client_packet_handler(packet_type, channel, packet, size);
                    break;
            }
            break;
        default:
            client_packet_handler(packet_type, channel, packet, size);
            break;
    }
}

// Return the context that contains connection_handle ==  con_handle;
// call with con_handle to HCI_CON_HANDLE_INVALID to find an available context
static midi_service_stream_connection_t* get_context_for_conn_handle(hci_con_handle_t con_handle)
{
    for (uint8_t idx = 0; idx < BLE_MIDI_SERVER_MAX_CONNECTIONS; idx++) {
        if (midi_service_stream_connection[idx].connection_handle == con_handle) {

            midi_service_stream_connection[idx].ble_midi_pkt_codec_data = ble_midi_pkt_codec_get_data_by_index(idx);
            return midi_service_stream_connection + idx;
        }
    }
    return NULL;
}

/**
 * @brief This function handles packets from the Blueooth stack
 * 
 * @param packet_type 
 * @param channel 
 * @param packet 
 * @param size 
 */
static void midi_service_stream_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    hci_con_handle_t con_handle;
    midi_service_stream_connection_t* context;
    switch (packet_type){
        case HCI_EVENT_PACKET:
            if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) break;
            switch (hci_event_gattservice_meta_get_subevent_code(packet)){
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                    con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                    context = get_context_for_conn_handle(con_handle);
                    if (!context) break;
                    context->le_notification_enabled = 1;
                    context->send_request.callback = midi_can_send;
                    context->send_request.context = context;
                    ble_midi_pkt_codec_init_data(context->ble_midi_pkt_codec_data, att_server_get_mtu(con_handle));
                    // also send this event up to the application so it can record the connection handle
                    client_packet_handler(packet_type, channel, packet, size);
                    break;
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                    con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                    context = get_context_for_conn_handle(con_handle);
                    if (!context) break;
                    context->le_notification_enabled = 0;
                    context->connection_handle = HCI_CON_HANDLE_INVALID;
                    client_packet_handler(packet_type, channel, packet, size);
                    break;
                default:
                    client_packet_handler(packet_type, channel, packet, size);
                    break;
            }
            break;
        case RFCOMM_DATA_PACKET:
            //printf("RECV: ");
            //printf_hexdump(packet, size);
            context = get_context_for_conn_handle((hci_con_handle_t) channel);
            uint16_t ndecoded = ble_midi_pkt_codec_ble_midi_decode_push(packet, size, context->ble_midi_pkt_codec_data);
            if (ndecoded != size) {
                printf("Parse error decoding midi packet\r\n");
                printf_hexdump(packet, size);
            }
            break;
        default:
            break;
    }
}

static void att_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    int mtu;
    midi_service_stream_connection_t* context;
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case ATT_EVENT_CONNECTED:
                    // setup new 
                    context = get_context_for_conn_handle(HCI_CON_HANDLE_INVALID);
                    if (!context) break;
                    // use the connection handle from this notification going forward
                    context->connection_handle = att_event_connected_get_handle(packet);
                    printf("%s: ATT connected, handle 0x%04x\r\n", context->name, context->connection_handle);
                    break;
                case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
                    mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
                    context = get_context_for_conn_handle(att_event_mtu_exchange_complete_get_handle(packet));
                    if (!context) break;
                    ble_midi_pkt_codec_update_mtu(context->ble_midi_pkt_codec_data , mtu-3);
                    printf("%s: ATT MTU = %u => max MIDI packet len %u\n", context->name, mtu, ble_midi_pkt_codec_get_mtu(context->ble_midi_pkt_codec_data));
                    break;
                case ATT_EVENT_DISCONNECTED:
                    context = get_context_for_conn_handle(att_event_disconnected_get_handle(packet));
                    if (!context) break;
                    // free connection
                    printf("%s: ATT disconnected, handle 0x%04x\n", context->name, context->connection_handle);
                    context->le_notification_enabled = 0;
                    context->connection_handle = HCI_CON_HANDLE_INVALID;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void midi_service_stream_init(btstack_packet_handler_t packet_handler)
{
    for (uint8_t idx = 0; idx < BLE_MIDI_SERVER_MAX_CONNECTIONS; idx++) {
        midi_service_stream_connection_t* context = midi_service_stream_connection+idx;
        context->ble_midi_pkt_codec_data = ble_midi_pkt_codec_get_data_by_index(idx);
        context->connection_handle = HCI_CON_HANDLE_INVALID;
        ble_midi_pkt_codec_init_data(context->ble_midi_pkt_codec_data, MAX_BLE_MIDI_PACKET);
        char name[] = "MIDI A";
        name[5] += idx;
        strcpy(context->name, name);
    }
    // register for HCI events
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    att_server_register_packet_handler(att_packet_handler);
    midi_service_server_init(midi_service_stream_packet_handler);
    client_packet_handler = packet_handler;
}

void midi_service_stream_deinit()
{
    hci_remove_event_handler(&hci_event_callback_registration);
}

uint8_t midi_service_stream_write(hci_con_handle_t con_handle, uint8_t nbytes, const uint8_t* midi_stream_bytes)
{
    midi_service_stream_connection_t* context = get_context_for_conn_handle(con_handle);
    uint16_t npushed = 0;
    if (context) {
        bool ready_to_send = false;
        npushed = ble_midi_pkt_codec_push_midi(midi_stream_bytes, nbytes, context->ble_midi_pkt_codec_data, &ready_to_send);
        if (ready_to_send) {
            midi_service_server_request_can_send_now(&context->send_request, context->connection_handle);
        }
    }
    return npushed;
}

uint8_t midi_service_stream_read(hci_con_handle_t con_handle, uint8_t max_bytes, uint8_t* midi_stream_bytes, uint16_t* timestamp)
{
    midi_service_stream_connection_t* context = get_context_for_conn_handle(con_handle);
    ble_midi_message_t mes;
    uint8_t nread = 0;
    if (context != NULL) {
        uint8_t nbytes = ble_midi_pkt_codec_pop_midi(&mes, context->ble_midi_pkt_codec_data);
        if (nbytes == sizeof(mes)) {
            uint8_t bytecount = mes.nbytes & ble_midi_packet_nbytes_mask;
            if (bytecount <= max_bytes) {
                *timestamp = mes.timestamp_ms;
                memcpy(midi_stream_bytes, mes.msg_bytes, bytecount);
                nread = bytecount;
            }
        }
    }

    return nread;
}
