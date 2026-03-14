#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include <pico/mutex.h>
#include "ble_midi_client.h"
#include "ble_midi_pkt_codec.h"
#include <inttypes.h>
#include <stdio.h>
#include "ble_midi_debug.h"

#include <Arduino.h>

#define CYW43_WL_GPIO_LED_PIN LED_BUILTIN
//#define LED_STATE(led_on) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on)
#define LED_STATE(led_on)

// Fixed passkey - used with sm_pairing_peripheral. Passkey is random in general
#define FIXED_PASSKEY 123456U

static enum {
    BLEMC_DEINIT = 0,
    BLEMC_IDLE,
    BLEMC_WAIT_FOR_SCAN_COMPLETE,
    BLEMC_WAIT_FOR_CONNECTION,
    BLEMC_WAIT_FOR_SERVICES,
    BLEMC_WAIT_FOR_CHARACTERISTICS,
    BLEMC_WAIT_FOR_ENABLE_NOTIFICATIONS_COMPLETE,
    BLEMC_WAIT_FOR_MIDI_DATA_RX,
    BLEMC_WAIT_FOR_DISCONNECTION,
} state = BLEMC_DEINIT;

typedef struct  {
    char name[BLE_MAX_DEVICE_NAME_LEN];
    uint8_t bdaddr[BLE_ADDRESS_LENGTH];
    uint8_t type; // BDADDR = 0; otherwise COMPLETE or SHORTENED
    uint8_t addr_type;
    int32_t timeout;
} Advertised_MIDI_Peripheral_t;

typedef struct {
    Advertised_MIDI_Peripheral_t midi_peripherals[BLEMC_MAX_SCAN_ITEMS];
    uint8_t n_midi_peripherals;
} BLEMC_client_t;

static uint32_t const scan_blink_timeout_ms = 500;
static int32_t const scan_remove_timeout = 6; // when decremented to 0, remove entry from midi_peripherals (in units of scan_blink_timeout_ms)
static btstack_packet_callback_registration_t sm_event_callback_registration;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_timer_source_t scan_timer;
static BLEMC_client_t midi_client;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint16_t conn_interval;
static uint8_t next_connect_bd_addr_type;
static uint8_t next_connect_bd_addr[6];
static uint8_t last_connected_bd_addr_type = BD_ADDR_TYPE_UNKNOWN;
static uint8_t last_connected_bd_addr[6];
static gatt_client_service_t midi_service;
static gatt_client_characteristic_t midi_data_io_characteristic;
static gatt_client_notification_t notification_listener;
static bool listener_registered = 0;
static ble_midi_codec_data_t* ble_midi_pkt_codec_data;
static btstack_context_callback_registration_t write_callback_registration;
static uint8_t *client_profile_data = NULL;
static io_capability_t iocaps;
static uint8_t secmask;
static bool midi_is_ready = false;
static bool keep_client_connected = false;
static ble_midi_client_scan_callback_t scan_callback = NULL;

static void printUUID(uint8_t * uuid128, uint16_t uuid16){
    if (uuid16){
        DEBUG_PRINTF("%04x",uuid16);
    } else {
        DEBUG_PRINTF("%s", uuid128_to_str(uuid128));
    }
}
static void dump_characteristic(gatt_client_characteristic_t * characteristic){
    DEBUG_PRINTF("    * characteristic: [0x%04x-0x%04x-0x%04x], properties 0x%02x, uuid ",
                            characteristic->start_handle, characteristic->value_handle, characteristic->end_handle, characteristic->properties);
    printUUID(characteristic->uuid128, characteristic->uuid16);
    DEBUG_PRINTF("\n");
}

static void dump_service(gatt_client_service_t * service){
    DEBUG_PRINTF("    * service: [0x%04x-0x%04x], uuid ", service->start_group_handle, service->end_group_handle);
    printUUID(service->uuid128, service->uuid16);
    DEBUG_PRINTF("\n");
}

static int find_midi_peripheral(BLEMC_client_t* blemc, uint8_t* bdaddr)
{
    uint8_t idx;
    for (idx = 0; idx < blemc->n_midi_peripherals && memcmp(blemc->midi_peripherals[idx].bdaddr, bdaddr, 6) != 0; idx++) {

    }
    return idx;
}

