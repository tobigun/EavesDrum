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
#include "midi_service_server.h"
#ifdef __cplusplus
    extern "C" {
#endif
/**
 * @brief initialize the MIDI service and MIDI parser/packet handlers
 * 
 */
void midi_service_stream_init(btstack_packet_handler_t packet_handler);

/**
 * @brief Shut down this service (e.g., to switch to client mode)
 */
void midi_service_stream_deinit();
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
uint8_t midi_service_stream_write(hci_con_handle_t con_handle, uint8_t nbytes, const uint8_t* midi_stream_bytes);

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
uint8_t midi_service_stream_read(hci_con_handle_t con_handle, uint8_t max_bytes, uint8_t* midi_stream_bytes, uint16_t* timestamp);
#ifdef __cplusplus
}
#endif
