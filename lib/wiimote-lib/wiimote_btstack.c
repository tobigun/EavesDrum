#include "wiimote_btstack.h"
#include <math.h>
#include "btstack.h"
#include "debug.h"

#include "sdp_consts.h"
#include "wiimote.h"
#include "motion.h"
#include <btstack_tlv.h>
#include "pico/flash.h"
#include <cyw43.h>

#define CONNECTION_TIMEOUT_MS 2000

// See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers if you don't have a USB Vendor ID and need a Bluetooth Vendor ID
// device info: BlueKitchen GmbH, product 1, version 1
#define DEVICE_VENDOR_ID           BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH
#define DEVICE_VENDOR_ID_SOURCE    DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH
#define DEVICE_PRODUCT_ID 1
#define DEVICE_VERSION 1
#define HID_COUNTRY_CODE 33 /*US*/

// For documentation: these are the original SDP infos
// Pairing works perfectly fine without this
//#define DEVICE_VENDOR_ID           0x057e // Nintendo
//#define DEVICE_VENDOR_ID_SOURCE    DEVICE_ID_VENDOR_ID_SOURCE_USB
//#define HID_SERVICE_PROVIDER_NAME  "Nintendo"

#ifdef WIIMOTE_TR
//#define DEVICE_OUI {0x40, 0xD2, 0x8A}
//#define DEVICE_PRODUCT_ID  0x0330
//#define DEVICE_VERSION 1
#define DEVICE_NAME  "Nintendo RVL-CNT-01-TR"
#define DEVICE_CLASS 0x508
//#define HID_COUNTRY_CODE 0
#else
//#define DEVICE_OUI {0x78, 0xA2, 0xA0}
//#define DEVICE_PRODUCT_ID  0x0306
//#define DEVICE_VERSION 0x8600
#define DEVICE_NAME  "Nintendo RVL-CNT-01"
#define DEVICE_CLASS 0x2504
//#define HID_COUNTRY_CODE 33 /*US*/
#endif

static bd_addr_t wii_host_addr;
static link_key_t wii_host_link_key;

static WiimoteConnectionCallback connection_callback;

static uint8_t hid_service_buffer[700];
static uint8_t pnp_service_buffer[200];
static const char hid_device_name[] = "Nintendo RVL-CNT-01";
static uint16_t hid_cid;
static struct wiimote_state wiimote;

static uint8_t buf[256];

static btstack_timer_source_t loop_wii_timer;
static btstack_timer_source_t reconnect_timer;
static btstack_timer_source_t shutdown_timer;

static bool isPairing = false;
static btstack_timer_source_t pairing_timer;
static PairingType_t pairing_type;

static WiimoteLedCallback set_led_state;
static btstack_timer_source_t led_state_timer;

static const double pointer_margin = 0.5;

static float pointer_x = 0.5;
static float pointer_y = 0.5;

static WiimoteReport input_report;

static WiimoteReportCallback report_callback;

static bool input_update_wiimote();
static void wiimote_bluetooth_init();
static void wiimote_bluetooth_deinit(bool restart);


static btstack_packet_callback_registration_t hci_event_callback_registration;

static uint32_t last_send_ms = 0;

static void send_data()
{   
    bool input_report_changed = false;
    bool handle_queue = wiimote.sys.queue != NULL;
    if (!handle_queue) {
        input_report_changed = input_update_wiimote();
    }

    int buf_in_len = generate_report(&wiimote, buf, input_report_changed); // buf is reused for input report
    if (buf_in_len > 0){
        #if 0
        log_printf("Send [0x%x]: ", buf[1]);
        printf_hexdump(buf, buf_in_len);
        #endif
        hid_device_send_interrupt_message(hid_cid, buf, buf_in_len);
    }
    if (handle_queue || wiimote.sys.queue != NULL) {
        hid_device_request_can_send_now_event(hid_cid);
    }

    last_send_ms = btstack_run_loop_get_time_ms();
}

