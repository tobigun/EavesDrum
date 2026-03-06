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
#include "btstack_config.h" // for ENABLE_LE_DATA_LENGTH_EXTENSION & BLE_MIDI_SERVER_MAX_CONNECTIONS
#include "bluetooth.h"  // for ATT_DEFAULT_MTU

#if defined __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif
// This driver makes use of the LE Data Packet Length Extension feature to improve throughput
#ifdef ENABLE_LE_DATA_LENGTH_EXTENSION
#define MAX_BLE_MIDI_PACKET 251-3
#else
#define MAX_BLE_MIDI_PACKET ATT_DEFAULT_MTU-3
#endif

// This structure is used to hold BLE-MIDI 1.0 data packets in a ring_buffer
typedef struct ble_midi_packet_s {
    uint16_t nbytes;
    uint8_t pkt[MAX_BLE_MIDI_PACKET];
} __attribute__((packed)) ble_midi_packet_t;

// This structure contains the BLE-MIDI encoder/decoder context data for every connection
// The definition is opaque to other applications
typedef struct ble_midi_codec_data_s ble_midi_codec_data_t;

/**
 * This structure defines a timestamped fragment of a MIDI message.
 * The upper 5 bits of nbytes encodes what sort of message is contained in msg_bytes[]
 * The lower 2 bits of nbytes encodes the number of valid bytes in msg_bytes[]
 * Only one byte per message is allowed.
 */
typedef struct ble_midi_message_s {
    uint8_t nbytes;         //!< Bit 7 is set if msg_bytes[0] contains 0xF0 to start a system exclusive message
                            //!< Bit 6 is reserved
                            //!< Bit 5 is set if the msg_bytes[] data contains payload bytes of a system exclusive message
                            //!< Bit 4 is set if msg_bytes[] contains a system real-time status byte
                            //!< Bit 3 is set if msg_bytes[] contains a channel status byte and data bytes; running status is not supported
                            //!< Bits 7:3 all clear means msg_bytes[] contains a system common status byte and data bytes.
                            //!< Bit 2 is reserved
                            //!< Bits 1:0 holdthe number of valid bytes in msg_bytes.
    uint8_t msg_bytes[3];   //!< msg_bytes[0:(nbytes & 0x7)-1] contains the timestamped data.
    uint16_t timestamp_ms;  //!< BLE MIDI timestamps are 13 bits; this value is 14 bits in case the timestamp wraps within one packet.
} __attribute__((packed)) ble_midi_message_t;

// Use the follow to set an clear flab bits stored in the ble_midi_message_t "nbytes" field
static const uint8_t ble_midi_packet_is_start_sysex = 0x80;
//static const uint8_t ble_midi_packet_reserved = 0x40;
static const uint8_t ble_midi_packet_is_sysex = 0x20;
static const uint8_t ble_midi_packet_is_real_time = 0x10;
static const uint8_t ble_midi_packet_is_channel = 0x08;
static const uint8_t ble_midi_packet_nbytes_mask = 0x03;

ble_midi_codec_data_t* ble_midi_pkt_codec_get_data_by_index(uint8_t idx);

void ble_midi_pkt_codec_init_data(ble_midi_codec_data_t* context, uint16_t ble_mtu);

void ble_midi_pkt_codec_update_mtu(ble_midi_codec_data_t* context, uint16_t ble_mtu);

void ble_midi_pkt_codec_set_mtu(ble_midi_codec_data_t* context, uint16_t ble_mtu);

uint16_t ble_midi_pkt_codec_get_mtu(ble_midi_codec_data_t* context);
/**
 * @brief push a MIDI stream to be encoded into the next BLE-MIDI 1.0 encoded packet
 * 
 * @param midi_stream a MIDI 1.0 byte stream (can be incomplete message)
 * @param nbytes number of bytes in the stream
 * @param context the data assocated with a BLE-MIDI 1.0 connection
 * @param ready_to_send true if a full MIDI packet is ready to be sent
 * @return uint16_t the number of bytes pushed
 */
uint16_t ble_midi_pkt_codec_push_midi(const uint8_t* midi_stream, uint16_t nbytes, ble_midi_codec_data_t* context, bool* ready_to_send);

/**
 * @brief pop the least recently pushed decoded ble_midi_message_t timestamped MIDI 1.0
 * message from the ring buffer
 * 
 * @param mes a pointer to storage for the message
 * @param context the data assocated with a BLE-MIDI 1.0 connection
 * @return uint16_t 0 if no message is in the ring buffer or sizeof(ble_midi_message_t) if there is
 */
uint16_t ble_midi_pkt_codec_pop_midi(ble_midi_message_t* mes, ble_midi_codec_data_t* context);

/**
 * @brief send a MIDI 1.0 encoded data packet to be decoded and cause each resulting timestamped
 * MIDI message to be pushed to the context's ring buffer.
 * 
 * @param pkt a BLE-MIDI 1.0 formatted packet
 * @param nbytes the number of bytes in the packet
 * @param context the data associated with a BLE-MIDI 1.0 connection
 * @return uint16_t the number of bytes from the pkt successfully pushed
 */
uint16_t ble_midi_pkt_codec_ble_midi_decode_push(const uint8_t* pkt, uint16_t nbytes, ble_midi_codec_data_t* context);

/**
 * @brief pop the next ble_midi_packet_t from the context's packet to send ring buffer
 * 
 * @param pkt a pointer to a structure to store the BLE-MIDI data packet and number of bytes in the packet
 * @param context the data associated with a BLE-MIDI 1.0 connection
 * @return uint16_t 0 if pop was unsuccessful or sizeof(ble_midi_packet_t) if it was
 */
uint16_t ble_midi_pkt_codec_ble_pkt_pop(ble_midi_packet_t* pkt, ble_midi_codec_data_t* context);

/**
 * @brief 
 * 
 * @param context the data associated with a BLE-MIDI 1.0 connection
 * @return true if there is at least one BLE-MIDI packet available to send
 */
bool ble_midi_pkt_codec_ble_pkt_available(ble_midi_codec_data_t* context);
#if defined __cplusplus
}
#endif