static void scan_timer_cb(btstack_timer_source_t* timer_)
{
    if (state != BLEMC_WAIT_FOR_SCAN_COMPLETE) {
        // should not get here
        return;
    }
    static bool led_on = true;

    led_on = !led_on;
    BLEMC_client_t *mp = (BLEMC_client_t*)(timer_->context);
    LED_STATE(led_on);
    // update the midi_peripheral list timeout field and delete entries with expired timers
    for (uint8_t idx = 0; idx < mp->n_midi_peripherals;) {
        if (--mp->midi_peripherals[idx].timeout <= 0) {
            mp->midi_peripherals[idx] = mp->midi_peripherals[mp->n_midi_peripherals - 1];
            mp->n_midi_peripherals--;
        }
        else {
            ++idx;
        }
    }

    if (scan_callback) {
        scan_callback();
    }

    // Restart timer
    btstack_run_loop_set_timer(timer_, scan_blink_timeout_ms);
    btstack_run_loop_add_timer(timer_);
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)packet_type;
    (void)channel;
    (void)size;
    static uint8_t midi_data_io_characteristic_uuid[16] = {0x77, 0x72, 0xE5, 0xDB, 0x38, 0x68, 0x41, 0x12, 0xA1, 0xA9, 0xF2, 0x66, 0x9D, 0x10, 0x6B, 0xF3};
    uint8_t att_status;
    uint16_t data_len;
    const uint8_t* data;
    uint16_t ndecoded;
    switch(hci_event_packet_get_type(packet)){
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            if (state != BLEMC_WAIT_FOR_SERVICES)
                break;
            gatt_event_service_query_result_get_service(packet, &midi_service);
            dump_service(&midi_service);
            break;
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            if (state == BLEMC_WAIT_FOR_CHARACTERISTICS) {
                gatt_event_characteristic_query_result_get_characteristic(packet, &midi_data_io_characteristic);
                dump_characteristic(&midi_data_io_characteristic);
            }
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            att_status = gatt_event_query_complete_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                DEBUG_PRINTF("SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                //state = BLEMC_WAIT_FOR_DISCONNECTION;
                //gap_disconnect(con_handle);
                break;  
            } 
            if (state == BLEMC_WAIT_FOR_SERVICES) {
                DEBUG_PRINTF("\nCHARACTERISTIC for SERVICE %s, [0x%04x-0x%04x]\n",
                    uuid128_to_str(midi_service.uuid128), midi_service.start_group_handle, midi_service.end_group_handle);
                state = BLEMC_WAIT_FOR_CHARACTERISTICS;
                gatt_client_discover_characteristics_for_service_by_uuid128(handle_gatt_client_event, con_handle, &midi_service, midi_data_io_characteristic_uuid);
                break;
            }
            else if (state == BLEMC_WAIT_FOR_CHARACTERISTICS) {
                DEBUG_PRINTF("found all characteristics query complete\r\n");
                state = BLEMC_WAIT_FOR_ENABLE_NOTIFICATIONS_COMPLETE;
                listener_registered = true;
                gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, con_handle, &midi_data_io_characteristic);
                if (ERROR_CODE_SUCCESS != gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, con_handle,
                    &midi_data_io_characteristic, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION)) {
                        DEBUG_PRINTF("failed to write client characteristic configuration\r\n");
                }
                else {
                    DEBUG_PRINTF("wrote client characteristic configuration\r\n");
                }
            }
            else if (state == BLEMC_WAIT_FOR_ENABLE_NOTIFICATIONS_COMPLETE) {
                state = BLEMC_WAIT_FOR_MIDI_DATA_RX;
                DEBUG_PRINTF("ready to receive MIDI data\r\n");
                LED_STATE(true);
                hci_connection_t * con = hci_connection_for_handle(con_handle);
                //DEBUG_PRINTF("HCI Connection: bdaddr=%s type=%u", bd_addr_to_str(con->address), con->address_type);
                last_connected_bd_addr_type = con->address_type;
                memcpy(last_connected_bd_addr, con->address, sizeof(last_connected_bd_addr));
                midi_is_ready = true;
            }
            break;
        case GATT_EVENT_NOTIFICATION:
            data_len = gatt_event_notification_get_value_length(packet);
            data = gatt_event_notification_get_value(packet);
#if 0
            DEBUG_PRINTF("data rx:");
            for (uint16_t idx = 0; idx < data_len; idx++) {
                DEBUG_PRINTF(" %02x", data[idx]);
            }
            DEBUG_PRINTF("\r\n");
#endif
            ndecoded = ble_midi_pkt_codec_ble_midi_decode_push(data, data_len, ble_midi_pkt_codec_data);
            if (ndecoded != data_len) {
                DEBUG_PRINTF("Parse error decoding midi packet\r\n");
                printf_hexdump(data, data_len);
            }
            break;
        default:
            //DEBUG_PRINTF("unhandled packet type %u", hci_event_packet_get_type(packet));
            break;
    }
}

