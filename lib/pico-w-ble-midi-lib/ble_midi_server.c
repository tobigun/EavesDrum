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
#include <stdio.h>
#include "ble_midi_server.h"
#include <inttypes.h>
#include <assert.h>
static hci_con_handle_t con_handle;
static const uint8_t APP_AD_FLAGS=0x06;
static const uint8_t adv_data[] = {
        // Flags general discoverable
        0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
        // Service class list
        0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7, 0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03,
};
static const uint8_t adv_data_len = sizeof(adv_data);
static uint8_t scan_resp_data[32];
static uint8_t scan_resp_data_len;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static bool initialized = false;
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    uint8_t event_type;
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint8_t status;
    // setup advertisements
    uint16_t adv_int_min = 800;
    uint16_t adv_int_max = 800;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    switch(packet_type) {
        case HCI_EVENT_PACKET:
            event_type = hci_event_packet_get_type(packet);
            switch(event_type){
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_OFF) {
                        midi_service_stream_deinit();
                        att_server_deinit();
                        sm_remove_event_handler(&sm_event_callback_registration);

                        sm_deinit();
                        btstack_crypto_deinit();
                        l2cap_deinit();
                        initialized = false;
                        break;
                    }
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) {
                        printf("unhandled BTSTACK_EVENT_STATE state=%u\r\n", btstack_event_state_get_state(packet));
                        return;
                    }
                    gap_local_bd_addr(local_addr);
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

                    memset(null_addr, 0, 6);
                    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
                    assert(adv_data_len <= 31); // ble limitation
                    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
                    assert(scan_resp_data_len <= 31); // ble limitation
                    gap_scan_response_set_data(scan_resp_data_len, (uint8_t*) scan_resp_data);
                    gap_advertisements_enable(1);

                    break;
                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    printf("ble server: HCI_EVENT_DISCONNECTION_COMPLETE event\r\n");
                    con_handle = HCI_CON_HANDLE_INVALID;
                    break;
                case HCI_EVENT_GATTSERVICE_META:
                    switch(hci_event_gattservice_meta_get_subevent_code(packet)) {
                        case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                            con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                            printf("ble server: GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED event handle = %u\r\n", con_handle);
                            break;
                        case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                            printf("ble server: GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED event\r\n");
                            con_handle = HCI_CON_HANDLE_INVALID;
                            break;
                        default:
                            break;
                    }
                    break;
                case SM_EVENT_JUST_WORKS_REQUEST:
                    printf("ble server: Just Works requested\n");
                    sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
                    break;
                case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
                    printf("ble server: Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
                    sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
                    break;
                case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
                    printf("ble server: Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
                    break;
                case SM_EVENT_IDENTITY_CREATED:
                    sm_event_identity_created_get_identity_address(packet, addr);
                    printf("ble server: Identity created: type %u address %s\n", sm_event_identity_created_get_identity_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
                    sm_event_identity_resolving_succeeded_get_identity_address(packet, addr);
                    printf("ble server: Identity resolved: type %u address %s\n", sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_IDENTITY_RESOLVING_FAILED:
                    sm_event_identity_created_get_address(packet, addr);
                    printf("ble server: Identity resolving failed\n");
                    break;
                case SM_EVENT_PAIRING_STARTED:
                    printf("ble server: Pairing started\n");
                    break;
                case SM_EVENT_PAIRING_COMPLETE:
                    switch (sm_event_pairing_complete_get_status(packet)){
                        case ERROR_CODE_SUCCESS:
                            printf("ble server: Pairing complete, success\n");
                            break;
                        case ERROR_CODE_CONNECTION_TIMEOUT:
                            printf("ble server: Pairing failed, timeout\n");
                            break;
                        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                            printf("ble server: Pairing failed, disconnected\n");
                            break;
                        case ERROR_CODE_AUTHENTICATION_FAILURE:
                            printf("ble server: Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                            break;
                        default:
                            break;
                    }
                    break;
                case SM_EVENT_REENCRYPTION_STARTED:
                    sm_event_reencryption_complete_get_address(packet, addr);
                    printf("ble server: Bonding information exists for addr type %u, identity addr %s -> re-encryption started\n",
                        sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_REENCRYPTION_COMPLETE:
                    switch (sm_event_reencryption_complete_get_status(packet)){
                        case ERROR_CODE_SUCCESS:
                            printf("ble server: Re-encryption complete, success\n");
                            break;
                        case ERROR_CODE_CONNECTION_TIMEOUT:
                            printf("ble server: Re-encryption failed, timeout\n");
                            break;
                        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                            printf("ble server: Re-encryption failed, disconnected\n");
                            break;
                        case ERROR_CODE_PIN_OR_KEY_MISSING:
                            printf("ble server: Re-encryption failed, bonding information missing\n\n");
                            printf("Assuming remote lost bonding information\n");
                            printf("Deleting local bonding information to allow for new pairing...\n");
                            sm_event_reencryption_complete_get_address(packet, addr);
                            addr_type = (bd_addr_type_t)(sm_event_reencryption_started_get_addr_type(packet));
                            gap_delete_bonding(addr_type, addr);
                            break;
                        default:
                            break;
                    }
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    status = gatt_event_query_complete_get_att_status(packet);
                    switch (status){
                        case ATT_ERROR_INSUFFICIENT_ENCRYPTION:
                            printf("ble server: GATT Query failed, Insufficient Encryption\n");
                            break;
                        case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                            printf("ble server: GATT Query failed, Insufficient Authentication\n");
                            break;
                        case ATT_ERROR_BONDING_INFORMATION_MISSING:
                            printf("ble server: GATT Query failed, Bonding Information Missing\n");
                            break;
                        case ATT_ERROR_SUCCESS:
                            printf("ble server: GATT Query successful\n");
                            break;
                        default:
                            printf("ble server: GATT Query failed, status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                            break;
                    }
                    break;
                default:
                    //printf("unhandled HCI_EVENT_PACKET event type=%u\r\n", event_type);
                    break;
            } // event_type
            break;
        default:
            //printf("unhandled packet type=%u\r\n", packet_type);
            break;
    } // HCI_PACKET
}

// TODO initialize scan response data and data length
void ble_midi_server_init(const uint8_t* profile_data, const uint8_t* resp_data, const uint8_t resp_data_len, io_capability_t iocaps, uint8_t secmask)
{
    scan_resp_data_len = resp_data_len;
    memcpy(scan_resp_data, resp_data, resp_data_len);
    if (initialized) {
        return; // already in server mode.
    }

    con_handle = HCI_CON_HANDLE_INVALID;
    l2cap_init();

    sm_init();

    // just works, legacy pairing, with bonding
    sm_set_io_capabilities(iocaps);
    sm_set_authentication_requirements(secmask);
    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);
    att_server_init(profile_data, NULL, NULL);
    midi_service_stream_init(sm_event_callback_registration.callback);

    // turn on bluetooth
    hci_power_control(HCI_POWER_ON);
    initialized = true;
}

void ble_midi_server_deinit()
{
    if (!initialized)
        return;
    // start a process that ends with HCI message BTSTACK_EVENT_STATE with state HCI_STATE_OFF
    hci_power_control(HCI_POWER_OFF);
#if 0
    midi_service_stream_deinit();
    att_server_deinit();
    sm_remove_event_handler(&sm_event_callback_registration);

    sm_deinit();
    btstack_crypto_deinit();
    l2cap_deinit();
    initialized = false;
#endif
}

uint8_t ble_midi_server_stream_read(uint8_t max_bytes, uint8_t* midi_stream_bytes, uint16_t* timestamp)
{
    if (ble_midi_server_is_connected())
        return midi_service_stream_read(con_handle, max_bytes, midi_stream_bytes, timestamp);
    return 0;
}

uint8_t ble_midi_server_stream_write(uint8_t nbytes, const uint8_t* midi_stream_bytes)
{
    if (ble_midi_server_is_connected())
        return midi_service_stream_write(con_handle, nbytes, midi_stream_bytes);
    return 0;
}


void ble_midi_server_request_disconnect()
{
    if (ble_midi_server_is_connected())
        gap_disconnect(con_handle);
}

bool ble_midi_server_is_connected()
{
    return con_handle != HCI_CON_HANDLE_INVALID;
}

bool ble_midi_server_is_initialized()
{
    return initialized;
}