static void led_handler(struct btstack_timer_source *ts)
{
    // Invert the led
    static bool led_on = false;
    led_on = !led_on;
    set_led_state(led_on);

    // Restart timer
    btstack_run_loop_set_timer(ts, 100);
    btstack_run_loop_add_timer(ts);
}

static void stop_sync() {
    if (!isPairing) {
        return;
    }

    btstack_run_loop_remove_timer(&pairing_timer);
    btstack_run_loop_remove_timer(&led_state_timer);

    set_led_state(false);

    gap_discoverable_control(0);
    
    log_printf("Pairing stopped\n");
    isPairing = false;
}

static void start_pairing(PairingType_t type) {
    pairing_type = type;
    btstack_run_loop_remove_timer(&reconnect_timer);

    gap_discoverable_control(1);
    
    btstack_run_loop_set_timer(&pairing_timer, 2 * 60 * 1000);
    btstack_run_loop_add_timer(&pairing_timer);

    // Start blinking led
    btstack_run_loop_set_timer(&led_state_timer, 100);
    btstack_run_loop_add_timer(&led_state_timer);

    log_printf("Pairing started\n");
    isPairing = true;
}

static void task_sync_timeout(struct btstack_timer_source *ts)
{
    stop_sync();
}

static void task_wiimote(struct btstack_timer_source *ts)
{
    bool stall_detected = hid_cid && last_send_ms && btstack_run_loop_get_time_ms() - last_send_ms > CONNECTION_TIMEOUT_MS;
    if (stall_detected) {
        // BT peripheral controller uses an (as it seems unchangeable) 20s supervision timeout.
        // When switching from Wii U to Wii mode, the Wii U does not close the connection properly.
        // As a consequence the connection is stalled for 20s until the connection is closed. During this time
        // neither hid_device_disconnect() nor hci_disconnect work.
        // The only way not to wait for 20s seems to perform a BT stack or full device reset.
        log_printf("HID Timeout: restart\n");
        wiimote_bluetooth_deinit(true);
        return;
    }

    hid_device_request_can_send_now_event(hid_cid);

    // Restart timer
    btstack_run_loop_set_timer(ts, 1);
    btstack_run_loop_add_timer(ts);
}

// hid_device_connect() might fail in some cases (e.g. after a stack reset).
// This method triggers a reconnect in this case
static void hid_safe_device_connect() {
    if (btstack_is_null_bd_addr(wii_host_addr)) {
        return;
    }

    log_printf("Connect: %s\n", bd_addr_to_str(wii_host_addr));

    uint8_t res = hid_device_connect(wii_host_addr, &hid_cid);
    if (res) {
        log_printf("Connection failed: 0x%0x\n", res);
        btstack_run_loop_set_timer(&reconnect_timer, 2000);
        btstack_run_loop_add_timer(&reconnect_timer);
    }
}

static void task_reconnect(struct btstack_timer_source *ts)
{
    set_led_state(true);
    hid_safe_device_connect();
}

void wiimote_emulator_pair_remote(PairingType_t type) {
    if (!isPairing) {
        if (hid_cid != 0) {
            hid_device_disconnect(hid_cid);
            memset(wii_host_addr, 0, sizeof(wii_host_addr));
        }
        start_pairing(type);
    }
}

static void receive_data_wii(uint16_t cid, hid_report_type_t report_type, uint16_t report_id, int report_size, uint8_t * report)
{
    if(report_type == HID_REPORT_TYPE_OUTPUT){
        process_report(&wiimote, report_id, report, report_size);
        #if 0
        logInfo("Recv 0x%x: ", report_id);
        printf_hexdump(report, report_size);
        #endif
        if (wiimote.sys.queue != NULL) {
            hid_device_request_can_send_now_event(cid);
        }
    }
}