static bool get_local_name_from_ad_data(uint8_t ad_len, const uint8_t* ad_data, Advertised_MIDI_Peripheral_t* peripheral)
{
    bool success = false;
    ad_context_t context;
    if (peripheral->type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME)
        return false; // don't copy over the complete local name
    
    ad_iterator_init(&context, ad_len, ad_data);
    while (ad_iterator_has_more(&context)) {
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_len  = ad_iterator_get_data_len(&context);
        const uint8_t * data = ad_iterator_get_data(&context);
        switch (data_type){
            case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
            case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
                if (data_len >= sizeof(peripheral->name))
                    data_len = sizeof(peripheral->name) - 1;
                memcpy(peripheral->name, data, data_len);
                peripheral->name[data_len] = '\0';
                peripheral->type = data_type;
                success = true;
                break;
            default:
                break;
        }  
        ad_iterator_next(&context);
    }
    return success;
}

static void midi_service_emit_state(hci_con_handle_t con_handle, bool enabled)
{
    // TODO Right now, all BT Stack messages are handled in this module
    (void)con_handle;
    (void)enabled;
#if 0
    uint8_t event[5];
    uint8_t pos = 0;
    event[pos++] = HCI_EVENT_GATTSERVICE_META;
    event[pos++] = sizeof(event) - 2;
    event[pos++] = enabled ? GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED : GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED;
    little_endian_store_16(event,pos, (uint16_t) con_handle);
    pos += 2;
    (*client_application_packet_handler)(HCI_EVENT_PACKET, 0, event, pos);
#endif
}

