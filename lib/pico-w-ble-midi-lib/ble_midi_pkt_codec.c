/**
 * This file contains code to
 * - encode MIDI byte streams to BLE-MIDI 1.0 packets and store the encoded packets
 *   in a byte stream.
 * - decode BLE-MIDI 1.0 packets into a series of timestamped MIDI 1.0 byte
 *   stream messages
 *
 * To the be of my knowledge, this code is an orignal work.
 *
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
#include "ble_midi_pkt_codec.h"
#include "ring_buffer_lib.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <stdio.h>

#define MIDI_SERVICE_HEADER(x) (0x80 | (((x) >> 7) & 0x3F) )
#define MIDI_SERVICE_TIMESTAMP_LOW(x) (0x80 | ((x) & 0x7F))

// This structure is used to hold data associated with encoding a MIDI 1.0
// byte stream into BLE-MIDI 1.0 formatted data packets
typedef struct to_ble_midi_stream_s {
    ble_midi_message_t mes;                     // intermediate encoding of message stream into message packets
    uint16_t next_msg_byte_idx;                 // the next status or data byte will be written to mes.msg_bytes[next_msg_byte_idx]
    uint16_t previous_timestamp;                // the timestamp of the last MIDI status byte encoded to be sent in pending_ble_pkt
    uint8_t running_status;                     // the MIDI message stream channel message running status
    uint8_t npending_rt;                        // number of pending real-time messages
    uint8_t pending_rt_status[4];               // buffer of pending real-time messages
    uint16_t pending_rt_timestamp[4];           // buffer of pending real-time message timestamps
    ble_midi_packet_t pending_ble_pkt;          // packet currently being encoded from the message stream
    uint8_t pending_ble_midi_pkt_running_status; // the channel message running status of the pending BLE-MIDI packet
    uint8_t pending_ble_midi_pkt_prev_status;   // the last encoded status message for the ble midi packet
} to_ble_midi_stream_t;

struct ble_midi_codec_data_s {
    // data packets sent to the Blueooth stack are stored here
    uint8_t to_ble_buffer_storage[sizeof(ble_midi_packet_t) * 10];
    to_ble_midi_stream_t to_ble_midi_stream;
    // parsed messages received from the Blueooth stack are stored here
    uint8_t from_ble_buffer_storage[sizeof(ble_midi_message_t) * 100];
    ring_buffer_t to_ble;
    ring_buffer_t from_ble;
    uint16_t ble_mtu;
};

static ble_midi_codec_data_t ble_midi_codec_data[BLE_MIDI_SERVER_MAX_CONNECTIONS];

static uint16_t midi_service_timestamp_decode(uint16_t* msb, uint8_t lsb, uint8_t* prev_lsb)
{
    // time has to either stand still or go forward
    if (*prev_lsb > lsb) {
        *msb += 1;
    }
    *prev_lsb = lsb;
    return ((*msb) | ((lsb) & 0x7F));
}

ble_midi_codec_data_t* ble_midi_pkt_codec_get_data_by_index(uint8_t idx)
{
    ble_midi_codec_data_t* context = NULL;
    if (idx < BLE_MIDI_SERVER_MAX_CONNECTIONS) {
        context = ble_midi_codec_data + idx;
        
    }
    return context;
}


static void ble_midi_pkt_codec_init_ring_buffers(ble_midi_codec_data_t* context)
{
    context->to_ble_midi_stream.next_msg_byte_idx = 0;
    context->to_ble_midi_stream.previous_timestamp = 0xffff; // illegal value
    context->to_ble_midi_stream.running_status = 0;
    context->to_ble_midi_stream.mes.nbytes = 0;
    context->to_ble_midi_stream.mes.timestamp_ms = 0xffff;
    context->to_ble_midi_stream.npending_rt = 0;
    context->to_ble_midi_stream.pending_ble_pkt.nbytes = 0;
    ring_buffer_init(&context->from_ble, (uint8_t *)context->from_ble_buffer_storage, sizeof(context->from_ble_buffer_storage), 0);
    ring_buffer_init(&context->to_ble, (uint8_t *)context->to_ble_buffer_storage, sizeof(context->to_ble_buffer_storage), 0);
}

void ble_midi_pkt_codec_init_data(ble_midi_codec_data_t* context, uint16_t ble_mtu)
{
    ble_midi_pkt_codec_set_mtu(context, ble_mtu);
    ble_midi_pkt_codec_init_ring_buffers(context);
}

void ble_midi_pkt_codec_set_mtu(ble_midi_codec_data_t* context, uint16_t ble_mtu)
{
    context->ble_mtu = ble_mtu;
}

void ble_midi_pkt_codec_update_mtu(ble_midi_codec_data_t* context, uint16_t ble_mtu)
{
    context->ble_mtu = MIN(ble_mtu, sizeof(context->to_ble_midi_stream.pending_ble_pkt.pkt));
}

uint16_t ble_midi_pkt_codec_get_mtu(ble_midi_codec_data_t* context)
{
    return context->ble_mtu;
}

static uint16_t midi_service_stream_get_system_13_bit_ms_timestamp()
{
    return (uint16_t)((time_us_32()/1000) & 0x1FFF);
}

static void midi_service_stream_add_header_if_needed(ble_midi_packet_t* pkt, uint16_t timestamp)
{
    if (pkt->nbytes == 0) {
        pkt->pkt[pkt->nbytes++] = MIDI_SERVICE_HEADER(timestamp);
    }
}

static void midi_service_stream_flush_rt_midi_to_pkt(ble_midi_codec_data_t* context)
{
    to_ble_midi_stream_t* ble_midi_stream = &context->to_ble_midi_stream;
    for (uint8_t idx = 0; idx < ble_midi_stream->npending_rt; idx++) {
        // make sure there is room in the ATT packet to store the data
        if ((ble_midi_stream->pending_ble_pkt.nbytes + 2) >= context->ble_mtu) {
            ring_buffer_push(&context->to_ble, (uint8_t*)&ble_midi_stream->pending_ble_pkt, sizeof(ble_midi_stream->pending_ble_pkt));
            ble_midi_stream->pending_ble_pkt.nbytes = 0;
            ble_midi_stream->pending_ble_midi_pkt_running_status = 0;
            ble_midi_stream->pending_ble_midi_pkt_prev_status = 0;
        }
        if (ble_midi_stream->pending_ble_pkt.nbytes == 0) {
            ble_midi_stream->pending_ble_pkt.pkt[ble_midi_stream->pending_ble_pkt.nbytes++] = MIDI_SERVICE_HEADER(ble_midi_stream->pending_rt_timestamp[0]);
        }
        ble_midi_stream->pending_ble_pkt.pkt[ble_midi_stream->pending_ble_pkt.nbytes++] = MIDI_SERVICE_TIMESTAMP_LOW(ble_midi_stream->pending_rt_timestamp[idx]);
        ble_midi_stream->pending_ble_pkt.pkt[ble_midi_stream->pending_ble_pkt.nbytes++] = ble_midi_stream->pending_rt_status[idx];
        ble_midi_stream->pending_ble_midi_pkt_prev_status = ble_midi_stream->pending_rt_status[idx];
    }
    ble_midi_stream->npending_rt = 0;
}

static void midi_service_stream_encode_bt_pkt(ble_midi_codec_data_t* context)
{
    to_ble_midi_stream_t* ble_midi_stream = &context->to_ble_midi_stream;
    uint8_t nbytes = ble_midi_stream->mes.nbytes & ble_midi_packet_nbytes_mask;
    if (nbytes == 0)
        return;
    if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_real_time) != ble_midi_packet_is_real_time) {
        midi_service_stream_flush_rt_midi_to_pkt(context);
    }

    // channel messages for which the midi packet running status match the status byte running status may omit byte 0 of msg_bytes
    bool requires_byte0 = (((ble_midi_stream->mes.nbytes & ble_midi_packet_is_channel) != ble_midi_packet_is_channel) ||
             (ble_midi_stream->pending_ble_midi_pkt_running_status != ble_midi_stream->mes.msg_bytes[0]));

    // Need to start the packet with a timestamp unless it is just sysex data or if
    // it is a channel message running status data with the same
    // timestamp as the previous timestamp not preceded by another status message.
    bool needs_timestamp = 
     (((ble_midi_stream->mes.nbytes & (ble_midi_packet_is_sysex | ble_midi_packet_is_start_sysex)) != ble_midi_packet_is_sysex) &&
     (requires_byte0 || ble_midi_stream->previous_timestamp != ble_midi_stream->mes.timestamp_ms));
    if (!needs_timestamp && ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_channel) == ble_midi_packet_is_channel) &&
        ble_midi_stream->pending_ble_midi_pkt_prev_status != ble_midi_stream->pending_ble_midi_pkt_running_status) {
        needs_timestamp = true;
    }
    uint8_t first_byte_idx = (requires_byte0 ? 0:1);
    uint8_t total_bytes = nbytes + (needs_timestamp ? 1:0) - first_byte_idx;
    if ((ble_midi_stream->pending_ble_pkt.nbytes + total_bytes) >= context->ble_mtu) {
        ring_buffer_push(&context->to_ble, (uint8_t*)&ble_midi_stream->pending_ble_pkt, sizeof(ble_midi_stream->pending_ble_pkt));
        ble_midi_stream->pending_ble_pkt.nbytes = 0;
        ble_midi_stream->pending_ble_midi_pkt_running_status = 0;
        ble_midi_stream->pending_ble_midi_pkt_prev_status = 0;
        needs_timestamp = true;
        requires_byte0 = true;
    }
    if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_channel) != 0) {
        ble_midi_stream->pending_ble_midi_pkt_running_status = ble_midi_stream->mes.msg_bytes[0];
    }
    // update the previously encoded status byte
    if (ble_midi_stream->mes.msg_bytes[0] & 0x80) {
        ble_midi_stream->pending_ble_midi_pkt_prev_status = ble_midi_stream->mes.msg_bytes[0];
    }
    midi_service_stream_add_header_if_needed(&ble_midi_stream->pending_ble_pkt, ble_midi_stream->mes.timestamp_ms);
    if (needs_timestamp) {
        ble_midi_stream->pending_ble_pkt.pkt[ble_midi_stream->pending_ble_pkt.nbytes++] = MIDI_SERVICE_TIMESTAMP_LOW(ble_midi_stream->mes.timestamp_ms);
    }

    for (uint8_t idx = first_byte_idx; idx < nbytes; idx++) {
        ble_midi_stream->pending_ble_pkt.pkt[ble_midi_stream->pending_ble_pkt.nbytes++] = ble_midi_stream->mes.msg_bytes[idx];
    }
}

static void midi_service_stream_encode_full_bt_pkt(ble_midi_codec_data_t* context)
{
    midi_service_stream_encode_bt_pkt(context);
    context->to_ble_midi_stream.next_msg_byte_idx = 0;
    //midi_service_stream_flush_rt_midi_to_pkt(context);
}


static void midi_service_stream_discard_stream(const uint8_t* midi_stream, uint16_t nbytes, to_ble_midi_stream_t* ble_midi_stream)
{
    printf("MIDI stream error\r\ndiscarding unsent MIDI bytes:\r\n");
    ble_midi_stream->next_msg_byte_idx = 0;
    ble_midi_stream->pending_ble_pkt.nbytes = 0;
    ble_midi_stream->previous_timestamp = 0xffff; // illegal value
    ble_midi_stream->running_status = 0;
    ble_midi_stream->mes.nbytes = 0;
    ble_midi_stream->mes.timestamp_ms = 0xffff;
    ble_midi_stream->npending_rt = 0;
    while(nbytes--) {
        printf("%02x ", *midi_stream++);
    }
    printf("\r\n");
}

uint16_t ble_midi_pkt_codec_push_midi(const uint8_t* midi_stream, uint16_t nbytes, ble_midi_codec_data_t* context, bool* ready_to_send)
{
    uint16_t timestamp = midi_service_stream_get_system_13_bit_ms_timestamp();
    uint16_t bytes_pushed = 0;
    *ready_to_send = false;
    to_ble_midi_stream_t* ble_midi_stream = &context->to_ble_midi_stream;
    while (bytes_pushed < nbytes && ble_midi_stream->pending_ble_pkt.nbytes < context->ble_mtu) {
        uint8_t ms_byte = midi_stream[bytes_pushed];
#ifdef BLE_MIDI_SERVER_FILTER_MIDI_CLOCK_TO_BLE
        if (ms_byte == 0xf8) {
            ++bytes_pushed;
            continue;
        }
#endif
#ifdef BLE_MIDI_SERVER_FILTER_ACTIVE_SENSING_TO_BLE
        if (ms_byte == 0xfe) {
            // filter out for now
            ++bytes_pushed;
            continue;
        }
#endif
        if (ms_byte & 0x80) {
            // status byte; potential start of message
            if (ms_byte == 0xF0) {
                ble_midi_stream-> previous_timestamp = timestamp;
                // start of system exclusive
                ble_midi_stream->mes.nbytes = ble_midi_packet_is_start_sysex | ble_midi_packet_is_sysex | 1;
                ++bytes_pushed;
                ble_midi_stream->running_status = 0;
                ble_midi_stream->mes.msg_bytes[0] = ms_byte;
                ble_midi_stream->next_msg_byte_idx = 1;
            }
            else if (ms_byte == 0xF7) {
                if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_sysex) == 0) {
                    // terminated a sysex message without being inside a sysex message.
                    // Something went wrong with the MIDI stream or this design
                    // Discard the pending message and pending real-time messages
                    midi_service_stream_discard_stream(midi_stream, nbytes, ble_midi_stream);

                    return bytes_pushed;
                }
                // end of system exclusive
                if (ble_midi_stream->next_msg_byte_idx != 0) {
                    // encode all the sysex data bytes and the potential start of sysex
                    midi_service_stream_encode_bt_pkt(context);
                }
                // timestamp is the same as the start of sysex or after the last real-time message
                ble_midi_stream->mes.timestamp_ms =  ble_midi_stream->previous_timestamp;
                ble_midi_stream->mes.msg_bytes[0] = ms_byte;
                ble_midi_stream->mes.nbytes = 1;
                ++bytes_pushed;
                midi_service_stream_encode_full_bt_pkt(context);
                ble_midi_stream->running_status = 0;
            }
            else if (ms_byte >= 0xF8) {
                if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_sysex) != 0 && (ble_midi_stream->mes.nbytes & ble_midi_packet_nbytes_mask) > 0) {
                    // empty out the pending packet; it is OK for real-time to occur anywhere inside a sysex message
                    midi_service_stream_encode_bt_pkt(context);
                    context->to_ble_midi_stream.next_msg_byte_idx = 0;
                    ble_midi_stream->mes.nbytes = ble_midi_packet_is_sysex;
                }
                ble_midi_stream->pending_rt_status[ble_midi_stream->npending_rt] = ms_byte;
                ble_midi_stream->pending_rt_timestamp[ble_midi_stream->npending_rt++] = timestamp;
                ++bytes_pushed;
                if (ble_midi_stream->npending_rt >= sizeof(ble_midi_stream->pending_rt_status)) {
                    // Something went wrong with the MIDI stream or this design
                    // Discard the pending message and pending real-time messages
                    midi_service_stream_discard_stream(midi_stream, nbytes, ble_midi_stream);

                    return bytes_pushed;
                }
                midi_service_stream_flush_rt_midi_to_pkt(context);
            }
            else if (ms_byte > 0xF0) {
                // system common message
                ble_midi_stream->mes.msg_bytes[0] = ms_byte;
                ble_midi_stream->previous_timestamp = timestamp;
                ble_midi_stream->mes.timestamp_ms = timestamp;
                ++bytes_pushed;
                ble_midi_stream->running_status = 0;
                if (ms_byte == 0xF1 || ms_byte == 0xF3) {
                    ble_midi_stream->mes.nbytes = 2;
                    ble_midi_stream->next_msg_byte_idx = 1;
                }
                else if (ms_byte == 0xF2) {
                    ble_midi_stream->mes.nbytes = 3;
                    ble_midi_stream->next_msg_byte_idx = 1;
                }
                else {
                    ble_midi_stream->mes.nbytes = 1;
                    midi_service_stream_encode_full_bt_pkt(context);
                    ble_midi_stream->mes.nbytes = 0;
                }
            }
            else {
                // channel_message
                ble_midi_stream->running_status = ms_byte;
                ble_midi_stream->mes.msg_bytes[0] = ms_byte;
                ble_midi_stream->previous_timestamp = timestamp;
                ble_midi_stream->mes.timestamp_ms = timestamp;

                ++bytes_pushed;
                ble_midi_stream->next_msg_byte_idx = 1;
                ble_midi_stream->mes.nbytes = ble_midi_packet_is_channel | 3;
                uint8_t chan_msg_base = (ms_byte >> 4) & 0xF;
                if (chan_msg_base == 0xC || chan_msg_base == 0xD) {
                    ble_midi_stream->mes.nbytes = ble_midi_packet_is_channel | 2;
                }
            }
        } // end parsing status
        else {
            // data byte
            if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_sysex) != 0) {
                // this is a byte in a system exclusive message
                ble_midi_stream->mes.msg_bytes[ble_midi_stream->next_msg_byte_idx++] = ms_byte;
                ++ble_midi_stream->mes.nbytes;
                ++bytes_pushed;
                if (ble_midi_stream->next_msg_byte_idx >= 3) {
                    //midi_service_stream_request_send(context);
                    midi_service_stream_encode_full_bt_pkt(context);
                    ble_midi_stream->mes.timestamp_ms = ble_midi_stream->previous_timestamp;
                    ble_midi_stream->mes.nbytes = ble_midi_packet_is_sysex; // set length to 0
                }
            }
            else if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_channel) != 0 && ble_midi_stream->next_msg_byte_idx == 0)  {
                if (ble_midi_stream->running_status == 0 || ble_midi_stream->running_status != ble_midi_stream->mes.msg_bytes[0]) {
                    // stream error
                    midi_service_stream_discard_stream(midi_stream, nbytes, ble_midi_stream);
                    return bytes_pushed;
                }
                else {
                    ble_midi_stream->mes.msg_bytes[1] = ms_byte;
                    ble_midi_stream->mes.timestamp_ms = timestamp;
                    ++bytes_pushed;
                    ble_midi_stream->next_msg_byte_idx = 2;
                    if ((ble_midi_stream->mes.nbytes & ble_midi_packet_nbytes_mask) == ble_midi_stream->next_msg_byte_idx) {
                        midi_service_stream_encode_full_bt_pkt(context);
                    }
                }
            }
            else {
                // normal message data bytes or a stream error
                uint8_t len = (ble_midi_stream->mes.nbytes & ble_midi_packet_nbytes_mask);
                if (ble_midi_stream->next_msg_byte_idx < len) {
                    ble_midi_stream->mes.msg_bytes[ble_midi_stream->next_msg_byte_idx++] = ms_byte;
                    ++bytes_pushed;
                    if (len == ble_midi_stream->next_msg_byte_idx) {
                        midi_service_stream_encode_full_bt_pkt(context);
                        if ((ble_midi_stream->mes.nbytes & ble_midi_packet_is_channel) == 0) {
                            ble_midi_stream->mes.nbytes = 0;
                        }
                    }
                }
                else {
                    // stream error
                    midi_service_stream_discard_stream(midi_stream, nbytes, ble_midi_stream);
                    return bytes_pushed;
                }
            }
        }
    }
    // parsed the full MIDI stream sent
    if (bytes_pushed > 0) {
        if (ble_midi_stream->pending_ble_pkt.nbytes > 0) {
            ring_buffer_push(&context->to_ble, (uint8_t*)&ble_midi_stream->pending_ble_pkt, sizeof(ble_midi_stream->pending_ble_pkt));
            ble_midi_stream->pending_ble_pkt.nbytes = 0;
            ble_midi_stream->pending_ble_midi_pkt_running_status = 0;
            ble_midi_stream->pending_ble_midi_pkt_prev_status = 0;
            *ready_to_send = true;
        }
    }
    return bytes_pushed;
}

static bool midi_service_stream_push(ring_buffer_t* buf, uint8_t* data, RING_BUFFER_SIZE_TYPE size)
{
    bool success = false;
    RING_BUFFER_SIZE_TYPE after_push = ring_buffer_get_num_bytes(buf);
    after_push += size;
    if (buf->bufsize >= after_push) {
        ring_buffer_push(buf, data, size);
        success = true;
    }
    return success;
}

static bool ble_midi_pkt_codec_decode_sysex_data(const uint8_t* pkt, uint16_t nbytes, ble_midi_codec_data_t* context, ble_midi_message_t* mes, uint16_t* ndecoded, uint8_t idx)
{
    while (*ndecoded < nbytes && (pkt[*ndecoded] & 0x80) == 0) {
        mes->msg_bytes[idx++] = pkt[(*ndecoded)++];
        mes->nbytes++;
        if (idx == 3) {
            if (!midi_service_stream_push(&context->from_ble, (uint8_t*)mes, sizeof(*mes))) {
                *ndecoded -= 3;
                return false;
            }
            idx = 0;
            mes->nbytes = ble_midi_packet_is_sysex;
        }
    }
    uint8_t bytecount = (mes->nbytes & ble_midi_packet_nbytes_mask);
    if (bytecount > 0) {
        if (!midi_service_stream_push(&context->from_ble, (uint8_t*)mes, sizeof(*mes))) {
            *ndecoded -= bytecount;
            return false;
        }
        // keep track of unterminated sysex message
        mes->nbytes = ble_midi_packet_is_sysex;
    }
    return true;
}

uint16_t ble_midi_pkt_codec_ble_midi_decode_push(const uint8_t* pkt, uint16_t nbytes, ble_midi_codec_data_t* context)
{
    // The shortest possible packet would be a packet header with one sysex byte
    if (pkt == NULL || nbytes < 2) {
        return 0;
    }
    // make sure the header byte is present
    if ((pkt[0] & 0xc0) != 0x80) {
        return 0;
    }
    uint16_t timestamp = ((uint16_t)(pkt[0] & 0x3f)) << 7;
    uint8_t prev_lsb = 0;
    uint16_t ndecoded = 1;
    uint8_t running_status = 0;
    uint8_t running_status_nbytes = 0;
    ble_midi_message_t mes = {0, {0,0,0}, 0};
    // check to see if the start of the packet is sysex continuation data
    if ((pkt[ndecoded] & 0x80) == 0) {
        // This has to be sysex continuation data or an error that we can't detect easily
        mes.nbytes = ble_midi_packet_is_sysex;
        if (!ble_midi_pkt_codec_decode_sysex_data(pkt, nbytes, context, &mes, &ndecoded, 0)) {
            return ndecoded;
        }
    }
    while (ndecoded < nbytes) {
        // message start usually is a timestamp followed by a status byte
        if (ndecoded+1 < nbytes) {
            if ((pkt[ndecoded] & 0x80) != 0 && (pkt[ndecoded+1] & 0x80) != 0) {
                // As long as it is not a sysex message, this is a timestamp followed by a status byte
                if ((mes.nbytes & ble_midi_packet_is_sysex) == 0) {
                    // 1st byte is the timestamp; 2nd byte is status byte
                    mes.timestamp_ms = midi_service_timestamp_decode(&timestamp, pkt[ndecoded++], &prev_lsb);
                    mes.msg_bytes[0] = pkt[ndecoded++];
                    if (mes.msg_bytes[0] >= 0xF8) {
                        mes.nbytes = ble_midi_packet_is_real_time + 1;
                        if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                            return ndecoded;
                        }
                    }
                    else if (mes.msg_bytes[0] > 0xF0 && mes.msg_bytes[0] <= 0xF7) {
                        // system common
                        if (mes.msg_bytes[0] == 0xF1 || mes.msg_bytes[0] == 0xF3) {
                            mes.nbytes = 2;
                        }
                        else if (mes.msg_bytes[0] == 0xF2) {
                            mes.nbytes = 3;
                        }
                        else {
                            mes.nbytes = 1;
                        }
                        if ((ndecoded + mes.nbytes - 1) <= nbytes) {
                            for (uint8_t idx = 1; idx < mes.nbytes; idx++) {
                                mes.msg_bytes[idx] = pkt[ndecoded++];
                            }
                            if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                                --ndecoded;
                                return ndecoded;
                            }
                        }
                        else {
                            return ndecoded;
                        }
                    }
                    else if (mes.msg_bytes[0] < 0xF0) {
                        // channel message
                        running_status = mes.msg_bytes[0];
                        uint8_t stat = (mes.msg_bytes[0] >> 4) & 0xF; // ignore the channel
                        if (stat == 0xC || stat == 0xD) {
                            mes.nbytes = 2;
                            running_status_nbytes = 2;
                        }
                        else {
                            running_status_nbytes = 3;
                        }
                        if ((running_status_nbytes + ndecoded -1) <= nbytes) {
                            for (uint8_t idx = 1; idx < running_status_nbytes; idx++) {
                                mes.msg_bytes[idx] = pkt[ndecoded++];
                            }
                            mes.nbytes = running_status_nbytes | ble_midi_packet_is_channel;
                            if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                                --ndecoded;
                                return ndecoded;
                            }
                        }
                    }
                    else { // 0xF0; start of sysex
                        // keep decoding until encountering a new timestamp.
                        mes.nbytes = ble_midi_packet_is_sysex + 1;
                        if (!ble_midi_pkt_codec_decode_sysex_data(pkt, nbytes, context, &mes, &ndecoded, 1)) {
                            return ndecoded;
                        }
                    }
                }
                else if (pkt[ndecoded+1] >= 0xF8) {
                    uint8_t bytecount = (mes.nbytes & ble_midi_packet_nbytes_mask);
                    if (bytecount > 0) {
                        if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                            ndecoded -= bytecount;
                            return ndecoded;
                        }
                    }
                    // This is a real-time message timestamp and status byte within a sysex message
                    ble_midi_message_t mes_rt = {0, {0,0,0}, 0};
                    mes_rt.nbytes = ble_midi_packet_is_real_time + 1;
                    mes_rt.timestamp_ms = midi_service_timestamp_decode(&timestamp, pkt[ndecoded++], &prev_lsb);
                    mes_rt.msg_bytes[0] = pkt[ndecoded++];
                    if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes_rt, sizeof(mes_rt))) {
                        ndecoded -= 2;
                        return ndecoded;
                    }
                    // real-time packet is out of the way, continue parsing the data
                    mes.nbytes = ble_midi_packet_is_sysex;
                    uint8_t idx = 0;
                    while (ndecoded < nbytes && (pkt[ndecoded] & 0x80) == 0) {
                        mes.msg_bytes[idx++] = pkt[ndecoded++];
                        mes.nbytes++;
                        if (idx == 3) {
                            if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                                ndecoded -= 3;
                                return ndecoded;
                            }
                            idx = 0;
                            mes.nbytes = ble_midi_packet_is_sysex;
                        }
                    }
                    bytecount = (mes.nbytes & ble_midi_packet_nbytes_mask);
                    if (bytecount > 0) {
                        if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                            ndecoded -= bytecount;
                            return ndecoded;
                        }
                        // if MSB==1 in the BLE packet, it could be a real-time message's timestamp or EOX message's timestamp
                        mes.nbytes = ble_midi_packet_is_sysex;
                    }
                }
                else if (pkt[ndecoded+1] == 0xF7) {
                    // was sysex, now sysex is over; push the last data packet, then the F7 packet on the next iteration.
                    uint8_t bytecount = (mes.nbytes & ble_midi_packet_nbytes_mask);
                    if (bytecount > 0) {
                        if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                            ndecoded -= bytecount;
                            return ndecoded;
                        }
                    }
                    mes.nbytes = 0; // clear is_sysex flag
                }
            }
            else if ((pkt[ndecoded] & 0x80) != 0 && (pkt[ndecoded+1] & 0x80) == 0) {
                // This pattern could be a channel message running status timestamp followed by the channel message data byte(s)
                if (running_status != 0 && (ndecoded + running_status_nbytes) <= nbytes) {
                    mes.timestamp_ms = midi_service_timestamp_decode(&timestamp, pkt[ndecoded++], &prev_lsb);
                    mes.msg_bytes[0] = running_status;
                    mes.nbytes = running_status_nbytes | ble_midi_packet_is_channel;
                    for (uint8_t idx = 1; idx < running_status_nbytes; idx++) {
                        mes.msg_bytes[idx] = pkt[ndecoded++];
                    }
                    if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                        ndecoded -= running_status_nbytes;
                        return ndecoded;
                    }
                }
                else {
                    // the pattern of timestamp plus not-timestamp is not legal otherwise at the beginning of a message
                    return ndecoded;
                }
            }
            else if ((pkt[ndecoded] & 0x80) == 0 && (pkt[ndecoded+1] & 0x80) == 0) {
                if (running_status == mes.msg_bytes[0] && (mes.nbytes & ble_midi_packet_is_channel) != 0 &&  (ndecoded + running_status_nbytes - 1) <= nbytes) {
                    // this is a running status channel message same as previous message with the same timestamp and new data
                    // status byte is already correct, and the there are enough data bytes in the undecoded (at least the full message length - 1)
                    for (uint8_t idx = 1; idx < running_status_nbytes; idx++) {
                        mes.msg_bytes[idx] = pkt[ndecoded++];
                    }
                    if (!midi_service_stream_push(&context->from_ble, (uint8_t*)&mes, sizeof(mes))) {
                        ndecoded -= (running_status_nbytes-1);
                        return ndecoded;
                    }
                }
                else {
                    // this should not happen
                    return ndecoded;
                }
            }
        }
    }
    return ndecoded;
}

uint16_t ble_midi_pkt_codec_pop_midi(ble_midi_message_t* mes, ble_midi_codec_data_t* context)
{
    uint8_t nbytes = 0;
    if (context != NULL && ring_buffer_get_num_bytes(&context->from_ble) >= sizeof(*mes)) {
        nbytes = ring_buffer_pop(&context->from_ble, (uint8_t*)mes, sizeof(*mes));
    }
    return nbytes;
}

uint16_t ble_midi_pkt_codec_ble_pkt_pop(ble_midi_packet_t* pkt, ble_midi_codec_data_t* context)
{
    return ring_buffer_pop(&context->to_ble, (uint8_t*)pkt, sizeof(*pkt));
}

bool ble_midi_pkt_codec_ble_pkt_available(ble_midi_codec_data_t* context)
{
    return ring_buffer_get_num_bytes(&context->to_ble) >= sizeof(ble_midi_packet_t);
}