static void restore_link_key() {
    link_key_t stored_link_key;
    link_key_type_t stored_link_key_type;
    bool link_key_provided = !btstack_is_null(wii_host_link_key, sizeof(link_key_t));
    if (link_key_provided && !gap_get_link_key_for_bd_addr(wii_host_addr, stored_link_key, &stored_link_key_type)) {
        // Pico TLV implementation uses the binary flash region to store data. So keys are erased whenever the firmware is updated.
        // Use an alternative storage method to restore the keys.
        log_printf("Restore link-key\n");
        gap_store_link_key_for_bd_addr(wii_host_addr, wii_host_link_key, COMBINATION_KEY);
    }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    // We only care about HCI packets
    if (packet_type != HCI_EVENT_PACKET) {
        //log_printf("Non-HCI %d\n", packet_type);
        return;
    }

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type) {
        case BTSTACK_EVENT_STATE: {
            // Wait for the stack to enter the initializing state, before setting the IAC LAP
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                hci_send_cmd(&hci_write_current_iac_lap_two_iacs, 2, GAP_IAC_GENERAL_INQUIRY, GAP_IAC_LIMITED_INQUIRY);
                restore_link_key();
            }
            break;
        }
        case HCI_EVENT_COMMAND_COMPLETE:
            // Check IAC LAP result
            if (hci_event_command_complete_get_command_opcode(packet) == HCI_OPCODE_HCI_WRITE_CURRENT_IAC_LAP_TWO_IACS) {
                uint8_t status = hci_event_command_complete_get_return_parameters(packet)[0];
                log_printf("Set IAC LAP: %s\n", (status != ERROR_CODE_SUCCESS) ? "Failed" : "OK");
                if (status == ERROR_CODE_SUCCESS) {
                    if (!btstack_is_null_bd_addr(wii_host_addr)) {
                        hid_safe_device_connect();
                    } else {
                        start_pairing(PAIRING_SYNC);
                    }
                }
            }
            break;
        case HCI_EVENT_PIN_CODE_REQUEST: {
            // inform about pin code request
            uint8_t pin_code[6];
            bd_addr_t remote_addr;
            hci_event_pin_code_request_get_bd_addr(packet, remote_addr);
            if (pairing_type == PAIRING_SYNC) {
                reverse_bd_addr(remote_addr, pin_code);
            } else { // PAIRING_GUEST: for temporary pairing with 1+2 button
                bd_addr_t local_addr;
                gap_local_bd_addr(local_addr);
                reverse_bd_addr(local_addr, pin_code);
            }
            log_printf("Pin request: %s\n", bd_addr_to_str(pin_code));
            gap_pin_code_response_binary(remote_addr, pin_code, sizeof(pin_code));
            break;
        }
        case HCI_EVENT_HID_META:
            uint8_t hid_event = hci_event_hid_meta_get_subevent_code(packet);
            switch (hid_event) {
                case HID_SUBEVENT_CONNECTION_OPENED:
                    btstack_run_loop_remove_timer(&reconnect_timer);

                    uint8_t status = hid_subevent_connection_opened_get_status(packet);
                    if (status != ERROR_CODE_SUCCESS) {
                        // outgoing connection failed
                        hid_cid = 0;
                        // Authentication Failure -> Need to Sync again with the console
                        if (status == 0x66) {
                            log_printf("Authentication failure -> Need to Sync again\n");
                        } else {
                            log_printf("Connection failed, status 0x%x\n", status);
                        }

                        set_led_state(false);
                        btstack_run_loop_remove_timer(&loop_wii_timer);
                        btstack_run_loop_set_timer(&reconnect_timer, 2000);
                        btstack_run_loop_add_timer(&reconnect_timer);
                        return;
                    }
                    
                    hid_subevent_connection_opened_get_bd_addr(packet, wii_host_addr);
                    hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);

                    stop_sync();

                    // Set the led on
                    set_led_state(true);

                    link_key_type_t link_key_type;
                    gap_get_link_key_for_bd_addr(wii_host_addr, wii_host_link_key, &link_key_type);
                    connection_callback(wii_host_addr, wii_host_link_key);

                    // Run wiimote process
                    last_send_ms = 0;
                    btstack_run_loop_set_timer(&loop_wii_timer, 1);
                    btstack_run_loop_add_timer(&loop_wii_timer);                        

                    break;
                case HID_SUBEVENT_CONNECTION_CLOSED:
                    log_printf("HID Disconnected\n");
                    if (hid_cid != 0) {
                        hid_cid = 0;

                        // Stop wiimote process
                        btstack_run_loop_remove_timer(&loop_wii_timer);
                        hid_safe_device_connect();
                    }
                    break;
                case HID_SUBEVENT_CAN_SEND_NOW:
                    send_data();
                    break;
                default:
                    //log_printf("HID-Event: 0x%x", hid_event);
                    break;
            }
            break;
        default:
            //log_printf("HCI-Event: 0x%x", event_type);
            break;
    }
}