static void handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;
    
    uint8_t event = hci_event_packet_get_type(packet);
    const uint8_t* ad_data;
    uint8_t ad_data_len;
    bd_addr_t bdaddr;
    static const uint8_t midi_service_uuid128[] = { 0x03, 0xB8, 0x0E, 0x5A, 0xED, 0xE8, 0x4B, 0x33, 0xA7, 0x51, 0x6C, 0xE3, 0x4E, 0xC4, 0xC7, 0x00 };
    bool mapped; // true if the advertising report is from a BD_ADDR already mapped
    int idx;
    int err;
    static const char * const phy_names[] = {
            "1 M", "2 M", "Codec"
    };
    switch (event) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_OFF) {
                DEBUG_PRINTF("HCI power is safely off\r\n");
                hci_remove_event_handler(&hci_event_callback_registration);
                sm_remove_event_handler(&sm_event_callback_registration);

                sm_deinit();
                btstack_crypto_deinit();
                att_server_deinit();
                l2cap_deinit();
                if (client_profile_data != NULL) {
                    free(client_profile_data);
                    client_profile_data = NULL;
                }
                state = BLEMC_DEINIT;
                con_handle = HCI_CON_HANDLE_INVALID;
                midi_is_ready = false;
                break;
            }
            // BTstack activated, get started
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) {
                DEBUG_PRINTF("client: unhandled BTSTACK_EVENT_STATE %u\r\n", btstack_event_state_get_state(packet));
                break;
            }
            if (state == BLEMC_WAIT_FOR_SCAN_COMPLETE) {
                midi_client.n_midi_peripherals = 0;
                DEBUG_PRINTF("BTstack activated, start active scanning\r\n");
                gap_set_scan_params(1,0x0030, 0x0030,0);
                gap_start_scan();
            }
            else if (state == BLEMC_WAIT_FOR_CONNECTION) {
                if (next_connect_bd_addr_type != BD_ADDR_TYPE_UNKNOWN) {
                    gap_connect(next_connect_bd_addr, next_connect_bd_addr_type);
                }
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            ad_data_len = gap_event_advertising_report_get_data_length(packet);
            ad_data = gap_event_advertising_report_get_data(packet);
            gap_event_advertising_report_get_address(packet, bdaddr);
            idx = find_midi_peripheral(&midi_client, bdaddr);
            mapped = idx < midi_client.n_midi_peripherals;
            if (ad_data_contains_uuid128(ad_data_len, ad_data, midi_service_uuid128) || mapped) {
                if (mapped) {
                    if (midi_client.midi_peripherals[idx].type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
                        midi_client.midi_peripherals[idx].timeout = scan_remove_timeout;
                        break; // no need to repeat ourselves.
                    }
                }
                else {
                    // initialize the map with the BD_ADDR as the name and record the address type
                    midi_client.midi_peripherals[idx].type = 0;
                    // Need the address type to connect
                    midi_client.midi_peripherals[idx].addr_type = gap_event_advertising_report_get_address_type(packet);
                    memcpy(midi_client.midi_peripherals[idx].bdaddr, bdaddr, sizeof(bdaddr));
                    strncpy(midi_client.midi_peripherals[idx].name, bd_addr_to_str(bdaddr), strlen(bd_addr_to_str(bdaddr))+1);
                    midi_client.n_midi_peripherals++;
                }
                midi_client.midi_peripherals[idx].timeout = scan_remove_timeout; // do not time out this entry
                get_local_name_from_ad_data(ad_data_len, ad_data, &midi_client.midi_peripherals[idx]);
                //DEBUG_PRINTF("    * adv. event: addr=%s[%s]\r\n", bd_addr_to_str(bdaddr), midi_client.midi_peripherals[idx].name);
            }
            break;
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    if (state != BLEMC_WAIT_FOR_CONNECTION) {
                        break;
                    }
                    if (hci_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER) {
                        DEBUG_PRINTF("\nCONNECT REQUEST Canceled\r\n");
                        state = BLEMC_IDLE;
                        break;
                    }
                    DEBUG_PRINTF("\nclient: CONNECTED status=%u\n", hci_subevent_le_connection_complete_get_status(packet));
                    con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // print connection parameters (without using float operations)
                    conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
                    DEBUG_PRINTF("Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                    DEBUG_PRINTF("Connection Latency: %u\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
                    // initialize gatt client context with handle, and add it to the list of active clients
                    // query primary services
                    DEBUG_PRINTF("Search for MIDI service.\n");
                    err = gatt_client_discover_primary_services_by_uuid128(handle_gatt_client_event, con_handle, midi_service_uuid128);
                    if (err != ERROR_CODE_SUCCESS)
                        DEBUG_PRINTF("Error(%d): Failed to discover primary services by uuid128\r\n", err);
                    state = BLEMC_WAIT_FOR_SERVICES;
                    midi_service_emit_state(con_handle, true); // pass the connection handle to the client application to this library
                    break;
                case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                    con_handle = hci_subevent_le_data_length_change_get_connection_handle(packet);
                    midi_service_emit_state(con_handle, true); // pass the connection handle to the client application to this library
                    DEBUG_PRINTF("- LE Connection 0x%04x: data length change - max %u bytes per packet\n", con_handle,
                           hci_subevent_le_data_length_change_get_max_tx_octets(packet));
                    break;
                case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                    con_handle = hci_subevent_le_phy_update_complete_get_connection_handle(packet);
                    midi_service_emit_state(con_handle, true); // pass the connection handle to the client application to this library
                    DEBUG_PRINTF("- LE Connection 0x%04x: PHY update - using LE %s PHY now\n", con_handle,
                           phy_names[hci_subevent_le_phy_update_complete_get_tx_phy(packet)]);
                    break;
                case HCI_SUBEVENT_LE_REMOTE_CONNECTION_PARAMETER_REQUEST:
                    DEBUG_PRINTF("HCI_SUBEVENT_LE_REMOTE_CONNECTION_PARAMETER_REQUEST\r\n");
                    // TODO: Do we need to do anything, or is this all handled in the HCI layer?
                    break;
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    DEBUG_PRINTF("HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE\r\n");
                    // print connection parameters (without using float operations)
                    con_handle    = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
                    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                    DEBUG_PRINTF("LE Connection - Connection Param update - connection interval %u.%02u ms, latency %u\n", conn_interval * 125 / 100,
                        25 * (conn_interval & 3), hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            DEBUG_PRINTF("\nclient: DISCONNECTED\n");
            if (listener_registered) {
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            midi_client.n_midi_peripherals = 0;
            midi_service_emit_state(con_handle, false); // pass the connection handle to the client application to this library
            LED_STATE(false);
            con_handle = HCI_CON_HANDLE_INVALID;
            midi_is_ready = false;
            if ((keep_client_connected && state != BLEMC_WAIT_FOR_DISCONNECTION) || state == BLEMC_WAIT_FOR_CONNECTION) { // then disconnected from previous to connect to next
                state = BLEMC_WAIT_FOR_CONNECTION;
                uint8_t status = gap_connect(next_connect_bd_addr, next_connect_bd_addr_type);
                if (status != ERROR_CODE_SUCCESS) {
                    DEBUG_PRINTF("reconnect failed code=%u\r\n", status);
                }
            }
            else {
                state = BLEMC_IDLE;
            }
            break;

        default:
            //DEBUG_PRINTF("client: unhandled HCI_EVENT %u\r\n", event);
            break;
    }
}

static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint16_t idx;

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_IDENTITY_RESOLVING_STARTED:
            DEBUG_PRINTF("SM_EVENT_IDENTITY_RESOLVING_STARTED\r\n");
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED:
            DEBUG_PRINTF("SM_EVENT_IDENTITY_RESOLVING_FAILED\r\n");
            break;
        case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
            DEBUG_PRINTF("SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED\r\n");
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            DEBUG_PRINTF("Just works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            DEBUG_PRINTF("Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            DEBUG_PRINTF("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            DEBUG_PRINTF("Passkey Input requested\n");
            DEBUG_PRINTF("Sending fixed passkey %" PRIu32 "\n", (uint32_t) FIXED_PASSKEY);
            sm_passkey_input(sm_event_passkey_input_number_get_handle(packet), FIXED_PASSKEY);
            break;
        case SM_EVENT_PAIRING_STARTED:
            DEBUG_PRINTF("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    DEBUG_PRINTF("Pairing complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    DEBUG_PRINTF("Pairing failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    DEBUG_PRINTF("Pairing failed, disconnected\n");
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    DEBUG_PRINTF("Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                    break;
                default:
                    break;
            }
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_PRINTF("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                   sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_CREATED:
            idx = sm_event_identity_created_get_index(packet);
            addr_type = sm_event_identity_created_get_addr_type(packet);
            sm_event_identity_created_get_address(packet, addr);
            DEBUG_PRINTF("new bonded idx=%d, addr=%s type=%u\r\n", idx, bd_addr_to_str(addr), addr_type);
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            switch (sm_event_reencryption_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    DEBUG_PRINTF("Re-encryption complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    DEBUG_PRINTF("Re-encryption failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    DEBUG_PRINTF("Re-encryption failed, disconnected\n");
                    break;
                case ERROR_CODE_PIN_OR_KEY_MISSING:
                    DEBUG_PRINTF("Re-encryption failed, bonding information missing\n\n");
                    DEBUG_PRINTF("Assuming remote lost bonding information\n");
                    DEBUG_PRINTF("Deleting local bonding information and start new pairing...\n");
                    sm_event_reencryption_complete_get_address(packet, addr);
                    addr_type = sm_event_reencryption_started_get_addr_type(packet);
                    gap_delete_bonding(addr_type, addr);
                    sm_request_pairing(sm_event_reencryption_complete_get_handle(packet));
                    break;
                default:
                    break;
            }
            break;
        default:
            DEBUG_PRINTF("unhandled SM event 0x%x\r\n",hci_event_packet_get_type(packet));
            break;
    }
}

static void exit_client_mode()
{
    if (ble_midi_client_is_connected())
        ble_midi_client_request_disconnect();
    ble_midi_client_scan_end();
    // Starts a process that ends with mmessage BTSTACK_EVENT_STATE with state HCI_STATE_OFF
    hci_power_control(HCI_POWER_OFF);
#if 0
    hci_remove_event_handler(&hci_event_callback_registration);
    sm_remove_event_handler(&sm_event_callback_registration);
    ble_midi_client_scan_end();

    sm_deinit();
    btstack_crypto_deinit();
    l2cap_deinit();
    if (client_profile_data != NULL) {
        free(client_profile_data);
        client_profile_data = NULL;
    }
    state = BLEMC_DEINIT;
    con_handle = HCI_CON_HANDLE_INVALID;
    midi_is_ready = false;
#endif
}

static void enter_client_mode()
{
    if (state >= BLEMC_IDLE) {
        // already in client mode
        // make sure scan is stopped
        if (state == BLEMC_WAIT_FOR_SCAN_COMPLETE)
            ble_midi_client_scan_end();
        else if (con_handle != HCI_CON_HANDLE_INVALID) {
            state = BLEMC_WAIT_FOR_DISCONNECTION;
            gap_disconnect(con_handle);
        }
        return;
    }

    hci_event_callback_registration.callback = &handle_hci_event;
    hci_add_event_handler(&hci_event_callback_registration);

    // security manager event processing
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);
    // Initialize L2CAP and register HCI event handler
    l2cap_init();
    // set up attribute server in case the device queries the client's name
    att_server_init(client_profile_data, NULL, NULL); 
    // Set up security manager
    sm_init();
    // Initialize GATT client
    gatt_client_init();
    // register for HCI events
    sm_set_io_capabilities(iocaps);
    sm_set_authentication_requirements(secmask);

    // use MIDI connection parameters: connection scan interval 60ms, connection scan window 30ms, connectionconn interval min/max (* 1.25 ms), slave latency, supervision timeout, CE len min/max (* 0.6125 ms) 
    // min con interval 15 ms for iOS per Apple accessory design guidelines, so we have to allow it:
    // https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf
    // If your server can handled a shorter connection interval, please have it request
    // to update the parameters to the shorter interval
    gap_set_connection_parameters(96, 48, 6, 12, 0, 1000, 0x01, 6 * 2);
}

void ble_midi_client_init(const char* profile_name, uint8_t profile_name_len, io_capability_t iocaps_, uint8_t secmask_)
{
    ble_midi_pkt_codec_data = ble_midi_pkt_codec_get_data_by_index(0);
    ble_midi_pkt_codec_init_data(ble_midi_pkt_codec_data, MAX_BLE_MIDI_PACKET);
    const uint8_t base_profile_data[] =
    {
        // ATT DB Version
        1,

        // 0x0001 PRIMARY_SERVICE-GAP_SERVICE
        0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18, 
        // 0x0002 CHARACTERISTIC-GAP_DEVICE_NAME - READ
        0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x02, 0x03, 0x00, 0x00, 0x2a, 
        // 0x0003 VALUE CHARACTERISTIC-GAP_DEVICE_NAME - READ -
        // READ_ANYBODY
        0x08, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x2a,
        // the profile name goes here
        // ...
        // need two 0 bytes to terminate
    };
    // 34 is 1 version byte+10 primary service-gap service bytes+
    // 13 value characteristic-gap device name bytes+
    // 8 characteristic value header bytes + 2 zero termination bytes
    client_profile_data = malloc(34+profile_name_len);
    con_handle = HCI_CON_HANDLE_INVALID;
    midi_is_ready = false;

    if (client_profile_data != NULL) {
        memcpy(client_profile_data, base_profile_data, sizeof(base_profile_data));
        memcpy(client_profile_data+32, profile_name, profile_name_len);
        client_profile_data[24] = 8+profile_name_len;
        client_profile_data[32+profile_name_len] = 0;
        client_profile_data[33+profile_name_len] = 0;
        iocaps = iocaps_;
        secmask = secmask_;
        enter_client_mode();
    }
}

void ble_midi_client_deinit()
{
    exit_client_mode();
}

void ble_midi_client_cancel_connection_request()
{
    gap_connect_cancel();
}

void ble_midi_client_set_last_connected(int addr_type, uint8_t* addr)
{
    if (addr) {
        last_connected_bd_addr_type = addr_type;
        memcpy(last_connected_bd_addr, addr, sizeof(last_connected_bd_addr));
    }
    else {
        last_connected_bd_addr_type = BD_ADDR_TYPE_UNKNOWN;
        memset(last_connected_bd_addr, 0, sizeof(last_connected_bd_addr));
    }
}

int ble_midi_client_get_last_conntected(uint8_t* addr)
{
    int result = last_connected_bd_addr_type;
    if (result != BD_ADDR_TYPE_UNKNOWN) {
        memcpy(addr, last_connected_bd_addr, sizeof(last_connected_bd_addr));
    }
    return result;
}

void ble_midi_client_scan_begin()
{
    if (state == BLEMC_WAIT_FOR_SCAN_COMPLETE)
        return; // already scanning
    // set up the scan timer to report newly discovered devices
    btstack_run_loop_set_timer_handler(&scan_timer, scan_timer_cb);
    btstack_run_loop_set_timer_context(&scan_timer, &midi_client);
    btstack_run_loop_set_timer(&scan_timer, scan_blink_timeout_ms);
    bool turn_on =  (state == BLEMC_DEINIT);
    state = BLEMC_WAIT_FOR_SCAN_COMPLETE;
    if (turn_on)
        hci_power_control(HCI_POWER_ON); // active scan will start when power on event happens
    else {
        midi_client.n_midi_peripherals = 0;
        DEBUG_PRINTF("BTstack activated, start active scanning\n");
        gap_set_scan_params(1,0x0030, 0x0030,0);        
        gap_start_scan();
    }
    btstack_run_loop_add_timer(&scan_timer);
}

void ble_midi_client_scan_end()
{
    if (state != BLEMC_WAIT_FOR_SCAN_COMPLETE)
        return;
    gap_stop_scan();
    btstack_run_loop_remove_timer(&scan_timer);
    LED_STATE(false);
    state = BLEMC_IDLE;
}

void ble_midi_client_dump_midi_peripherals()
{
    DEBUG_PRINTF("Index Bluetooth Address Name\r\n");
    for (uint8_t idx=0; idx < midi_client.n_midi_peripherals; idx++) {
        DEBUG_PRINTF("%d %s '%s' %d\r\n", idx+1, bd_addr_to_str(midi_client.midi_peripherals[idx].bdaddr), midi_client.midi_peripherals[idx].name, midi_client.midi_peripherals[idx].addr_type );
    }
}

void ble_midi_client_get_midi_peripherals(Ble_Scan_Result_t* results, uint8_t* max_n_results)
{
    uint8_t n_results = *max_n_results;
    if (n_results > midi_client.n_midi_peripherals) {
        n_results = midi_client.n_midi_peripherals;
    }

    for (uint8_t idx=0; idx < n_results; idx++) {
        memcpy(results[idx].bdaddr, midi_client.midi_peripherals[idx].bdaddr, BLE_ADDRESS_LENGTH);
        strncpy(results[idx].name, midi_client.midi_peripherals[idx].name, BLE_MAX_DEVICE_NAME_LEN);
    }

    *max_n_results = n_results;
}

bool ble_midi_client_request_connect(uint8_t idx)
{
    if (idx > midi_client.n_midi_peripherals)
        return false;
    if (idx == 0) {
        if (last_connected_bd_addr_type <= (int)BD_ADDR_TYPE_LE_RANDOM_IDENTITY) {
            // for some reason, gap_connect() runs into issues if the addr type is BD_ADDR_TYPE_LE_PUBLIC_IDENTITY but works if it is BD_ADDR_TYPE_LE_PUBLIC
            next_connect_bd_addr_type = (last_connected_bd_addr_type == BD_ADDR_TYPE_LE_PUBLIC_IDENTITY) ? BD_ADDR_TYPE_LE_PUBLIC: last_connected_bd_addr_type;
            memcpy(next_connect_bd_addr, last_connected_bd_addr, sizeof(next_connect_bd_addr));
        }
        else {
            return false;
        }
    }
    else {
        idx -= 1;
        // for some reason, gap_connect() runs into issues if the addr type is BD_ADDR_TYPE_LE_PUBLIC_IDENTITY but works if it is BD_ADDR_TYPE_LE_PUBLIC
        next_connect_bd_addr_type = (midi_client.midi_peripherals[idx].addr_type == BD_ADDR_TYPE_LE_PUBLIC_IDENTITY) ? BD_ADDR_TYPE_LE_PUBLIC: midi_client.midi_peripherals[idx].addr_type;
        memcpy(next_connect_bd_addr, midi_client.midi_peripherals[idx].bdaddr, sizeof(next_connect_bd_addr));
    }
    uint8_t result;
    switch(state) {
        case BLEMC_DEINIT:
            enter_client_mode();
            state = BLEMC_WAIT_FOR_CONNECTION;
            //when power on completes, will request connection
            hci_power_control(HCI_POWER_ON);
            break;
        case BLEMC_IDLE:
            state = BLEMC_WAIT_FOR_CONNECTION;
            result = gap_connect(next_connect_bd_addr, next_connect_bd_addr_type);
            if (ERROR_CODE_SUCCESS != result) {
                DEBUG_PRINTF("gap_connect: Bluetooth error code %u", result);
            }
            break;
        case BLEMC_WAIT_FOR_DISCONNECTION:
        case BLEMC_WAIT_FOR_SERVICES:
        case BLEMC_WAIT_FOR_CHARACTERISTICS:
        case BLEMC_WAIT_FOR_ENABLE_NOTIFICATIONS_COMPLETE:
        case BLEMC_WAIT_FOR_MIDI_DATA_RX:
            if (con_handle == HCI_CON_HANDLE_INVALID)
                return false; // does not make sense
            state = BLEMC_WAIT_FOR_CONNECTION;
            result = gap_disconnect(con_handle);
            if (ERROR_CODE_SUCCESS != result) {
                DEBUG_PRINTF("gap_disconnect: Bluetooth error code %u", result);
            }
            break;
        case BLEMC_WAIT_FOR_CONNECTION:
            if (con_handle != HCI_CON_HANDLE_INVALID) {
                return false; // does not make sense
            }
            state = BLEMC_WAIT_FOR_CONNECTION;
            result = gap_connect(next_connect_bd_addr, next_connect_bd_addr_type);
            if (ERROR_CODE_SUCCESS != result) {
                DEBUG_PRINTF("gap_connect: Bluetooth error code %u", result);
            }
            break;
        case BLEMC_WAIT_FOR_SCAN_COMPLETE:
            if (con_handle != HCI_CON_HANDLE_INVALID) {
                return false; // does not make sense
            }
            gap_stop_scan();
            state = BLEMC_WAIT_FOR_CONNECTION;
            result = gap_connect(next_connect_bd_addr, next_connect_bd_addr_type);
            if (ERROR_CODE_SUCCESS != result) {
                DEBUG_PRINTF("gap_connect: Bluetooth error code %u", result);
            }
            break;
        default:
            return false; // should not get here
            break;
    }

    return true;
}

void ble_midi_client_request_disconnect()
{
    if (state > BLEMC_WAIT_FOR_SCAN_COMPLETE && ble_midi_client_is_connected()) {
        state = BLEMC_WAIT_FOR_DISCONNECTION;
        uint8_t result = gap_disconnect(con_handle);
        if (ERROR_CODE_SUCCESS != result) {
            DEBUG_PRINTF("gap_disconnect: Bluetooth error code %u", result);
        }
    }
}

static void handle_can_write_without_response(void * context)
{
    (void)context;
    ble_midi_packet_t pkt;
    if (ble_midi_pkt_codec_ble_pkt_pop(&pkt, ble_midi_pkt_codec_data))
        gatt_client_write_value_of_characteristic_without_response(con_handle, midi_data_io_characteristic.value_handle, pkt.nbytes, pkt.pkt);
    // ready next packet to send if there is one buffered
    if (ble_midi_pkt_codec_ble_pkt_available(ble_midi_pkt_codec_data)) {
        write_callback_registration.callback = handle_can_write_without_response;
        write_callback_registration.context = NULL;

        gatt_client_request_to_write_without_response(&write_callback_registration, con_handle);
    }
}


uint8_t ble_midi_client_stream_write(uint8_t nbytes, const uint8_t* midi_stream_bytes)
{
    // do not send data in unconnected state, otherwise a crash will occur on reconnect
    if (!ble_midi_client_is_ready()) {
        return 0;
    }

    uint8_t bytes_written = 0;
    bool ready_to_send = false;
    bytes_written = ble_midi_pkt_codec_push_midi(midi_stream_bytes, nbytes, ble_midi_pkt_codec_data, &ready_to_send);
    if (bytes_written > 0) {
        if (ready_to_send) {
            write_callback_registration.callback = handle_can_write_without_response;
            write_callback_registration.context = NULL;

            gatt_client_request_to_write_without_response(&write_callback_registration, con_handle);
        }

    }
    return bytes_written;
}

uint8_t ble_midi_client_stream_read(uint8_t max_bytes, uint8_t* midi_stream_bytes, uint16_t* timestamp)
{
    ble_midi_message_t mes;
    uint8_t nread = 0;
    uint8_t nbytes = ble_midi_pkt_codec_pop_midi(&mes, ble_midi_pkt_codec_data);
    if (nbytes == sizeof(mes)) {
        uint8_t bytecount = mes.nbytes & ble_midi_packet_nbytes_mask;
        if (bytecount <= max_bytes) {
            *timestamp = mes.timestamp_ms;
            memcpy(midi_stream_bytes, mes.msg_bytes, bytecount);
            nread = bytecount;
        }
    }

    return nread;
}

bool ble_midi_client_stream_read_available(void)
{
    return ble_midi_pkt_codec_ble_pkt_available(ble_midi_pkt_codec_data);
}

bool ble_midi_client_is_connected(void)
{
    return con_handle != HCI_CON_HANDLE_INVALID;
}

bool ble_midi_client_is_ready(void)
{
    return midi_is_ready;
}

bool ble_midi_client_is_off(void)
{
    return state == BLEMC_DEINIT;
}

bool ble_midi_client_waiting_for_connection()
{
    return state == BLEMC_WAIT_FOR_CONNECTION;
}

bool ble_midi_client_get_keep_connected()
{
    return keep_client_connected;
}

void ble_midi_client_set_keep_connected(bool keep_client_connected_)
{
    keep_client_connected = keep_client_connected_;
}

void ble_midi_client_set_scan_callback(ble_midi_client_scan_callback_t scan_callback_)
{
    scan_callback = scan_callback_;
}