// Needs linker setting:
// -Wl,--wrap=hci_transport_cyw43_instance
#ifdef DEVICE_OUI

const hci_transport_t *hci_transport_cyw43_instance(void);
extern int cyw43_bluetooth_hci_init(void);

static int (*real_hci_transport_cyw43_open)(void);

static int hci_transport_cyw43_open(void) {    
    int res = real_hci_transport_cyw43_open();
    
    bd_addr_t addr;
    cyw43_hal_get_mac(0, (uint8_t*)&addr);
    addr[BD_ADDR_LEN - 1]++; // use WiFi addr + 1
    const uint8_t bt_oui[] = DEVICE_OUI;
    memcpy(addr, bt_oui, 3); // and replace OUI
    hci_set_bd_addr(addr);

    return res;
}

extern const hci_transport_t *__real_hci_transport_cyw43_instance(void);

const hci_transport_t *__wrap_hci_transport_cyw43_instance(void) {
    static bool initialized = false;
    static hci_transport_t transport_wii;
    const hci_transport_t * transport = __real_hci_transport_cyw43_instance();
    if (!initialized) {
        real_hci_transport_cyw43_open = transport->open;
        transport_wii = *transport;
        transport_wii.open = &hci_transport_cyw43_open;
        initialized = true;
    }
    return &transport_wii;
}

#endif

static void add_browse_group_list(uint8_t * service_record){
    de_add_number(service_record,  DE_UINT, DE_SIZE_16, 0x0101);
    de_add_data(service_record,  DE_STRING, (uint16_t) strlen(hid_device_name), (uint8_t *) hid_device_name);

#ifdef HID_SERVICE_PROVIDER_NAME
    static const char hid_provider_name[] = HID_SERVICE_PROVIDER_NAME;
    de_add_number(service_record,  DE_UINT, DE_SIZE_16, 0x0102);
    de_add_data(service_record,  DE_STRING, (uint16_t) strlen(hid_provider_name), (uint8_t *) hid_provider_name);
#endif

    de_add_number(service_record, DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_BROWSE_GROUP_LIST);
    uint8_t * attribute = de_push_sequence(service_record);
    de_add_number(attribute, DE_UUID, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_PUBLIC_BROWSE_ROOT);
    de_pop_sequence(service_record, attribute);
}

static void wiimote_bluetooth_init()
{
    // Disable SSP (will enable PIN authentication and reconnection with authentication links)
    gap_ssp_set_enable(0);

    // Allow pairing without PIN (as original)
    gap_set_security_level(LEVEL_0);

    // Set local name
    gap_set_local_name(DEVICE_NAME);
    
    // Set class
    gap_set_class_of_device(DEVICE_CLASS);

    // Set device bondable
    gap_set_bondable_mode(1);

    // Set device non discoverable for now, we'll set this after setting IAC LAP
    gap_discoverable_control(0);

    // It's weird but this fix the lag on sticks 
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE);

    // Register HCI callback to set IAC LAP once HCI is working
    // and to catch the connection handle
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // init L2CAP
    l2cap_init();

    // SDP Server
    sdp_init();
    gap_connectable_control(1);

    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));
    hid_sdp_record_t hid_sdp_record = {
        .hid_device_subclass = 0x04,
        .hid_country_code = HID_COUNTRY_CODE,
        .hid_virtual_cable = 0,
        .hid_remote_wake = 1,
        .hid_reconnect_initiate = 1,
        .hid_normally_connectable = 0,
        .hid_boot_device = 0,
        .hid_ssr_host_max_latency = 1600,
        .hid_ssr_host_min_timeout = 3200,
        .hid_supervision_timeout = 3200, // 2s
        .hid_descriptor = wiimote_report_descriptor,
        .hid_descriptor_size = sizeof(wiimote_report_descriptor),
        .device_name = hid_device_name
    };

    // Differences to original:
    // - HID version: 0x0101 instead of 0x0100
    // - no BLUETOOTH_ATTRIBUTE_HID_DEVICE_RELEASE_NUMBER=0x0100 (deprecated in v1.1.1)
    hid_create_sdp_record(hid_service_buffer, 0x10000, &hid_sdp_record);
    add_browse_group_list(hid_service_buffer);
    sdp_register_service(hid_service_buffer);

    device_id_create_sdp_record(pnp_service_buffer, sdp_create_service_record_handle(),
        DEVICE_VENDOR_ID_SOURCE,
        DEVICE_VENDOR_ID, DEVICE_PRODUCT_ID, DEVICE_VERSION);
    sdp_register_service(pnp_service_buffer);

     // register for HID events
    hid_device_register_packet_handler(&hci_packet_handler);
    hid_device_init(0, sizeof(wiimote_report_descriptor), wiimote_report_descriptor);

    // HID Device report callback 
    hid_device_register_report_data_callback(&receive_data_wii);

    // Power on!
    hci_power_control(HCI_POWER_ON);
}

static void task_perform_shutdown_action(struct btstack_timer_source *ts) {
    if (hci_get_state() != HCI_STATE_OFF) {
        btstack_run_loop_set_timer(&shutdown_timer, 1);
        btstack_run_loop_add_timer(&shutdown_timer);
        return;
    }

    log_printf("HCI shut down: cleanup\n");
    hid_device_deinit();
    l2cap_deinit();
    sdp_deinit();

    bool restart = (bool) ts->context;
    if (restart) {
        log_printf("HCI restart\n");
        wiimote_bluetooth_init();
    }
}

static void wiimote_bluetooth_deinit(bool restart) {
    btstack_run_loop_remove_timer(&loop_wii_timer);
    btstack_run_loop_remove_timer(&reconnect_timer);
    btstack_run_loop_remove_timer(&pairing_timer);
    btstack_run_loop_remove_timer(&led_state_timer);

    hid_cid = 0;
    set_led_state(false);
    
    log_printf("HCI power down\n");
    hci_power_control(HCI_POWER_OFF);

    // we have to wait for power off state until we can clean up the stack
    btstack_run_loop_set_timer_context(&shutdown_timer, (void*)restart);
    btstack_run_loop_set_timer(&shutdown_timer, 1);
    btstack_run_loop_add_timer(&shutdown_timer);
}

void wiimote_emulator_init(struct WiimoteConfig* config)
{
    log_printf("Init Wiimote Emulator\n");

    memcpy(wii_host_addr, config->wii_addr, sizeof(wii_addr_t));
    memcpy(wii_host_link_key, config->wii_link_key, sizeof(wii_link_key_t));

    memset(&input_report, 0, sizeof(input_report));

    // Data from gamepad
    report_callback = config->report_callback;
    connection_callback = config->connection_callback;

    set_led_state = config->set_led_state_callback;
    btstack_run_loop_set_timer_handler(&led_state_timer, &led_handler);

    // Init wiimote structure
    wiimote_init(&wiimote);
    
    //No extension for default
    reset_input_ir(wiimote.usr.ir_object);
    wiimote.sys.wmp_connected = config->has_wmp;
    wiimote.sys.has_speaker = config->has_speaker;
    wiimote.usr.connected_extension_type = NoExtension;
    
    btstack_run_loop_set_timer_handler(&loop_wii_timer, &task_wiimote);
    btstack_run_loop_set_timer_handler(&reconnect_timer, &task_reconnect);
    btstack_run_loop_set_timer_handler(&pairing_timer, &task_sync_timeout);
    btstack_run_loop_set_timer_handler(&shutdown_timer, &task_perform_shutdown_action);

    wiimote_bluetooth_init();

    // not required for earlephilhower Pico Arduino Stack
    //btstack_run_loop_execute();
}

void wiimote_emulator_deinit() {
    wiimote_bluetooth_deinit(false);
}

static bool input_update_wiimote(){
    bool changed = report_callback(&input_report);
    if (!changed) {
        return false;
    }

    if(input_report.reset_ir){
        reset_input_classic(&wiimote.usr.classic);
        reset_input_nunchuk(&wiimote.usr.nunchuk);
        reset_input_ir(wiimote.usr.ir_object);
        // Change the extension too
        wiimote.usr.connected_extension_type = input_report.extension;
        input_report.reset_ir = 0;
    }

    switch(input_report.extension){
        case NoExtension:{
            memcpy(&wiimote.usr, &input_report.wiimote, sizeof(struct wiimote_buttons_only));

            float pointer_delta_x = 0, pointer_delta_y = 0;
            pointer_delta_x += (input_report.wiimote.ir_x / 127.0) * 0.008;
            pointer_delta_y += (input_report.wiimote.ir_y / 127.0) * 0.008;

            pointer_x = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_x + pointer_delta_x));
            pointer_y = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_y + pointer_delta_y));

            set_motion_state(&wiimote, pointer_x, pointer_y);
        }
            break;
        case Nunchuk:{
            // Wiimote
            memcpy(&wiimote.usr, &input_report.wiimote, sizeof(struct wiimote_buttons_only));

            // Nunchuck
            memcpy(&wiimote.usr.nunchuk, &input_report.nunchuk, sizeof(struct wiimote_nunchuk));

            float pointer_delta_x = 0, pointer_delta_y = 0;
            pointer_delta_x += (input_report.wiimote.ir_x / 127.0) * 0.008;
            pointer_delta_y += (input_report.wiimote.ir_y / 127.0) * 0.008;

            pointer_x = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_x + pointer_delta_x));
            pointer_y = fmax(-pointer_margin, fmin(1.0 + pointer_margin, pointer_y + pointer_delta_y));

            set_motion_state(&wiimote, pointer_x, pointer_y);

            if(input_report.fake_motion || input_report.center_accel){

                if(input_report.fake_motion) {
                    static int step = 48;

                    if(input_report.wiimote.accel_x >= 1000 || input_report.wiimote.accel_x <= 100){
                        step = -step;
                    }

                    // Start moving all directions

                    int temp_value = step + input_report.wiimote.accel_x;

                    input_report.wiimote.accel_x = (uint16_t)temp_value;
                    input_report.nunchuk.accel_x = (uint16_t)temp_value;

                    input_report.wiimote.accel_y = (uint16_t)temp_value;
                    input_report.nunchuk.accel_y = (uint16_t)temp_value;

                    input_report.wiimote.accel_z = (uint16_t)temp_value;
                    input_report.nunchuk.accel_z = (uint16_t)temp_value;
                }

                wiimote.usr.accel_x = input_report.wiimote.accel_x;
                wiimote.usr.accel_y = input_report.wiimote.accel_y;
                wiimote.usr.accel_z = input_report.wiimote.accel_z;

                if(input_report.center_accel){
                    input_report.center_accel = 0;
                }
            }
        }
            break;
        case Classic:
            memcpy(&wiimote.usr, &input_report.wiimote, sizeof(struct wiimote_buttons_only));
            memcpy(&wiimote.usr.classic, &input_report.classic, sizeof(struct wiimote_classic));
            break;
        default:
            break;
    }

    return true;
}
